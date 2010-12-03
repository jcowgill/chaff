/*
 * context.cpp
 *
 *  Created on: 14 Oct 2010
 *      Author: James
 */

#include "chaff.h"
#include "memmgr.h"
#include "inlineasm.h"

//Kernel page directory data
PageDirectory kernelPageDirectory[1024] __attribute__((aligned(4096)));
PageTable kernelPageTable254[1024] __attribute__((aligned(4096)));			//For physical reference table

//Kernel context
MemContext MemKernelContextData = { LIST_HEAD_INIT(MemKernelContextData.regions), 0, INVALID_PAGE };

//Temporary pages
// These should be unmapped while outside a region function
#define MEM_TEMPPAGE1 ((void *) 0xFF800000)
#define MEM_TEMPPAGE2 ((void *) 0xFF801000)

//Raw page mapping
// Maps pages and handles all creation and management of page tables
static void mapPage(void * address, PhysPage page, RegionFlags flags);
static void unmapPage(void * address);
static void unmapPageAndFree(void * address);

#warning TODO - must switch to new context in most functions temporarilly
#warning TODO - don't allow mapping in some areas
#warning TODO - kernel version updating in some functions

//Creates a new blank memory context
MemContext * MemContextInit()
{
	//Allocate new context
	MemContext * newContext = MAlloc(sizeof(MemContext));
	INIT_LIST_HEAD(&newContext->regions);

	//Allocate directory
	newContext->physDirectory = MemPhysicalAlloc(1);

	//Temporarily map directory
	mapPage(MEM_TEMPPAGE1, newContext->physDirectory, MEM_READABLE | MEM_WRITABLE);

	//Wipe user area
	PageDirectory * dir = (PageDirectory *) MEM_TEMPPAGE1;
	MemSet(dir, 0, sizeof(PageDirectory) * 768);

	//Copy kernel area
	MemCpy(dir + 768, kernelPageDirectory + 768, sizeof(PageDirectory) * 255);
	newContext->kernelVersion = MemKernelContext->kernelVersion;

	//Setup final page
	dir[1023].rawValue = 0;
	dir[1023].present = 1;
	dir[1023].writable = 1;
	dir[1023].pageID = newContext->physDirectory;
	
	//Unmap directory
	unmapPage(MEM_TEMPPAGE1);

	//Return context
	return newContext;
}

//Clones this memory context
MemContext * MemContextClone()
{
	//
#warning TODO
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
	setCR3(context->physDirectory);

	//Check kernel version
	if(context->kernelVersion != MemKernelContext->kernelVersion)
	{
		//Update kernel page tables
		// The TLB should already contain the correct values so we SHOULN'T need to INVLPG them
#warning Kernel pages must be GLOBAL for this to work
		MemCpy(0xFFFFFC00, kernelPageDirectory + 768, sizeof(PageDirectory) * 255);

		context->kernelVersion = MemKernelContext->kernelVersion;
	}
}

//Deletes this memory context - DO NOT delete the memory context
// which is currently in use!
void MemContextDelete(MemContext * context)
{
	unsigned int oldContext = getCR3();

	//Check current context
	if(context->physDirectory == oldContext / 4096)
	{
		PrintLog(Critical, "MemContextDelete: Cannot delete current memory context.");
		return;
	}
	else if(context->physDirectory == INVALID_PAGE)
	{
		PrintLog(Critical, "MemContextDelete: Invalid memory context passed.");
		return;
	}

	//Switch to context to delete pages
	MemContextSwitchTo(context);

	//Process tables
	for(int i = 0; i < 0x300; ++i)
	{
		PageDirectory * dir = ((PageDirectory *) 0xFFFFF000) + i;

		//If present, free pages in page table first
		if(dir->present)
		{
			PageTable * tableBase = ((PageTable *) 0xFFC00000) + (i * 1024);

			//Free pages
			for(int j = 0; j < 1024; ++j)
			{
				PageTable * table = tableBase + j;

				//Free page if present
				if(table->present)
				{
					MemPhysicalFree(table->pageID, 1);
				}
			}

			MemPhysicalFree(dir->pageID, 1);
		}
	}

	//Switch back to other directory
	// We can skip the kernel update since there won't be one
	setCR3(oldContext);

	//Free directory
	MemPhysicalFree(context->physDirectory, 1);

	//Free regions
	MemRegion * region;
	list_for_each_entry(region, &context->regions, listItem)
	{
		//Deallocate region stuffs
#warning TODO - Do we do something (like closing) the file handle here?

		//Free region
		MFree(region);
	}

	//Free the final context
	MFree(context);
}

//Frees the pages associated with a given region of memory without destroying the region
// (pages will automatically be allocated when memory is referenced)
void MemRegionFreePages(MemRegion * region, void * address, unsigned int length)
{
	//Check this op can be done on the region
#warning This allowed pages from memory mapped files to be freed (if changing, other functions need changing too)
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
	for(; startAddr < endAddr; startAddr += 4096)
	{
		unmapPageAndFree(startAddr);
	}
}

//Finds the region which contains the given address
// or returns NULL if there isn't one
MemRegion * MemRegionFind(MemContext * context, void * address)
{
#warning Assumes ORDERED list of regions

	unsigned int addr = (unsigned int) address;

	//Check each address
	MemRegion * region;
	list_for_each_entry(region, &context->regions, listItem)
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

//Creates a new blank memory region
// If the context given is not the current or kernel context,
//  a temporary memory context switch may occur
MemRegion * MemRegionCreate(MemContext * context, void * startAddress,
		unsigned int length, RegionFlags flags)
{
	unsigned int startAddr = (unsigned int) startAddress;

	//Validate flags
	flags &= MEM_ALLFLAGS;

	if((flags & MEM_MAPPEDFILE) && (flags & MEM_FIXED))
	{
		PrintLog(Error, "MemRegionCreate: Region cannot be both memory mapped and fixed");
		return NULL;
	}

	//Ensure start address and length are page aligned
	if(startAddr % 4096 != 0 || length % 4096 != 0)
	{
		PrintLog(Error, "MemRegionCreate: Region start address and length must be page aligned");
		return NULL;
	}

	//Find place to insert region
	MemRegion * region;
	list_for_each_entry(region, &context->regions, listItem)
	{
		//Is region beyond address?
		if(region->start > startAddr)
		{
			//Not found
			break;
		}
	}

	// region is now the region AFTER where we should insert

	//Check previous region overlap
	bool prevOverlap = false;
	struct list_head * prev = region->listItem.prev;

	if(prev != &context->regions)
	{
		//Check overlap
		MemRegion * prevRegion = list_entry(prev, MemRegion, listItem);

		prevOverlap = (startAddr >= prevRegion->start && startAddr < (prevRegion->start + prevRegion->length)) ||
				(prevRegion->start >= startAddr && prevRegion->start < (startAddr + length));
	}

	//Ensure new region doesn't overlap
	if(prevOverlap || (startAddr >= region->start && startAddr < (region->start + region->length)) ||
			(region->start >= startAddr && region->start < (startAddr + length)))
	{
		PrintLog(Error, "MemRegionCreate: Region start address and length must be page aligned");
		return NULL;
	}

	//Validation complete!
	// Actually allocate region
	MemRegion * newRegion = MAlloc(sizeof(MemRegion));

	newRegion->myContext = context;
	INIT_LIST_HEAD(&newRegion->listItem);

	list_add_tail(&newRegion->listItem, &region->listItem);

	newRegion->flags = flags;
	newRegion->length = length;
	newRegion->start = startAddr;

	// We do no mapping until a page fault
	return newRegion;
}

//Creates a new memory mapped region
// If the context given is not the current or kernel context,
//  a temporary memory context switch may occur
// MEM_MAPPEDFILE is implied by this
MemRegion * MemRegionCreateMMap(MemContext * context, void * startAddress,
		unsigned int length, FileHandle file,
		unsigned int fileOffset, unsigned int fileSize, RegionFlags flags)
{
	//Pass to MemRegionCreate and set params
	MemRegion * region = MemRegionCreate(context, startAddress, length, flags | MEM_MAPPEDFILE);

	if(region)
	{
		region->file = file;
		region->fileOffset = fileOffset;
		region->fileSize = fileSize;
	}

	return region;
}

//Creates a new fixed memory section
// These regions always point to a particular area of physical memory
// If the context given is not the current or kernel context,
//  a temporary memory context switch may occur
// MEM_FIXED is implied by this
MemRegion * MemRegionCreateFixed(MemContext * context, void * startAddress,
		unsigned int length, PhysPage firstPage, RegionFlags flags)
{
	//Pass to MemRegionCreate and set params
	MemRegion * region = MemRegionCreate(context, startAddress, length, flags | MEM_FIXED);

	if(region)
	{
		region->firstPage = firstPage;

		//With fixed regions, we map the pages now
		for(char * startAddr = (char *) startAddress; length > 0;
				length -= 4096, startAddr += 4096, ++firstPage)
		{
			//Map this page
			mapPage(startAddr, firstPage, flags);
		}
	}

	return region;
}

//Resizes the region of allocated memory
// If the context of the region given is not the current or kernel context,
//  a temporary memory context switch may occur
void MemRegionResize(MemRegion * region, unsigned int newLength)
{
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

		//If fixed region, we don't free the pages - we just unmap them
		if(region->flags & MEM_FIXED)
		{
			for(; start < end; start += 4096)
			{
				unmapPage((void *) start);
			}
		}
		else
		{
			for(; start < end; start += 4096)
			{
				unmapPageAndFree((void *) start);
			}
		}
	}

	//Set new length
	region->length = newLength;
}

//Deletes the specified region
// If the context of the region given is not the current or kernel context,
//  a temporary memory context switch may occur
void MemRegionDelete(MemRegion * region)
{
	//Resize the region to 0 length to free all the pages
	MemRegionResize(region, 0);

	//Close file handle if mapped
	if(region->flags & MEM_MAPPEDFILE)
	{
#warning TODO fclose file here
	}

	//Remove file from region list
	list_del(&region->listItem);

	//Free region
	MFree(region);
}
