/*
 * memInit.c
 *
 *  Created on: 24 Oct 2010
 *      Author: James
 */

#include "chaff.h"
#include "multiboot.h"
#include "memmgr.h"

extern PageDirectory kernelPageDirectory[1024];
extern PageTable kernelPageTable254[1024];
MemPageStatus * MemPageStateTableEnd;

//Kernel end symbol
DECLARE_SYMBOL(_kernel_end_page);

//Performs a bounds check
// We check if the start of either ranges is within the other
#define BOUNDS_CHECK(start, end) \
	do \
	{ \
		if(((position > (start)) && (position < (end))) || \
			(((start) > position) && ((start) < endPosition))) \
		{ \
			position = (end); \
			goto boundsCheckLoop; \
		} \
	} while(0)

//Gets the location of the physical page references table and its length
static inline void GetPhysicalTableLocation(multiboot_info_t * bootInfo,
		unsigned int * tableLength, PhysPage * tablePage)
{
	unsigned int highestAddr = 0;

	//1st Pass - find highest memory location to get size of array
	MMAP_FOREACH(mmapEntry, bootInfo->mmap_addr, bootInfo->mmap_length)
	{
	    //Only map available regions
	    if(mmapEntry->type == MULTIBOOT_MEMORY_AVAILABLE)
	    {
	        //Ignore regions starting in 64 bit
	        // + if region ends in 64 bit - use full highAddr
	        if((mmapEntry->addr >> 32) != 0)
	        {
	            continue;
			}
			else
			{
			    //Regions which end in 64 bits raise the address to the maximum
				if(mmapEntry->addr + mmapEntry->len >= 0xFFFFFFFF)
				{
					highestAddr = 0xFFFFFFFF;
				}
				else
				{
					//Update only if this is higher
					if((unsigned int) (mmapEntry->addr + mmapEntry->len) > highestAddr)
					{
						highestAddr = (unsigned int) (mmapEntry->addr + mmapEntry->len);
					}
				}
			}
		}
	}

	//Get space required
	*tableLength = (highestAddr / 4096) * sizeof(MemPageStatus);

	//2nd pass - find space for it
	MMAP_FOREACH(mmapEntry, bootInfo->mmap_addr, bootInfo->mmap_length)
	{
		//Only check available regions
		if(mmapEntry->type == MULTIBOOT_MEMORY_AVAILABLE && (mmapEntry->addr >> 32) == 0)
		{
			//Get a position and check that it works
			unsigned int position = (mmapEntry->addr + 4095) & ~0xFFF;
			unsigned int endPosition = position + (mmapEntry->len - (mmapEntry->addr - position));

			//Enter checking loop
			// Ensure block is large enough
		boundsCheckLoop:
			while(position < endPosition && (endPosition - position >= *tableLength))
			{
				//Is block invading any of these zones?
				//The end position of zones should be page aligned
				// ROM area (0xA0000 - 0x100000)
				BOUNDS_CHECK(0xA0000, 0x100000);

				// Kernel area (0x100000 - kernel end page * 4096)
				BOUNDS_CHECK(0x100000, GET_SYMBOL_UINT(_kernel_end_page) * 4096);

				//Boot modules
				if(bootInfo->flags & MULTIBOOT_INFO_MODS)
				{
					MODULES_FOREACH(module, bootInfo->mods_addr, bootInfo->mods_count)
					{
						//Check module
						unsigned int end = (module->mod_end + 4095) & ~0xFFF;
						BOUNDS_CHECK(module->mod_start, end);
					}
				}

				//OK - if we get here, we've found a valid place!
				*tablePage = position / 4096;
				return;
			}
		}
	}

	//If we're here, a section hasn't been found
	Panic("Out of memory for physical memory table");
}

//Memory manager initialisation
void MemManagerInit(multiboot_info_t * bootInfo)
{
	PhysPage tableLocation;
	unsigned int tableLength;

	//PHASE 1 - Get table location
	GetPhysicalTableLocation(bootInfo, &tableLength, &tableLocation);

	//PHASE 2 - Setup page tables for the physical table
	kernelPageDirectory[1022].present = 1;
	kernelPageDirectory[1022].writable = 1;
	kernelPageDirectory[1022].global = 1;
	kernelPageDirectory[1022].pageID =
			((unsigned int) kernelPageTable254 - (unsigned int) KERNEL_VIRTUAL_BASE) / 4096;

	for(unsigned int i = 0; i < tableLength; ++i)
	{
		kernelPageTable254[i].present = 1;
		kernelPageTable254[i].writable = 1;
		kernelPageTable254[i].global = 1;
		kernelPageTable254[i].pageID = tableLocation + i;
	}

	//PHASE 3 - Fill memory table
	// Free everything first
	MemPhysicalTotalPages = tableLength / sizeof(MemPageStatus);
	MemSet(MemPageStateTable, 0, tableLength);

	// Allocate reserved areas of the memory map
	MMAP_FOREACH(mmapEntry, bootInfo->mmap_addr, bootInfo->mmap_length)
	{
		//Allocate if correct type
		if(mmapEntry->type != MULTIBOOT_MEMORY_AVAILABLE && (mmapEntry->addr >> 32) == 0)
		{
			//Allocate
			PhysPage startPages = mmapEntry->addr / 4096;
			unsigned int lengthPages =
					(mmapEntry->len + (mmapEntry->addr - (mmapEntry->addr / 4096)) + 4095) / 4096;

			// Update total
			MemPhysicalTotalPages -= lengthPages;

			// Set ref counts
			for(; lengthPages > 0; ++startPages, --lengthPages)
			{
				MemPageStateTable[startPages].refCount = 1;
			}
		}
	}

	// Allocate ROM and Kernel area
	//  (Total not updated for kernel area)
	MemPhysicalTotalPages -= (0x100 - 0xA0);
	for(PhysPage page = 0xA0; page < (int) GET_SYMBOL_UINT(_kernel_end_page); ++page)
	{
		MemPageStateTable[page].refCount = 1;
	}

	// Allocate boot module area
	//  (Total not updated for modules area)
	if(bootInfo->flags & MULTIBOOT_INFO_MODS)
	{
		MODULES_FOREACH(module, bootInfo->mods_addr, bootInfo->mods_count)
		{
			//Allocate
			PhysPage startPages = module->mod_start / 4096;
			PhysPage endPages = (module->mod_end + 4095) / 4096;

			for(; startPages < endPages; ++startPages)
			{
				MemPageStateTable[startPages].refCount = 1;
			}
		}
	}

	//Set page state table end
	MemPageStateTableEnd = (MemPageStatus *) (((unsigned int) MemPageStateTable) + tableLength);

	//Done
}
