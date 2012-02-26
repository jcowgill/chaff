/**
 * @file
 * Memory region and context manager
 *
 * Regions are contiguous areas of virtual memory used for user mode memory managment.
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

#ifndef MM_REGION_H_
#define MM_REGION_H_

#include "chaff.h"
#include "list.h"
#include "mm/physical.h"

/**
 * Flags assigned to memory regions
 */
typedef enum MemRegionFlags
{
	MEM_NOACCESS = 0,     ///< No access to memory
	MEM_READABLE = 1,     ///< Memory is readable (currently ignored - all memory is readable)
	MEM_WRITABLE = 2,     ///< Memory is writable
	MEM_EXECUTABLE = 4,   ///< Memory is executable
	MEM_CACHEDISABLE = 8, ///< Disables cache lookups for the region

	MEM_ALLFLAGS = 15     ///< All previous flags (used internally)

} MemRegionFlags;

struct MemContext;

/**
 * A region of virtual memory which some properties are applied to
 */
typedef struct MemRegion
{
	ListHead listItem;				///< Item in regions list

	struct MemContext * myContext;	///< Context this region is assigned to

	MemRegionFlags flags;			///< The properties of the region

	unsigned int start;				///< Pointer to start of region
	unsigned int length;			///< Length of region in pages

} MemRegion;

/**
 * A group of memory regions which make up the virtual memory space for a process
 */
typedef struct MemContext
{
	ListHead regions;				///< List of regions in this context
	MemPhysPage physDirectory;		///< Page directory physical page

	unsigned int refCount;			///< Memory context reference counter

} MemContext;

/**
 * Creates a new blank memory context
 *
 * The reference count will be 0
 */
MemContext * MemContextInit();

/**
 * Clones the current memory context
 *
 * The reference count will be 0.
 *
 * Do not clone the kernel context with this. MemContextInit() has the same effect and is faster.
 */
MemContext * MemContextClone();

/**
 * Switches to another memory context
 */
void MemContextSwitchTo(MemContext * context);

/**
 * Deletes a memory context
 *
 * You probably want MemContextDeleteReference() instead of this
 *
 * The context passed must not be the current context.
 *
 * #MEM_FIXED regions are deleted by this once it runs out of references
 *  if you want to "save" it, keep some references to it
 *
 * @param context memory context to delete
 */
void MemContextDelete(MemContext * context);

/**
 * Adds a reference to a memory context
 *
 * @param context context to increment reference count of
 */
static inline void MemContextAddReference(MemContext * context)
{
	context->refCount++;
}

/**
 * Deletes a reference to a memory context
 *
 * The context passed must not be the current context.
 *
 * #MEM_FIXED regions are deleted by this
 *
 * @param context memory context to delete
 */
void MemContextDeleteReference(MemContext * context);

/**
 * Creates a new blank memory region
 *
 * @param context context to add region to
 * @param startAddress address of the start of the region (must be page aligned)
 * @param length length of the region in bytes (must be page aligned)
 * @param flags flags to assign to the region
 * @retval NULL if an error occurs (logged using PrintLog())
 * @retval region on success
 */
MemRegion * MemRegionCreate(MemContext * context, void * startAddress,
		unsigned int length, MemRegionFlags flags);

/**
 * Frees the pages associated with a given region of memory without destroying the region
 *
 * Pages are automatically reallocated when memory is referenced again.
 * All the memory is wiped (filled with 0s).
 *
 * The address and length given must be within the bounds of the region given.
 *
 * The memory manager may free less pages than requested in certain situations.
 *
 * @param region region to free pages from
 * @param address start address to free
 * @param length length of data to free
 */
void MemRegionFreePages(MemRegion * region, void * address, unsigned int length);

/**
 * Finds the region which contains the given address
 *
 * @param context memory context to search
 * @param address address to search for
 * @retval NULL if there is no region allocated to that address
 * @retval region the region found for that address
 */
MemRegion * MemRegionFind(MemContext * context, void * address);

/**
 * Resizes a region
 *
 * @param region region to resize
 * @param newLength new length of the region
 */
void MemRegionResize(MemRegion * region, unsigned int newLength);

/**
 * Deletes the specified region
 *
 * @param region region to delete
 */
void MemRegionDelete(MemRegion * region);

/**
 * Kernel context data pointer
 *
 * @private
 */
extern MemContext MemKernelContextData;

/**
 * Pointer to the kernel memory context
 *
 * The kernel context is special and can only be passed to MemContextSwitchTo(),
 * MemContextAddReference() and MemContextDeleteReference().
 *
 * You MUST NOT pass it to any other memory manager function.
 */
#define MemKernelContext (&MemKernelContextData)

/**
 * Pointer to the current memory context
 *
 * Do not change this
 */
extern MemContext * MemCurrentContext;

#endif
