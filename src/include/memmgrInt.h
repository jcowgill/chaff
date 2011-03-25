/*
 * memmgrInt.h
 *
 *  Created on: 4 Dec 2010
 *      Author: James
 */

#ifndef MEMMGRINT_H_
#define MEMMGRINT_H_

//Internal memory manager functions

#include "chaff.h"
#include "memmgr.h"

//Kernel page directory entries
extern PageDirectory kernelPageDirectory[1024];
extern PageTable kernelPageTable254[1024];
extern PageTable kernelPageTable253[1024];

//Raw page mapping
// Maps pages and handles all creation and management of page tables
void MemIntMapPage(MemContext * currContext, void * address, PhysPage page, RegionFlags flags);
void MemIntUnmapPage(MemContext * currContext, void * address);
void MemIntUnmapPageAndFree(MemContext * currContext, void * address);

//Can be used for mapping tmp pages ONLY
// Do not use other functions for temporary pages
void MemIntMapTmpPage(void * address, PhysPage page);
void MemIntUnmapTmpPage(void * address);

//Create region from already allocated MemRegion
// See MemRegionCreate
// Returns true on success
bool MemIntRegionCreate(MemContext * context, void * startAddress,
		unsigned int length, RegionFlags flags, MemRegion * newRegion);

//Temporary pages
// These should be unmapped while outside a region function
#define MEM_TEMPPAGE1 ((void *) 0xFF400000)
#define MEM_TEMPPAGE2 ((void *) 0xFF401000)

//Temporary context change
#define CONTEXT_SWAP(context) \
{ \
	unsigned int _oldCR3 = getCR3(); \
	MemContextSwitchTo(context); \

#define CONTEXT_SWAP_END \
	setCR3(_oldCR3); \
}

#endif /* MEMMGRINT_H_ */
