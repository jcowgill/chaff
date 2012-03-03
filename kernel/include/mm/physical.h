/**
 * @file
 * Physical memory manager
 *
 * Generally you don't need to access these directly, use regions instead.
 *
 * @date February 2012
 * @author James Cowgill
 * @ingroup Mem
 */

/*
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
 */

#ifndef MM_PHYSICAL_H_
#define MM_PHYSICAL_H_

#include "chaff.h"

/**
 * Type used for physical page identifiers
 */
typedef int MemPhysPage;

/**
 * Page returned on error
 */
#define INVALID_PAGE (-1)

/**
 * Page size
 */
#define PAGE_SIZE 4096

/**
 * Final memory address not identity mapped into kernel space
 */
#define MEM_KFIXED_MAX 0xF0000000

/**
 * The page after the last page identity mapped into kernel space
 */
#define MEM_KFIXED_MAX_PAGE ((MEM_KFIXED_MAX - (unsigned int) KERNEL_VIRTUAL_BASE) / PAGE_SIZE)

/**
 * Gets the address of a physical page mapped directly into kernel space
 *
 * This can only be used for pages allocated in the #MEM_DMA and #MEM_KERNEL zones (not #MEM_HIGHMEM).
 *
 * @param page page number to convert
 * @return kernel mode pointer to page
 */
static inline void * MemPhys2Virt(MemPhysPage page)
{
	return ((void *) ((unsigned int) KERNEL_VIRTUAL_BASE + (page) * PAGE_SIZE));
}

/**
 * Gets the physical page from an address mapped directly into kernel space
 *
 * This can only be used for pages allocated in the #MEM_DMA and #MEM_KERNEL zones (not #MEM_HIGHMEM).
 *
 * @param addr address to get page of
 * @return physical page number
 */
static inline MemPhysPage MemVirt2Phys(void * addr)
{
	return (addr - KERNEL_VIRTUAL_BASE) / PAGE_SIZE;
}

/**
 * @name Allocation Zones
 * @{
 */

#define MEM_DMA 0		///< Memory under 16MB which can be used for direct memory access
#define MEM_KERNEL 1	///< Memory mapped for use by the kernel (general structures)
#define MEM_HIGHMEM 2	///< Memory above 1GB which cannot directly be used by the kernel
						///< (usually used for user mode memory)

/** @} */

/**
 * Initializes the physical memory manager zones
 *
 * @private
 */
void INIT MemPhysicalInit();

/**
 * Allocates normal physical pages
 *
 * This function allocates the specified number of physical pages in contiguous physical space.
 *
 * The pages returned will have a reference count of 1.
 *
 * Zones "contain" all the zones that are below them. This means, requesting memory
 * from #MEM_HIGHMEM could return memory from the #MEM_DMA zone (but not vice-versa).
 *
 * @param number number of pages to allocate
 * @param zone zone to allocate memory from - one of #MEM_DMA, #MEM_KERNEL or #MEM_HIGHMEM
 * @return the first page in the allocated series
 * @bug panics when out of memory
 */
MemPhysPage MemPhysicalAlloc(unsigned int number, int zone);

/**
 * Adds a reference to the given page(s)
 *
 * @param page first page to add reference to
 * @param number number of contiguous pages to add reference to
 */
void MemPhysicalAddRef(MemPhysPage page, unsigned int number);

/**
 * Deletes a reference to the given page(s)
 *
 * @param page first page to delete reference of
 * @param number number of contiguous pages to delete reference of
 */
void MemPhysicalDeleteRef(MemPhysPage page, unsigned int number);

/**
 * Frees physical pages allocated by MemPhysicalAlloc() and MemPhysicalAllocISA()
 *
 * This sets the reference count of the pages to 0
 *
 * @param page first page in series to free
 * @param number number of contiguous pages to free
 */
void MemPhysicalFree(MemPhysPage page, unsigned int number);

/**
 * Total usable number of pages in RAM
 *
 * This is for statistical purposes only. It DOES NOT say where the highest avaliable page is.
 */
extern unsigned int MemPhysicalTotalPages;

/**
 * Number of free pages in RAM
 */
extern unsigned int MemPhysicalFreePages;

/**
 * Memory page status structure
 *
 * Contains page reference counts
 */
typedef struct
{
	/**
	 * The number of references to this page (contexts)
	 */
	unsigned int refCount;

} MemPage;

/**
 * Address of global page state table
 */
extern MemPage * MemPageStateTable;

/**
 * Address of the end of the page state table (after end)
 */
extern MemPage * MemPageStateTableEnd;

/**
 * Returns the number of references to a page
 *
 * @param page page to check
 * @return number of references to this page
 */
static inline unsigned int MemPhysicalRefCount(MemPhysPage page)
{
	return MemPageStateTable[page].refCount;
}

#endif
