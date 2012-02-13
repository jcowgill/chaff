/*
 * mm/pagingInt.h
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
 *  Created on: 7 Feb 2012
 *      Author: James
 */

#ifndef MM_PAGINGINT_H_
#define MM_PAGINGINT_H_

//Internal paging functions and structures
//

#include "chaff.h"
#include "mm/region.h"
#include "mm/physical.h"

//The structure for a x86 page directory
typedef union
{
	//Gets the raw value of the page directory
	unsigned int rawValue;

	struct
	{
		//1 if page is in use
		unsigned int present		: 1;

		//1 if page can be written to (otherwise writing causes a page fault)
		unsigned int writable		: 1;

		//1 if page is available from user mode
		unsigned int userMode		: 1;

		//1 if all writes should bypass the cache
		unsigned int writeThrough	: 1;

		//1 if all reads should bypass the cache
		unsigned int cacheDisable	: 1;

		//1 if the page has been accessed since this was last set to 0
		unsigned int accessed		: 1;

		//1 if the page has been written to since this was last set to 0
		// Ignored if this is a page table entry
		unsigned int dirty			: 1;

		//1 if this entry references a 4MB page rather than a page table
		unsigned int hugePage		: 1;

		//1 if this page should not be invalidated when changing CR3
		// Must enable this feature in a CR4 bit first
		// Ignored if this is a page table entry
		unsigned int global			: 1;

		unsigned int /* can be used by OS */	: 3;

		//The page frame ID of the 4MB page or page table
		// If this is a 4MB page, it MUST be aligned to 4MB (or we'll crash)
		MemPhysPage pageID			: 20;
	};

} MemPageDirectory;

//The structure for a x86 page table
typedef union
{
	//Gets the raw value of the page table
	unsigned int rawValue;

	struct
	{
		//1 if page is in use
		unsigned int present		: 1;

		//1 if page can be written to (otherwise writing causes a page fault)
		unsigned int writable		: 1;

		//1 if page is available from user mode
		unsigned int userMode		: 1;

		//1 if all writes should bypass the cache
		unsigned int writeThrough	: 1;

		//1 if all reads should bypass the cache
		unsigned int cacheDisable	: 1;

		//1 if the page has been accessed since this was last set to 0
		unsigned int accessed		: 1;

		//1 if the page has been written to since this was last set to 0
		unsigned int dirty			: 1;
		unsigned int /* reserved */	: 1;

		//1 if this page should not be invalidated when changing CR3
		// Must enable this feature in a CR4 bit first
		unsigned int global			: 1;

		//Not used by CPU
		// 3 bits of table counter to count number of pages allocated in page table
		// tableCount is used only in the first 5 page table entries
		unsigned int tableCount		: 3;

		//The page frame ID of the 4KB page
		MemPhysPage pageID			: 20;
	};

} MemPageTable;

//Kernel page directory entries
extern MemPageDirectory kernelPageDirectory[1024];
extern MemPageTable kernelPageTable254[1024];
extern MemPageTable kernelPageTable253[1024];

//Raw page mapping
// Maps pages and handles all creation and management of page tables
void MemIntMapPage(void * address, MemPhysPage page, MemRegionFlags flags);
void MemIntUnmapPage(void * address);

//This function uses MemIntFreePageOrDecRefs to decrease reference count first
void MemIntUnmapPageAndFree(void * address);

//Can be used for mapping tmp pages ONLY
// Do not use other functions for temporary pages
void MemIntMapTmpPage(void * address, MemPhysPage page);
void MemIntUnmapTmpPage(void * address);

//Page table locations
#define THIS_PAGE_DIRECTORY ((MemPageDirectory *) 0xFFFFF000)
#define THIS_PAGE_TABLES ((MemPageTable *) 0xFFC00000)

//Temporary pages
// These should be unmapped while outside a region function
#define MEM_TEMPPAGE1 ((void *) 0xFF400000)
#define MEM_TEMPPAGE2 ((void *) 0xFF401000)			//No page faults must occur while this is in use

//Temporary context change
#define CONTEXT_SWAP(context) \
{ \
	unsigned int _oldCR3 = getCR3(); \
	MemContextSwitchTo(context);

#define CONTEXT_SWAP_END \
	setCR3(_oldCR3); \
}

#endif
