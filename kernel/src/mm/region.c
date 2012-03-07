/*
 * region.c
 *
 *  Copyright 2012 James Cowgill
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  Created on: 14 Oct 2010
 *      Author: James
 */

#include "chaff.h"
#include "inlineasm.h"
#include "process.h"
#include "mm/region.h"
#include "mm/pagingInt.h"
#include "mm/physical.h"
#include "mm/misc.h"
#include "mm/kmemory.h"

#warning TODO Copy-On-Write Page tables

//Kernel context
MemContext MemKernelContextData = { LIST_INLINE_INIT(MemKernelContextData.regions), INVALID_PAGE, 0x1000 };
	//INVALID_PAGE changed in MemManagerInit

//Current context
MemContext * MemCurrentContext = MemKernelContext;

//Check if a region will run into another
static bool MemRegionIsCollision(MemRegion * thisRegion, MemRegion * nextRegion);

//Creates a new blank memory context
MemContext * MemContextInit()
{
	//Allocate new context
	MemContext * newContext = MemKAlloc(sizeof(MemContext));
	ListHeadInit(&newContext->regions);

	//Allocate directory
	newContext->physDirectory = MemPhysicalAlloc(1, MEM_KERNEL);
	newContext->refCount = 0;

	//Get directory pointer
	MemPageDirectory * dir = MemPhys2Virt(newContext->physDirectory);

	//Wipe user area
	MemSet(dir, 0, sizeof(MemPageDirectory) * 768);

	//Copy kernel area
	MemCpy(dir + 0x300, MemKernelPageDirectory + 0x300, sizeof(MemPageDirectory) * 256);

	//Return context
	return newContext;
}

//Clones this memory context
MemContext * MemContextClone()
{
	//Allocate new context
	MemContext * newContext = MemKAlloc(sizeof(MemContext));
	ListHeadInit(&newContext->regions);

	//Copy regions
	MemRegion * oldRegion;
	ListForEachEntry(oldRegion, &MemCurrentContext->regions, listItem)
	{
		//Allocate new region
		MemRegion * newRegion = MemKAlloc(sizeof(MemRegion));

		//Copy region data
		MemCpy(newRegion, oldRegion, sizeof(MemRegion));

		//Set context
		newRegion->myContext = newContext;

		//Add to new list
		ListHeadAddLast(&newRegion->listItem, &newContext->regions);
	}

	//Allocate directory
	newContext->physDirectory = MemPhysicalAlloc(1, MEM_KERNEL);
	newContext->refCount = 0;

	//Copy kernel area
	MemPageDirectory * dir = MemPhys2Virt(newContext->physDirectory);
	MemCpy(dir + 0x300, MemKernelPageDirectory + 0x300, sizeof(MemPageDirectory) * 256);

	//Get CURRENT page directory
	MemPageDirectory * currDir = MemPhys2Virt(MemCurrentContext->physDirectory);

	//Copy all the page tables and increase refcounts on all pages
	for(int i = 0; i < 0x300; ++i)
	{
		//Copy directory entry
		dir[i] = currDir[i];

		//If present, increase page ref counts first
		if(currDir[i].present)
		{
			MemPageTable * table = MemPhys2Virt(currDir[i].pageID);

			//Increase ref count on any pages
			for(int j = 0; j < 1024; ++j)
			{
				if(table[j].present)
				{
					//Increase count and make readonly
					MemPhysicalAddRef(table[j].pageID, 1);

					//Fixed pages are made writable again in the page fault handler
					table[j].writable = 0;
				}
			}

			//Duplicate page table
			MemPhysPage newTable = MemPhysicalAlloc(1, MEM_KERNEL);
			MemCpy(MemPhys2Virt(newTable), table, sizeof(MemPageTable) * 1024);

			//Store in directory
			dir[i].pageID = newTable;
		}
	}

	//Flush user mode paging caches
	setCR3(getCR3());

	//Return context
	return newContext;
}

//Switches to this memory context
void MemContextSwitchTo(MemContext * context)
{
	//Check for valid directory
	if(context->physDirectory == INVALID_PAGE)
	{
		PrintLog(Critical, "MemContextSwitchTo: Invalid memory context passed.");
		return;
	}

	//Switch to directory
	setCR3((unsigned int) context->physDirectory * 4096);

	//Save current context
	MemCurrentContext = context;
}

//Deletes this memory context - DO NOT delete the memory context
// which is currently in use!
void MemContextDelete(MemContext * context)
{
	//Check current context
	if((unsigned int) context->physDirectory == getCR3() / 4096)
	{
		PrintLog(Critical, "MemContextDelete: Cannot delete current memory context.");
		return;
	}
	else if(context->physDirectory == INVALID_PAGE)
	{
		PrintLog(Critical, "MemContextDelete: Invalid memory context passed.");
		return;
	}
	else if(context == MemKernelContext)
	{
		PrintLog(Critical, "MemContextDelete: Cannot delete kernel memory context.");
		return;
	}

	//Get root directory
	MemPageDirectory * dir = MemPhys2Virt(context->physDirectory);

	//Process user mode tables
	for(int i = 0; i < 0x300; ++i)
	{
		//If present, free pages in page table first
		if(dir[i].present)
		{
			MemPageTable * table = MemPhys2Virt(dir[i].pageID);

			//Free pages
			for(int j = 0; j < 1024; ++j)
			{
				//Free page if present
				if(table[j].present)
				{
					MemPhysicalDeleteRef(table[j].pageID, 1);
				}
			}

			//Free this table
			MemPhysicalFree(dir[i].pageID, 1);
		}
	}

	//Free directory
	MemPhysicalFree(context->physDirectory, 1);

	//Free regions
	MemRegion * region, * tmpRegion;
	ListForEachEntrySafe(region, tmpRegion, &context->regions, listItem)
	{
		//Free region
		MemKFree(region);
	}

	//Free the final context
	MemKFree(context);
}

//Deletes a reference to a memory context
void MemContextDeleteReference(MemContext * context)
{
	if(context->refCount <= 1)
	{
		//Delete context
		MemContextDelete(context);
	}
	else
	{
		//Decrement ref count
		context->refCount--;
	}
}

//Frees the pages associated with a given region of memory without destroying the region
void MemRegionFreePages(MemRegion * region, void * address, unsigned int length)
{
	//Check range is within region
	unsigned int startAddr = (unsigned int) address;
	if(startAddr < region->start || startAddr >= (region->start + region->length))
	{
		PrintLog(Warning, "MemRegionFreePages: Memory region passed outside region limits");
		return;
	}

	//Round addresses inwards (only whole pages are freed)
	unsigned int endAddr = (startAddr + length) / 4096;
	startAddr = (startAddr + 4095) / 4096;

	//Free pages
	MemContext * context = region->myContext;

	for(; startAddr < endAddr; startAddr += 4096)
	{
		MemIntUnmapUserPageAndFree(context, (void *) startAddr);
	}
}

//Finds the region which contains the given address
MemRegion * MemRegionFind(MemContext * context, void * address)
{
	unsigned int addr = (unsigned int) address;

	//Check kernel context
	if(context == MemKernelContext)
	{
		PrintLog(Error, "MemRegionFind: this function does not work with the kernel context");
		return NULL;
	}

	//Check each address
	MemRegion * region;
	ListForEachEntry(region, &context->regions, listItem)
	{
		//Is region beyond address?
		if(region->start > addr)
		{
			//Not found
			break;
		}

		//Is region lower than address?
		if(addr < region->start + region->length)
		{
			//Region found
			return region;
		}
	}

	//Can't find
	return NULL;
}

//Check if a region will run into another
static bool MemRegionIsCollision(MemRegion * prevRegion, MemRegion * nextRegion)
{
	//This checks that the next region starts after the previous region
	// and does a check that prevRegion doesn't wrap around (fooling the results)
	return !((nextRegion->start > (prevRegion->start + prevRegion->length)) &&
			((prevRegion->start + prevRegion->length) >= prevRegion->start));
}

//Creates a new blank memory region
MemRegion * MemRegionCreate(MemContext * context, void * startAddress,
		unsigned int length, MemRegionFlags flags)
{
	unsigned int startAddr = (unsigned int) startAddress;

	//Validate flags
	flags &= MEM_ALLFLAGS;

	//Not kernel context
	if(context == MemKernelContext)
	{
		PrintLog(Error, "MemRegionCreate: this function does not work with the kernel context");
		return NULL;
	}

	//Ensure start address and length are page aligned
	if(startAddr % 4096 != 0 || length % 4096 != 0)
	{
		PrintLog(Error, "MemRegionCreate: Region start address and length must be page aligned");
		return NULL;
	}

	if(startAddr + length < startAddr)
	{
		//Wrapped around
		PrintLog(Error, "MemRegionCreate: Region range wrapped around");
		return NULL;
	}

	//Validate addresses
	if((startAddr == 0 || startAddr + length >= 0xC0000000))
	{
		PrintLog(Error, "MemRegionCreate: Region outside valid range");
		return NULL;
	}

	//Allocate and initialize new region
	MemRegion * newRegion = MemKAlloc(sizeof(MemRegion));
	newRegion->flags = flags;
	newRegion->length = length;
	newRegion->start = startAddr;

	//Find place to insert region
	MemRegion * region = NULL;

	if(!ListEmpty(&context->regions))
	{
		ListForEachEntry(region, &context->regions, listItem)
		{
			//Is region beyond address?
			if(region->start > startAddr)
			{
				break;
			}
		}

		// region is now the region AFTER where we should insert

		//Check previous region overlap
		bool prevOverlap = false;
		ListHead * prev = region->listItem.prev;

		if(prev != &context->regions)
		{
			//Check overlap
			MemRegion * prevRegion = ListEntry(prev, MemRegion, listItem);

			prevOverlap = MemRegionIsCollision(prevRegion, newRegion);
		}

		//Ensure new region doesn't overlap
		if(prevOverlap || MemRegionIsCollision(newRegion, region))
		{
			PrintLog(Error, "MemRegionCreate: Region overlaps with another region");

			MemKFree(newRegion);
			return NULL;
		}
	}

	//Validation complete!
	// Actually allocate region
	newRegion->myContext = context;
	ListHeadInit(&newRegion->listItem);

	if(region == NULL)
	{
		ListHeadAddLast(&newRegion->listItem, &context->regions);
	}
	else
	{
		ListHeadAddLast(&newRegion->listItem, &region->listItem);
	}

	// We do no mapping until a page fault
	return newRegion;
}

//Resizes the region of allocated memory
void MemRegionResize(MemRegion * region, unsigned int newLength)
{
	MemContext * context = region->myContext;

	//Length must be page aligned
	if(newLength % 4096 != 0)
	{
		PrintLog(Error, "MemRegionResize: New region length must be page aligned");
		return;
	}

	//If size is reduced, free pages concerned
	if(newLength < region->length)
	{
		unsigned int start = region->start + newLength;
		unsigned int end = region->start + region->length;

		//Free pages in region
		// Fixed pages are also freed (ref count decrement)
		for(; start < end; start += 4096)
		{
			MemIntUnmapUserPageAndFree(context, (void *) start);
		}
	}
	else
	{
		//Disallow wrap around
		if(region->start + newLength < region->start)
		{
			//Error
			PrintLog(Error, "MemRegionResize: Region wraps around memory space");
			return;
		}

		//Do not enter kernel zone
		if(region->start + newLength > 0xC0000000)
		{
			//Error
			PrintLog(Error, "MemRegionResize: User mode region cannot be resized into kernel mode");
			return;
		}

		//Check for collision into next region
		if(region->listItem.next != &region->myContext->regions)
		{
			if(MemRegionIsCollision(region, ListEntry(region->listItem.next, MemRegion, listItem)))
			{
				//Error
				PrintLog(Error, "MemRegionResize: Region overlaps with another region");
				return;
			}
		}
	}

	//Set new length
	region->length = newLength;
}

//Deletes the specified region
void MemRegionDelete(MemRegion * region)
{
	//Resize the region to 0 length to free all the pages
	MemRegionResize(region, 0);

	//Remove file from region list
	ListDelete(&region->listItem);

	//Free region
	MemKFree(region);
}
