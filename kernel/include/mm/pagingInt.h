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
 * Kernel page directory
 */
extern MemPageDirectory kernelPageDirectory[1024];

/**
 * 254th Kernel page directory
 */
extern MemPageTable kernelPageTable254[1024];

/**
 * 253rd Kernel page directory
 */
extern MemPageTable kernelPageTable253[1024];

/**
 * Maps a page to a particular address
 *
 * @param currContext the current memory context
 * @param address address of page to map (must be page aligned)
 * @param page physical page to map
 * @param flags flags to assign to page (must be same as region flags)
 */
void MemIntMapPage(MemContext * currContext, void * address, MemPhysPage page, MemRegionFlags flags);

/**
 * Unmaps a page
 *
 * @param currContext the current memory context
 * @param address address of page to unmap (must be page aligned)
 */
void MemIntUnmapPage(MemContext * currContext, void * address);

/**
 * Combined MemIntUnmapPage() and MemIntFreePageOrDecRefs()
 *
 * @param currContext the current memory context
 * @param address address of unmap and free (must be page aligned)
 */
void MemIntUnmapPageAndFree(MemContext * currContext, void * address);

/**
 * Maps a temporary page
 *
 * This function does fewer checks but can ONLY be used for pages in the temporary region.
 *
 * Do NOT use the other page mappers for temporary pages.
 *
 * @param address address to map at
 * @param page physical page to map
 */
void MemIntMapTmpPage(void * address, MemPhysPage page);

/**
 * Unmaps a temporary page mapped with MemIntMapTmpPage()
 *
 * @param address address to unmap from
 */
void MemIntUnmapTmpPage(void * address);

/**
 * Decrements the reference count of a page
 *
 * ... and frees it when it has a reference count of 0
 *
 * @param page physical page to free
 */
void MemIntFreePageOrDecRefs(MemPhysPage page);


/**
 * Location of page directory for the current memory context
 *
 * This is automatically updated and ALWAYS has the table for the current context
 */
#define THIS_PAGE_DIRECTORY ((MemPageDirectory *) 0xFFFFF000)

/**
 * Location of page tables for the current memory context
 *
 * This is automatically updated and ALWAYS has the table for the current context
 */
#define THIS_PAGE_TABLES ((MemPageTable *) 0xFFC00000)

/**
 * Temporary page 1
 *
 * Must be unmapped while outside memory functions
 */
#define MEM_TEMPPAGE1 ((void *) 0xFF400000)

/**
 * Temporary page 2
 *
 * Must be unmapped while outside memory functions
 */
#define MEM_TEMPPAGE2 ((void *) 0xFF401000)

/**
 * Temporarily changes memory context
 *
 * Example:
 * @code
 * ...
 * CONTEXT_SWAP(newContext)
 *
 *     someCodeHere();
 *
 * CONTEXT_SWAP_END
 * ...
 * @endcode
 */
#define CONTEXT_SWAP(context) \
{ \
	unsigned int _oldCR3 = getCR3(); \
	MemContextSwitchTo(context);

/**
 * Ends temporary change of context
 */
#define CONTEXT_SWAP_END \
	setCR3(_oldCR3); \
}

#endif
