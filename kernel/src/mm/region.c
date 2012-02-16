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
#include "mm/misc.h"

#warning TODO Copy-On-Write Page tables

//Kernel page directory data
MemPageDirectory kernelPageDirectory[1024] __attribute__((aligned(4096)));
MemPageTable kernelPageTable254[1024] __attribute__((aligned(4096)));			//For physical reference table

//Kernel context
MemContext MemKernelContextData = { LIST_INLINE_INIT(MemKernelContextData.regions), 0, INVALID_PAGE, 0 };
	//INVALID_PAGE changed in MemManagerInit

//Current context
MemContext * MemCurrentContext = MemKernelContext;

//Check if a region will run into another
static bool MemRegionIsCollision(MemRegion * thisRegion, MemRegion * nextRegion);

//Creates a new blank memory context
MemContext * MemContextInit()
{
	//Allocate new context
	MemContext * newContext = MAlloc(sizeof(MemContext));
	ListHeadInit(&newContext->regions);

	//Allocate directory
	newContext->physDirectory = MemPhysicalAlloc(1);
	newContext->kernelVersion = 0;
	newContext->refCount = 0;

	//Temporarily map directory
	MemIntMapTmpPage(MEM_TEMPPAGE1, newContext->physDirectory);

	//Wipe user area
	MemPageDirectory * dir = (MemPageDirectory *) MEM_TEMPPAGE1;
	MemSet(dir, 0, sizeof(MemPageDirectory) * 768);

	//Copy kernel area
	MemCpy(dir + 0x300, kernelPageDirectory + 0x300, sizeof(MemPageDirectory) * 255);
	newContext->kernelVersion = MemKernelContext->kernelVersion;

	//Setup final page
	dir[1023].rawValue = 0;
	dir[1023].present = 1;
	dir[1023].writable = 1;
	dir[1023].pageID = newContext->physDirectory;
	
	//Unmap directory
	MemIntUnmapTmpPage(MEM_TEMPPAGE1);

	//Return context
	return newContext;
}

//Clones this memory context
MemContext * MemContextClone()
{
	//Allocate new context
	MemContext * newContext = MAlloc(sizeof(MemContext));
	ListHeadInit(&newContext->regions);

	//Copy regions
	MemRegion * oldRegion;
	ListForEachEntry(oldRegion, &MemCurrentContext->regions, listItem)
	{
		//Allocate new region
		MemRegion * newRegion = MAlloc(sizeof(MemRegion));

		//Copy region data
		MemCpy(newRegion, oldRegion, sizeof(MemRegion));

		//Set context
		newRegion->myContext = newContext;

		//Add to new list
		ListHeadAddLast(&newRegion->listItem, &newContext->regions);
	}

	//Allocate directory
	newContext->physDirectory = MemPhysicalAlloc(1);
	newContext->kernelVersion = 0;
	newContext->refCount = 0;

	//Temporarily map directory
	MemIntMapTmpPage(MEM_TEMPPAGE1, newContext->physDirectory);

	//Copy kernel area
	MemPageDirectory * dir = (MemPageDirectory *) MEM_TEMPPAGE1;
	MemCpy(dir + 0x300, kernelPageDirectory + 0x300, sizeof(MemPageDirectory) * 255);
	newContext->kernelVersion = MemKernelContext->kernelVersion;

	//Setup final page
	dir[1023].rawValue = 0;
	dir[1023].present = 1;
	dir[1023].writable = 1;
	dir[1023].pageID = newContext->physDirectory;

	//Copy all the page tables and increase refcounts on all pages
	for(int i = 0; i < 0x300; ++i)
	{
		MemPageDirectory * currDir = THIS_PAGE_DIRECTORY + i;

		//Copy directory entry
		dir[i] = *currDir;

		//If present, increase page ref counts first
		if(currDir->present)
		{
			MemPageTable * tableBase = THIS_PAGE_TABLES + (i * 1024);

			//Increase ref count on any pages
			for(int j = 0; j < 1024; ++j)
			{
				MemPageTable * table = tableBase + j;

				if(table->present)
				{
					//Increase count and make readonly
					MemPhysicalAddRef(table->pageID, 1);

					//Fixed pages are made writable again in the page fault handler
					table->writable = 0;
				}
			}

			//Duplicate page table
			MemPhysPage newTable = MemPhysicalAlloc(1);

			MemIntMapTmpPage(MEM_TEMPPAGE2, newTable);				//Page faults must not occur here
				MemCpy(MEM_TEMPPAGE2, tableBase, sizeof(MemPageTable) * 1024);
			MemIntUnmapTmpPage(MEM_TEMPPAGE2);

			//Store in directory
			dir[i].pageID = newTable;
		}
	}
	
	//Unmap directory
	MemIntUnmapTmpPage(MEM_TEMPPAGE1);

	//Flush paging caches
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

	//Check kernel version
	if(context->kernelVersion != MemKernelContext->kernelVersion)
	{
		//Update kernel page tables
		// The TLB should already contain the correct values so we SHOULN'T need to INVLPG them
		MemCpy(THIS_PAGE_DIRECTORY + 0x300, kernelPageDirectory + 0x300, sizeof(MemPageDirectory) * 255);

		context->kernelVersion = MemKernelContext->kernelVersion;
	}

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

	//Switch to context to delete pages
	CONTEXT_SWAP(context)
	{
		//Process tables
		for(int i = 0; i < 0x300; ++i)
		{
			MemPageDirectory * dir = THIS_PAGE_DIRECTORY + i;

			//If present, free pages in page table first
			if(dir->present)
			{
				MemPageTable * tableBase = THIS_PAGE_TABLES + (i * 1024);

				//Free pages
				for(int j = 0; j < 1024; ++j)
				{
					MemPageTable * table = tableBase + j;

					//Free page if present
					if(table->present)
					{
						MemPhysicalDeleteRef(table->pageID, 1);
					}
				}

				MemPhysicalFree(dir->pageID, 1);
			}
		}

		//Switch back to other directory
		// We can skip the kernel update since there won't be one
	}
	CONTEXT_SWAP_END

	//Free directory
	MemPhysicalFree(context->physDirectory, 1);

	//Free regions
	MemRegion * region, * tmpRegion;
	ListForEachEntrySafe(region, tmpRegion, &context->regions, listItem)
	{
		//Free region
		MFree(region);
	}

	//Free the final context
	MFree(context);
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
	//Check this op can be done on the region
	if(region->flags & MEM_FIXED)
	{
		PrintLog(Warning, "MemRegionFreePages: Cannot free pages from fixed page region");
		return;
	}

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
	CONTEXT_SWAP(context)
	{
		for(; startAddr < endAddr; startAddr += 4096)
		{
			MemIntUnmapPageAndFree((void *) startAddr);
		}
	}
	CONTEXT_SWAP_END
}

//Finds the region which contains the given address
MemRegion * MemRegionFind(MemContext * context, void * address)
{
	unsigned int addr = (unsigned int) address;

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
		unsigned int length, MemPhysPage firstPage, MemRegionFlags flags)
{
	MemRegion * region = MAlloc(sizeof(MemRegion));
	if(MemRegionCreateStatic(context, startAddress, length, flags, region))
	{
		//Map pages if fixed
		if(flags & MEM_FIXED)
		{
			region->firstPage = firstPage;

			//Increment ref counts of pages
			MemPhysicalAddRef(firstPage, length / PAGE_SIZE);

			//With fixed regions, we map the pages now
			CONTEXT_SWAP(context)
			{
				for(char * startAddr = (char *) startAddress; length > 0;
						length -= 4096, startAddr += 4096, ++firstPage)
				{
					//Map this page
					MemIntMapPage(startAddr, firstPage, flags);
				}
			}
			CONTEXT_SWAP_END
		}

		return region;
	}
	else
	{
		MFree(region);
		return NULL;
	}
}

//Create region from already allocated MemRegion
bool MemRegionCreateStatic(MemContext * context, void * startAddress,
		unsigned int length, MemRegionFlags flags, MemRegion * newRegion)
{
	unsigned int startAddr = (unsigned int) startAddress;

	//Validate flags
	flags &= MEM_ALLFLAGS;

	//Ensure start address and length are page aligned
	if(startAddr % 4096 != 0 || length % 4096 != 0)
	{
		PrintLog(Error, "MemRegionCreate: Region start address and length must be page aligned");
		return false;
	}

	if(startAddr + length < startAddr)
	{
		//Wrapped around
		PrintLog(Error, "MemRegionCreate: Region range wrapped around");
		return false;
	}

	//Validate addresses
	if((context == MemKernelContext && (startAddr < 0xC0000000 || startAddr + length >= (unsigned int) MEM_TEMPPAGE1)) ||
		(context != MemKernelContext && (startAddr == 0 || startAddr + length >= 0xC0000000)))
	{
		PrintLog(Error, "MemRegionCreate: Region outside valid range");
		return false;
	}

	//Enter data required for collision checking
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
			return false;
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
	return true;
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
		CONTEXT_SWAP(context)
		{
			for(; start < end; start += 4096)
			{
				MemIntUnmapPageAndFree((void *) start);
			}
		}
		CONTEXT_SWAP_END
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

		//Disallow if it enters restricted area
		if(region->start < 0xC0000000)
		{
			//Do not enter kernel zone
			if(region->start + newLength > 0xC0000000)
			{
				//Error
				PrintLog(Error, "MemRegionResize: User mode region cannot be resized into kernel mode");
				return;
			}
		}
		else
		{
			//Do not enter memory manager zone
			if(region->start + newLength > (unsigned int) MEM_TEMPPAGE1)
			{
				//Error
				PrintLog(Error, "MemRegionResize: Region cannot be resized into memory manager area");
				return;
			}
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
	MFree(region);
}

#warning TODO Memory Committing Function
