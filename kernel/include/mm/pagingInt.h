/**
 * @file
 * Internal paging functions
 *
 * @date February 2012
 * @author James Cowgill
 * @privatesection
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

#ifndef MM_PAGINGINT_H_
#define MM_PAGINGINT_H_

#include "chaff.h"
#include "mm/region.h"
#include "mm/physical.h"

/**
 * x86 Page Directory Structure
 */
typedef union
{
	///Gets the raw value of the page directory
	unsigned int rawValue;

	struct
	{
		///1 if page is in use
		unsigned int present		: 1;

		///1 if page can be written to (otherwise writing causes a page fault)
		unsigned int writable		: 1;

		///1 if page is available from user mode
		unsigned int userMode		: 1;

		///1 if all writes should bypass the cache
		unsigned int writeThrough	: 1;

		///1 if all reads should bypass the cache
		unsigned int cacheDisable	: 1;

		///1 if the page has been accessed since this was last set to 0
		unsigned int accessed		: 1;

		///1 if the page has been written to since this was last set to 0
		/// Ignored if this is a page table entry
		unsigned int dirty			: 1;

		///1 if this entry references a 4MB page rather than a page table
		unsigned int hugePage		: 1;

		/**
		 * 1 if this page should not be invalidated when changing CR3
		 *
		 * Must enable this feature in a CR4 bit first.
		 * Ignored if this is a page table entry
		 */
		unsigned int global			: 1;

		unsigned int /* can be used by OS */	: 3;

		///The page frame ID of the 4MB page or page table
		/// If this is a 4MB page, it MUST be aligned to 4MB (or we'll crash)
		MemPhysPage pageID			: 20;
	};

} MemPageDirectory;

/**
 * x86 Page Table Structure
 */
typedef union
{
	///Gets the raw value of the page table
	unsigned int rawValue;

	struct
	{
		///1 if page is in use
		unsigned int present		: 1;

		///1 if page can be written to (otherwise writing causes a page fault)
		unsigned int writable		: 1;

		///1 if page is available from user mode
		unsigned int userMode		: 1;

		///1 if all writes should bypass the cache
		unsigned int writeThrough	: 1;

		///1 if all reads should bypass the cache
		unsigned int cacheDisable	: 1;

		///1 if the page has been accessed since this was last set to 0
		unsigned int accessed		: 1;

		///1 if the page has been written to since this was last set to 0
		unsigned int dirty			: 1;
		unsigned int /* reserved */	: 1;

		///1 if this page should not be invalidated when changing CR3
		/// Must enable this feature in a CR4 bit first
		unsigned int global			: 1;

		/**
		 * Not used by CPU
		 *
		 * 3 bits of table counter to count number of pages allocated in page table.
		 * @a tableCount is used only in the first 5 page table entries
		 */
		unsigned int tableCount		: 3;

		///The page frame ID of the 4KB page
		MemPhysPage pageID			: 20;
	};

} MemPageTable;

/**
 * Gets a pointer to the page directory for the given address
 *
 * @param context memory context to use
 * @param addr address to lookup
 */
static inline MemPageDirectory * MemGetPageDirectory(MemContext * context, unsigned int addr)
{
	return &((MemPageDirectory *) MemPageAddr(context->physDirectory))[addr >> 22];
}

/**
 * Gets a pointer to the page table entry for the given address
 *
 * @param dir directory entry (from MemGetPageDirectory())
 * @param addr address to lookup
 */
static inline MemPageTable * MemGetPageTable(MemPageDirectory * dir, unsigned int addr)
{
	return &((MemPageTable *) MemPageAddr(dir->pageID))[(addr >> 12) & 0xFFF];
}

/**
 * Kernel page directory
 */
extern MemPageDirectory MemKernelPageDirectory[1024];

/**
 * Page tables for virtual region of kernel memory (>= 0xF0000000)
 */
extern MemPageTable MemVirtualPageTables[64 * 1024];

/**
 * Maps a user mode page to a particular address
 *
 * @param context memory context to map in
 * @param address address of page to map (must be page aligned)
 * @param page physical page to map
 * @param flags flags to assign to page (must be same as region flags)
 */
void MemIntMapUserPage(MemContext * context, void * address, MemPhysPage page, MemRegionFlags flags);

/**
 * Unmaps a user mode page
 *
 * @param context memory context to unmap from
 * @param address address of page to unmap (must be page aligned)
 * @return the page which was unmapped (can be #INVALID_PAGE)
 */
MemPhysPage MemIntUnmapUserPage(MemContext * context, void * address);

/**
 * Combined MemIntUnmapUserPage() and MemPhysicalDeleteRef()
 *
 * @param context memory context to unmap from
 * @param address address of unmap and free (must be page aligned)
 */
static inline void MemIntUnmapUserPageAndFree(MemContext * context, void * address)
{
	MemPhysPage page = MemIntUnmapUserPage(context, address);

	if(page != INVALID_PAGE)
	{
	    MemPhysicalDeleteRef(page, 1);
	}
}

/**
 * @name Temporary pages
 *
 * Temporary virtual memory addresses used by the memory manager.
 *
 * @{
 */

#define MEM_TEMPPAGE1 ((void *) 0xFFFF0000)		///< Temporary page 1
#define MEM_TEMPPAGE2 ((void *) 0xFFFF4000)		///< Temporary page 2
#define MEM_TEMPPAGE3 ((void *) 0xFFFF8000)		///< Temporary page 3 (used in page fault handler)

#endif
