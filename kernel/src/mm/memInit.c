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

MemPageStatus * MemPageStateTableEnd;

//Kernel end symbol
extern char _kernel_end_page[];

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
		unsigned int * highestAddr, unsigned int * tableLength, MemPhysPage * tablePage)
{
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
					*highestAddr = 0xFFFFFFFF;
				}
				else
				{
					//Update only if this is higher
					if((unsigned int) (mmapEntry->addr + mmapEntry->len) > *highestAddr)
					{
						*highestAddr = (unsigned int) (mmapEntry->addr + mmapEntry->len);
					}
				}
			}
		}
	}

	//Get space required
	*tableLength = (*highestAddr / 4096) * sizeof(MemPageStatus);

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
	unsigned int tableLength;
	unsigned int highestAddr;

	//Set kernel context page directory location
	MemKernelContext->physDirectory =
			(((unsigned int) &kernelPageDirectory) - ((unsigned int) KERNEL_VIRTUAL_BASE)) / 4096;

	//PHASE 1 - Get table location
	GetPhysicalTableLocation(bootInfo, &highestAddr, &tableLength, &tableLocation);

	//PHASE 2 - Setup page tables
	// Self mapping top page
	kernelPageDirectory[1023].present = 1;
	kernelPageDirectory[1023].writable = 1;
	kernelPageDirectory[1023].pageID =
			((unsigned int) kernelPageDirectory - (unsigned int) KERNEL_VIRTUAL_BASE) / 4096;

	// Physical page tables
	kernelPageDirectory[1022].present = 1;
	kernelPageDirectory[1022].writable = 1;
	kernelPageDirectory[1022].global = 1;
	kernelPageDirectory[1022].pageID =
			((unsigned int) kernelPageTable254 - (unsigned int) KERNEL_VIRTUAL_BASE) / 4096;

	// Temp page tables while we're at it
	kernelPageDirectory[1021].present = 1;
	kernelPageDirectory[1021].writable = 1;
	kernelPageDirectory[1021].global = 1;
	kernelPageDirectory[1021].pageID =
			((unsigned int) kernelPageTable253 - (unsigned int) KERNEL_VIRTUAL_BASE) / 4096;

	// Map physical page tables to memory
	unsigned int tableLengthPages = (tableLength + 4095) / 4096;
	for(unsigned int i = 0; i < tableLengthPages; ++i)
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
	MemPhysicalTotalPages -= (0x100 - 0xA0);
	for(MemPhysPage page = 0xA0; page < ((int) _kernel_end_page); ++page)
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
			MemPhysPage startPages = module->mod_start / 4096;
			MemPhysPage endPages = (module->mod_end + 4095) / 4096;

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