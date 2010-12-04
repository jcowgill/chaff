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

//Raw page mapping
// Maps pages and handles all creation and management of page tables
void MemIntMapPage(void * address, PhysPage page, RegionFlags flags);
void MemIntUnmapPage(void * address);
void MemIntUnmapPageAndFree(void * address);

#endif /* MEMMGRINT_H_ */
