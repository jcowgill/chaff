/*
 * mm/region.h
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
 *  Created on: 8 Feb 2012
 *      Author: James
 */

#ifndef MM_REGION_H_
#define MM_REGION_H_

#include "chaff.h"
#include "list.h"
#include "mm/physical.h"

//Memory region and context management
// Regions are contiguous areas of virtual memory
//

/*
 * Memory Layout
 * -------------
 * 00000000 - 00000FFF	Reserved (cannot be mapped)
 * 00001000 - BFFFFFFF	User mode code
 * C0000000 - C03FFFFF	Mapped to first 4MB of physical space. Contains all kernel code.
 * C0400000 - CFFFFFFF	Driver code
 * D0000000 - DFFFFFFF	Kernel heap
 * F0000000 - FF6FFFFF	No use at the moment
 * FF700000 - FFFFFFFF	Memory manager use
 * 	FF700000 - FF7FFFFF		Temporary page mappings
 * 	FF800000 - FFBFFFFF		Physical page references
 * 	FFC00000 - FFFFFFFF		Current page tables
 */

//Bitmask containing the flags for memory regions
// Note that restrictions only apply to user mode
// All non-fixed pages use copy-on-write when the context is cloned
typedef enum MemRegionFlags
{
	MEM_NOACCESS = 0,
	MEM_READABLE = 1,
	MEM_WRITABLE = 2,
	MEM_EXECUTABLE = 4,
	MEM_FIXED = 8,
	MEM_CACHEDISABLE = 16,

	MEM_ALLFLAGS = 31

} MemRegionFlags;

struct MemContext;

//A region of virtual memory with the same properties
typedef struct STagMemRegion
{
	ListHead listItem;				//Item in regions list

	struct MemContext * myContext;	//Context this region is assigned to

	MemRegionFlags flags;			//The properties of the region

	unsigned int start;				//Pointer to start of region
	unsigned int length;			//Length of region in pages
	MemPhysPage firstPage;			//First page - used only for fixed regions

} MemRegion;

//A group of memory regions which make up the virtual memory space for a process
typedef struct MemContext
{
	ListHead regions;				//Regions in this context
	unsigned int kernelVersion;		//Version of kernel page directory for this context
	MemPhysPage physDirectory;		//Page directory physical page

	unsigned int refCount;			//Memory contex reference counter

} MemContext;

//Creates a new blank memory context
// The reference count will be 0
MemContext * MemContextInit();

//Clones this memory context
// The reference count will be 0
MemContext * MemContextClone();

//Switches to this memory context
void MemContextSwitchTo(MemContext * context);

//Deletes this memory context
// You should probably use MemContextDeleteReference instead of this
// MEM_FIXED memory is deleted by this once it runs out of references
//  if you want to "save" it, keep some references to it
void MemContextDelete(MemContext * context);

//Adds a reference to a memory context
static inline void MemContextAddReference(MemContext * context)
{
	context->refCount++;
}

//Deletes a reference to a memory context
// The context passed MUST NOT be the current context
// When the refCount reaches zero, the context will be destroyed
// MEM_FIXED memory IS DELETED by this
void MemContextDeleteReference(MemContext * context);


//Creates a new blank memory region
// If the context given is not the current or kernel context,
//  a temporary memory context switch may occur
// The start address MUST be page aligned
MemRegion * MemRegionCreate(MemContext * context, void * startAddress,
		unsigned int length, MemPhysPage firstPage, MemRegionFlags flags);

//Frees the pages associated with a given region of memory without destroying the region
// (pages will automatically be allocated when memory is referenced)
// The memory address range must be withing the bounds of the given region
void MemRegionFreePages(MemRegion * region, void * address, unsigned int length);

//Finds the region which contains the given address
// or returns NULL if there isn't one
MemRegion * MemRegionFind(MemContext * context, void * address);

//Resizes the region of allocated memory
// If the context of the region given is not the current or kernel context,
//  a temporary memory context switch may occur
void MemRegionResize(MemRegion * region, unsigned int newLength);

//Deletes the specified region
// If the context of the region given is not the current or kernel context,
//  a temporary memory context switch may occur
void MemRegionDelete(MemRegion * region);

//Kernel and current context
extern MemContext MemKernelContextData;
#define MemKernelContext (&MemKernelContextData)
extern MemContext * MemCurrentContext;

#endif
