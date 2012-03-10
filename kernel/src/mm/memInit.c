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
#include "loader/elf.h"

//Kernel page directory
MemPageDirectory MemKernelPageDirectory[1024] __attribute__((aligned(4096)));

//Page tables for virtual memory region (0xF0000000 and above)
MemPageTable MemVirtualPageTables[64 * 1024] __attribute__((aligned(4096)));

//Page status table variables
MemPage * MemPageStateTable;
MemPage * MemPageStateTableEnd;

//Kernel end symbols
extern char _kernel_end_page[];
extern char _kernel_init_start_page[];

//End of INIT region
static INIT MemPhysPage endOfInitRegion;

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
	((var) += (4096 - ((var) % 4096)))

//Very unlikely to generated except by us
#define INIT_REFCOUNT 0xDEADBEEF

//Returns maximum of 2 inputs
#define MAX(a,b) ((a) > (b) ? (a) : (b))

//Gets the location of the physical page references table and its length
static inline INIT void GetPhysicalTableLocation(multiboot_info_t * bootInfo,
		unsigned int * numPages, MemPhysPage * tablePage, bool withPageTables)
{
	unsigned int highestAddr;

	//Find highest memory location to get size of array
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
	unsigned int tableLength = *numPages * sizeof(MemPage);

	if(withPageTables)
	{
		//Add page data (4 bytes for each page entry) with alignment
		PAGE_ALIGN(tableLength);
		tableLength += *numPages * 4;
		PAGE_ALIGN(tableLength);
	}

	//Find lowest address for table
	unsigned int position = ((unsigned int) _kernel_end_page) * 4096;

	//Boot modules
	if(bootInfo->flags & MULTIBOOT_INFO_MODS)
	{
		MODULES_FOREACH(module, bootInfo->mods_addr, bootInfo->mods_count)
		{
			//Higher?
			position = MAX(position, module->mod_end);
		}
	}

	//ELF Symbols
	if(bootInfo->flags & MULTIBOOT_INFO_ELF_SHDR)
	{
		LdrElfSection * section = (LdrElfSection *) (bootInfo->u.elf_sec.addr + KERNEL_VIRTUAL_BASE);

		for(unsigned int i = 0; i < bootInfo->u.elf_sec.num; i++)
		{
			//Skip string table and symbol table sections
			if(section->type == LDR_ELF_SHT_STRTAB || section->type == LDR_ELF_SHT_SYMTAB)
			{
				//Higher?
				position = MAX(position, section->size + section->addr);
			}

			section = (LdrElfSection *) ((char *) section + bootInfo->u.elf_sec.size);
		}
	}

	//Mark end of init region
	endOfInitRegion = (position + 4095) / 4096;

	//Validate in memory map
	MMAP_FOREACH(mmapEntry, bootInfo->mmap_addr, bootInfo->mmap_length)
	{
		//Only check available regions
		if(mmapEntry->type == MULTIBOOT_MEMORY_AVAILABLE && (mmapEntry->addr >> 32) == 0)
		{
			unsigned long long endOfBlock = mmapEntry->addr + mmapEntry->len;

			//Advance position to start of block
			if(position < mmapEntry->addr)
			{
				position = mmapEntry->addr;
			}

			//Page align position
			PAGE_ALIGN(position);

			//Ensure block is large enough
			if(position >= mmapEntry->addr && position + tableLength < endOfBlock)
			{
				//OK, save this
				*tablePage = position / 4096;
				return;
			}
		}
	}

	//If we're here, a section hasn't been found
	Panic("Out of memory for physical memory table");
}

//Reserves an area of memory
// If permanent is false, MemFreeInitPages frees the memory again
static inline INIT void ReserveMemoryArea(MemPhysPage start, MemPhysPage end, bool permanent, bool decrementFree)
{
	//Reserve everything
	for(; start < end; ++start)
	{
		if(permanent)
		{
			MemPageStateTable[start].refCount = 1;
		}
		else
		{
			//Do not overwrite a permanent page
			if(MemPageStateTable[start].refCount == 0)
			{
				MemPageStateTable[start].refCount = INIT_REFCOUNT;

				if(decrementFree)
				{
					MemPhysicalFreePages--;
				}
			}
		}
	}
}

//Memory manager initialisation
void INIT MemManagerInit(multiboot_info_t * bootInfo)
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
			((MemPhysicalTotalPages * sizeof(MemPage) + 4095) / 4096);

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
	MemPageStateTable = (MemPage *) ((tableLocation * 4096) + 0xC0000000);
	MemPageStateTableEnd = &MemPageStateTable[MemPhysicalTotalPages];
	MemSet(MemPageStateTable, 0, MemPhysicalTotalPages * sizeof(MemPage));

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
			ReserveMemoryArea(startPages, endPages, true, false);
		}
	}

	// Allocate ROM and Kernel area
	//  (Total not updated for kernel area)
	ReserveMemoryArea(0xA0, (int) _kernel_end_page, true, false);

	//All other pages are initially free
	MemPhysicalFreePages = MemPhysicalTotalPages;

	// Allocate page state table and initial page tables
	ReserveMemoryArea(tableLocation, endOfAllocedTable, true, true);

	// Allocate the entire area up to endOfMultiboot as INIT pages
	ReserveMemoryArea(0, endOfInitRegion, false, true);

	//Setup physical manager zones
	MemPhysicalInit();
}

//Frees INIT pages
void INIT MemFreeInitPages()
{
	//Free INIT pages
	for(MemPhysPage i = 0; i < endOfInitRegion; i++)
	{
		if(MemPhysicalRefCount(i) == INIT_REFCOUNT)
		{
			MemPhysicalFree(i, 1);
		}
	}

	//Free all pages between kernel_init_start_page and kernel_end_page
	MemPhysicalFree((MemPhysPage) _kernel_init_start_page,
			(unsigned int) _kernel_end_page - (unsigned int) _kernel_init_start_page);
}
