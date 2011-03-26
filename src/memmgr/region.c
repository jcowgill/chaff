/*
 * region.c
 *
 *  Created on: 14 Oct 2010
 *      Author: James
 */

#include "chaff.h"
#include "memmgr.h"
#include "inlineasm.h"
#include "memmgrInt.h"
#include "process.h"

#warning TODO Copy-On-Write Page tables

//Kernel page directory data
PageDirectory kernelPageDirectory[1024] __attribute__((aligned(4096)));
PageTable kernelPageTable254[1024] __attribute__((aligned(4096)));			//For physical reference table

//Kernel context
MemContext MemKernelContextData = { LIST_HEAD_INIT(MemKernelContextData.regions), 0, INVALID_PAGE };

//Free a page OR decrease it's reference count if it is > 1
void MemIntFreePageOrDecRefs(PhysPage page)
{
	unsigned int * refCount = MemIntPhysicalRefCount(page);
	if(*refCount > 1)
	{
		//Decrease copy-on-write reference count
		--(*refCount);
	}
	else
	{
		MemPhysicalFree(page, 1);
	}
}

//Creates a new blank memory context
MemContext * MemContextInit()
{
	//Allocate new context
	MemContext * newContext = MAlloc(sizeof(MemContext));
	INIT_LIST_HEAD(&newContext->regions);

	//Allocate directory
	newContext->physDirectory = MemPhysicalAlloc(1);
	newContext->kernelVersion = 0;

	//Temporarily map directory
	MemIntMapTmpPage(MEM_TEMPPAGE1, newContext->physDirectory);

	//Wipe user area
	PageDirectory * dir = (PageDirectory *) MEM_TEMPPAGE1;
	MemSet(dir, 0, sizeof(PageDirectory) * 768);

	//Copy kernel area
	MemCpy(dir + 0x300, kernelPageDirectory + 0x300, sizeof(PageDirectory) * 255);
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
	INIT_LIST_HEAD(&newContext->regions);

	//Allocate directory
	newContext->physDirectory = MemPhysicalAlloc(1);
	newContext->kernelVersion = 0;

	//Temporarily map directory
	MemIntMapTmpPage(MEM_TEMPPAGE1, newContext->physDirectory);

	//Copy kernel area
	PageDirectory * dir = (PageDirectory *) MEM_TEMPPAGE1;
	MemCpy(dir + 0x300, kernelPageDirectory + 0x300, sizeof(PageDirectory) * 255);
	newContext->kernelVersion = MemKernelContext->kernelVersion;

	//Setup final page
	dir[1023].rawValue = 0;
	dir[1023].present = 1;
	dir[1023].writable = 1;
	dir[1023].pageID = newContext->physDirectory;

	//Copy all the page tables and increase refcounts on all pages
	for(int i = 0; i < 0x300; ++i)
	{
		PageDirectory * currDir = THIS_PAGE_DIRECTORY + i;

		//Copy directory entry
		dir[i] = *currDir;

		//If present, increase page ref counts first
		if(currDir->present)
		{
			PageTable * tableBase = THIS_PAGE_TABLES + (i * 1024);

			//Increase ref count on any pages
			for(int j = 0; j < 1024; ++j)
			{
				PageTable * table = tableBase + j;

				if(table->present)
				{
					//Increase count and make readonly
					++(*MemIntPhysicalRefCount(table->pageID));
					table->writable = 0;
				}
			}

			//Duplicate page table
			PhysPage newTable = MemPhysicalAlloc(1);

			MemIntMapTmpPage(MEM_TEMPPAGE2, newTable);				//Page faults must not occur here
				MemCpy(MEM_TEMPPAGE2, tableBase, sizeof(PageTable) * 1024);
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
		MemCpy(THIS_PAGE_DIRECTORY + 0x300, kernelPageDirectory + 0x300, sizeof(PageDirectory) * 255);

		context->kernelVersion = MemKernelContext->kernelVersion;
	}
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

	//Switch to context to delete pages
	CONTEXT_SWAP(context)
	{
		//Process tables
		for(int i = 0; i < 0x300; ++i)
		{
			PageDirectory * dir = THIS_PAGE_DIRECTORY + i;

			//If present, free pages in page table first
			if(dir->present)
			{
				PageTable * tableBase = THIS_PAGE_TABLES + (i * 1024);

				//Free pages
				for(int j = 0; j < 1024; ++j)
				{
					PageTable * table = tableBase + j;

					//Free page if present
					if(table->present)
					{
						MemIntFreePageOrDecRefs(table->pageID);
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
			MemIntUnmapPageAndFree(context, (void *) startAddr);
		}
	}
	CONTEXT_SWAP_END
}

//Finds the region which contains the given address
// or returns NULL if there isn't one
MemRegion * MemRegionFind(MemContext * context, void * address)
{
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
	MemRegion * region = MAlloc(sizeof(MemRegion));
	if(MemIntRegionCreate(context, startAddress, length, flags, region))
	{
		return region;
	}
	else
	{
		MFree(region);
		return NULL;
	}
}

//Create region from already allocated MemRegion
// See MemRegionCreate
bool MemIntRegionCreate(MemContext * context, void * startAddress,
		unsigned int length, RegionFlags flags, MemRegion * newRegion)
{
	unsigned int startAddr = (unsigned int) startAddress;

	//Validate flags
	flags &= MEM_ALLFLAGS;

	if((flags & MEM_MAPPEDFILE) && (flags & MEM_FIXED))
	{
		PrintLog(Error, "MemRegionCreate: Region cannot be both memory mapped and fixed");
		return false;
	}

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

	//Find place to insert region
	MemRegion * region = NULL;

	if(!list_empty(&context->regions))
	{
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
			PrintLog(Error, "MemRegionCreate: Region overlaps with another region");
			return false;
		}
	}

	//Validation complete!
	// Actually allocate region
	newRegion->myContext = context;
	INIT_LIST_HEAD(&newRegion->listItem);

	if(region == NULL)
	{
		list_add_tail(&newRegion->listItem, &context->regions);
	}
	else
	{
		list_add_tail(&newRegion->listItem, &region->listItem);
	}

	newRegion->flags = flags;
	newRegion->length = length;
	newRegion->start = startAddr;

	// We do no mapping until a page fault
	return true;
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
		CONTEXT_SWAP(context)
		{
			for(char * startAddr = (char *) startAddress; length > 0;
					length -= 4096, startAddr += 4096, ++firstPage)
			{
				//Map this page
				MemIntMapPage(context, startAddr, firstPage, flags);
			}
		}
		CONTEXT_SWAP_END
	}

	return region;
}

//Resizes the region of allocated memory
// If the context of the region given is not the current or kernel context,
//  a temporary memory context switch may occur
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

		//If fixed region, we don't free the pages - we just unmap them
		CONTEXT_SWAP(context)
		{
			if(region->flags & MEM_FIXED)
			{
				for(; start < end; start += 4096)
				{
					MemIntUnmapPage(context, (void *) start);
				}
			}
			else
			{
				for(; start < end; start += 4096)
				{
					MemIntUnmapPageAndFree(context, (void *) start);
				}
			}
		}
		CONTEXT_SWAP_END
	}
	else
	{
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

//Performs the user mode memory checks on the current memory context
static bool MemUserChecks(unsigned int addr, unsigned int length, unsigned int flagsReqd)
{
	//Cut off kernel mode
	if(addr >= 0xC0000000)
	{
		return false;
	}

	//Find first region
	MemContext * context = ProcCurrProcess->memContext;
	MemRegion * region = MemRegionFind(context, (void *) addr);

	while(region != NULL && (region->flags & flagsReqd) == flagsReqd)
	{
		//Calculate data left
		unsigned int end = region->start + region->length;
		unsigned int lengthLeft = end - addr;

		if(lengthLeft >= length)
		{
			//Passed (since we known this region is readable)
			return true;
		}

		//Move to next region if it is continuous
		if(region->listItem.next == &context->regions)
		{
			//This is the last region, fail
			return false;
		}

		region = list_entry(region->listItem.next, MemRegion, listItem);

		if (region->start == end)
		{
			//Not continuous, fail
			return false;
		}

		//Update parameters
		addr = region->start;
		length -= lengthLeft;
	}

	return false;
}

//Returns true if user mode can read data from a specific location and length
// Reads may cause page faults
bool MemUserCanRead(void * data, unsigned int length)
{
	return MemUserChecks((unsigned int) data, length, MEM_READABLE);
}

//Returns true if user mode can write data to a specific location and length
// Writes may cause page faults
// Note: being able to write DOES NOT IMPLY being able to read
bool MemUserCanWrite(void * data, unsigned int length)
{
	return MemUserChecks((unsigned int) data, length, MEM_WRITABLE);
}

//Returns true if user mode can read and write data to a specific location and length
// Writes may cause page faults
bool MemUserCanReadWrite(void * data, unsigned int length)
{
	return MemUserChecks((unsigned int) data, length, MEM_READABLE | MEM_WRITABLE);
}
