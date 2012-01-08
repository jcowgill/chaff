/*
 * memmgrInt.h
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
 *  Created on: 4 Dec 2010
 *      Author: James
 */

#ifndef MEMMGRINT_H_
#define MEMMGRINT_H_

//Internal memory manager functions

#include "chaff.h"
#include "memmgr.h"

/*
Guide to counters in the memory manager
- Kernel Version
	This is used to determine when the kernel memory context has been updated
	so that it doesn't need to be copied to every context each time it is changed.
	(managed by region.c)
- Physical page refcount
	This is the number of contexts which are currently referencing a particular
	physical page of memory. Used to implement copy on write.
- Page table counter
	This keeps a record of the number of page tables in a directory so that page
	directories can be freed without having to check how many page tables they have
	inside (managed by pageMapping.c)
*/

//Memory page statuses
typedef struct
{
	//The number of references to the page
	unsigned int refCount;

} MemPageStatus;

//Page status area
#define MemPageStateTable ((MemPageStatus *) 0xFF800000)
extern MemPageStatus * MemPageStateTableEnd;		//Address after page state table (this can be NULL)

//Returns a pointer to the reference counter for the specified page
static inline unsigned int * MemIntPhysicalRefCount(PhysPage page)
{
	return &MemPageStateTable[page].refCount;
}

//Kernel page directory entries
extern PageDirectory kernelPageDirectory[1024];
extern PageTable kernelPageTable254[1024];
extern PageTable kernelPageTable253[1024];

//Raw page mapping
// Maps pages and handles all creation and management of page tables
void MemIntMapPage(MemContext * currContext, void * address, PhysPage page, RegionFlags flags);
void MemIntUnmapPage(MemContext * currContext, void * address);

//This function uses MemIntFreePageOrDecRefs to decrease reference count first
void MemIntUnmapPageAndFree(MemContext * currContext, void * address);

//Can be used for mapping tmp pages ONLY
// Do not use other functions for temporary pages
void MemIntMapTmpPage(void * address, PhysPage page);
void MemIntUnmapTmpPage(void * address);

//Free a page OR decrease it's reference count if it is > 1
void MemIntFreePageOrDecRefs(PhysPage page);

//Create region from already allocated MemRegion
// See MemRegionCreate
// Returns true on success
bool MemIntRegionCreate(MemContext * context, void * startAddress,
		unsigned int length, RegionFlags flags, MemRegion * newRegion);

//Page table locations
#define THIS_PAGE_DIRECTORY ((PageDirectory *) 0xFFFFF000)
#define THIS_PAGE_TABLES ((PageTable *) 0xFFC00000)

//Temporary pages
// These should be unmapped while outside a region function
#define MEM_TEMPPAGE1 ((void *) 0xFF400000)
#define MEM_TEMPPAGE2 ((void *) 0xFF401000)			//No page faults must occur while this is in use

//Temporary context change
#define CONTEXT_SWAP(context) \
{ \
	unsigned int _oldCR3 = getCR3(); \
	MemContextSwitchTo(context); \

#define CONTEXT_SWAP_END \
	setCR3(_oldCR3); \
}

#endif /* MEMMGRINT_H_ */
