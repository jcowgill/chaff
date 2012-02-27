/*
 * memInit.c
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
 *  Created on: 24 Oct 2010
 *      Author: James
 */

#include "chaff.h"
#include "multiboot.h"
#include "mm/physical.h"
#include "mm/region.h"
#include "mm/pagingInt.h"
#include "cpu.h"
#include "inlineasm.h"

//Kernel page directory
MemPageDirectory MemKernelPageDirectory[1024] __attribute__((aligned(4096)));

//Page tables for virtual memory region (0xF0000000 and above)
MemPageTable MemVirtualPageTables[64 * 1024] __attribute__((aligned(4096)));

//Page status table variables
MemPageStatus * MemPageStateTable;
MemPageStatus * MemPageStateTableEnd;

//Kernel end symbol
extern char _kernel_end_page[];

//Performs a bounds check
// We check if the start of either ranges is within the other
#define BOUNDS_CHECK(start, end) \
	do \
	{ \
		if(((position >= (start)) && (position < (end))) || \
			(((start) > position) && ((start) < endPosition))) \
		{ \
			position = (end); \
			goto boundsCheckLoop; \
		} \
	} while(0)

//Alignes given variable to the next page boundary
#define PAGE_ALIGN(var) \
	((var) += ((4096 - ((tableLength) % 4096)) % 4096))

//Gets the location of the physical page references table and its length
static inline void GetPhysicalTableLocation(multiboot_info_t * bootInfo,
		unsigned int * numPages, MemPhysPage * tablePage, bool withPageTables)
{
	unsigned int highestAddr;

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

	//Store number of pages
	*numPages = (highestAddr / 4096);

	//Calculate length of status table
	unsigned int tableLength = *numPages * sizeof(MemPageStatus);

	if(withPageTables)
	{
		//Add page data (4 bytes for each page entry) with alignment
		PAGE_ALIGN(tableLength);
		tableLength += *numPages * 4;
		PAGE_ALIGN(tableLength);
	}

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
			while(position < endPosition && (endPosition - position >= tableLength))
			{
				//Is block invading any of these zones?
				//The end position of zones should be page aligned

#warning Bit of a hack for handling memory manager position
				// HACK: Reserved entire lower region since this is usually
				//  where the multiboot record is placed (this also reserves ROM area)
				BOUNDS_CHECK(1, 0x100000);

				// Kernel area (0x100000 - kernel end page * 4096)
				BOUNDS_CHECK(0x100000, ((unsigned int) _kernel_end_page) * 4096);

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
	MemPhysPage tableLocation;

	//Set kernel context page directory location
	MemKernelContext->physDirectory =
			(((unsigned int) &MemKernelPageDirectory) - ((unsigned int) KERNEL_VIRTUAL_BASE)) / 4096;

	//Determine if 4MB pages are available
	bool using4MBPages = CpuFeaturesEDX & (1 << 3);

	//PHASE 1 - Get table location
	GetPhysicalTableLocation(bootInfo, &MemPhysicalTotalPages, &tableLocation, !using4MBPages);

	//PHASE 2 - Setup page tables for all kernel memory
	// Setup page tables for virtual region
	for(int i = 0x3C0; i < 0x400; i++)
	{
		//Only the directory is setup here
		MemKernelPageDirectory[i].rawValue = 0;
		MemKernelPageDirectory[i].present = 1;
		MemKernelPageDirectory[i].writable = 1;
		MemKernelPageDirectory[i].pageID =
				((void *) &MemVirtualPageTables[i - 0x3C0] - KERNEL_VIRTUAL_BASE) / 4096;
	}

	// Get page directory entries
	//  Number of 4MB Pages to allocate (rounded up)
	unsigned int num4MBPages = (MemPhysicalTotalPages + 1023) / 1024;

	//  End of pre-allocated table (marked as allocated later)
	MemPhysPage endOfAllocedTable = tableLocation +
			((MemPhysicalTotalPages * sizeof(MemPageStatus) + 4095) / 4096);

	//Limit number of pages
	if(num4MBPages > 0x3C0)
	{
		num4MBPages = 0x3C0;
	}

	// Determine of 4MB pages are available
	if(using4MBPages)
	{
		//Map physical memory using 4MB pages
		for(unsigned int i = 0x300; i < (num4MBPages + 0x300); i++)
		{
			MemKernelPageDirectory[i].rawValue = 0;
			MemKernelPageDirectory[i].present = 1;
			MemKernelPageDirectory[i].writable = 1;
			MemKernelPageDirectory[i].hugePage = 1;
			MemKernelPageDirectory[i].global = 1;
			MemKernelPageDirectory[i].pageID = (i - 0x300) * 0x400;
		}
	}
	else
	{
		//Page tables were allocated after the end of the page status table
		MemPhysPage firstPTable = endOfAllocedTable;
		endOfAllocedTable += (MemPhysicalTotalPages * 4 + 4095) / 4096;

		//Setup first virtual page
		MemVirtualPageTables[0].rawValue = 0;
		MemVirtualPageTables[0].present = 1;
		MemVirtualPageTables[0].writable = 1;

		//Map physical memory
		for(unsigned int i = 0x300; i < (num4MBPages + 0x300); i++)
		{
			//Map table to start of virtual memory
			MemVirtualPageTables[0].pageID = firstPTable;
			invlpg((void *) MEM_KFIXED_MAX);

			//Fill table
			for(unsigned int j = 0; j < 1024; j++)
			{
				((MemPageTable *) MEM_KFIXED_MAX)[j].rawValue = 0;
				((MemPageTable *) MEM_KFIXED_MAX)[j].present = 1;
				((MemPageTable *) MEM_KFIXED_MAX)[j].writable = 1;
				((MemPageTable *) MEM_KFIXED_MAX)[j].global = 1;
				((MemPageTable *) MEM_KFIXED_MAX)[j].pageID = ((i - 0x300) * 0x400) + j;
			}

			//Setup directory entry
			MemKernelPageDirectory[i].rawValue = 0;
			MemKernelPageDirectory[i].present = 1;
			MemKernelPageDirectory[i].writable = 1;
			MemKernelPageDirectory[i].pageID = firstPTable;
		}

		//Clear first virtual page
		MemVirtualPageTables[0].rawValue = 0;
		invlpg((void *) MEM_KFIXED_MAX);
	}

	//PHASE 3 - Fill memory table
	// Store table pointer and free everything
	MemPageStateTable = (MemPageStatus *) ((tableLocation * 4096) + 0xC0000000);
	MemPageStateTableEnd = &MemPageStateTable[MemPhysicalTotalPages];
	MemSet(MemPageStateTable, 0, MemPhysicalTotalPages * sizeof(MemPageStatus));

	// Allocate reserved areas of the memory map
	unsigned int highestAddr = MemPhysicalTotalPages * 4096;
	MMAP_FOREACH(mmapEntry, bootInfo->mmap_addr, bootInfo->mmap_length)
	{
		//Allocate if correct type
		if(mmapEntry->type != MULTIBOOT_MEMORY_AVAILABLE && mmapEntry->addr < highestAddr
				&& (mmapEntry->addr >> 32) == 0)
		{
			//Allocate
			MemPhysPage startPages = mmapEntry->addr / 4096;
			MemPhysPage endPages = (mmapEntry->len + mmapEntry->addr + 4095) / 4096;

			// Update total
			MemPhysicalTotalPages -= (endPages - startPages);

			// Set ref counts
			for(; startPages < endPages; ++startPages)
			{
				MemPageStateTable[startPages].refCount = 1;
			}
		}
	}

	// Allocate ROM and Kernel area
	//  (Total not updated for kernel area)
	for(MemPhysPage page = 0xA0; page < ((int) _kernel_end_page); ++page)
	{
		MemPageStateTable[page].refCount = 1;
	}

	//All other pages are initially free
	MemPhysicalFreePages = MemPhysicalTotalPages;

	// Allocate boot module area
	//  (Total not updated for modules area)
	if(bootInfo->flags & MULTIBOOT_INFO_MODS)
	{
		MODULES_FOREACH(module, bootInfo->mods_addr, bootInfo->mods_count)
		{
			//Allocate
			MemPhysPage startPages = module->mod_start / 4096;
			MemPhysPage endPages = (module->mod_end + 4095) / 4096;

			for(; startPages < endPages; ++startPages)
			{
				if(MemPageStateTable[startPages].refCount == 0)
				{
					MemPageStateTable[startPages].refCount = 1;
					MemPhysicalFreePages--;
				}
			}
		}
	}

	// Allocate page state table and initial page tables
	for(MemPhysPage page = tableLocation; page < endOfAllocedTable; ++page)
	{
		if(MemPageStateTable[page].refCount == 0)
		{
			MemPageStateTable[page].refCount = 1;
			MemPhysicalFreePages--;
		}
	}

	//Setup physical manager zones
	MemPhysicalInit();
}
