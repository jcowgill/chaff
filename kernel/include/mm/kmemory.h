/**
 * @file
 * Functions for managing kernel memory
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

#ifndef MM_KMEMORY_H_
#define MM_KMEMORY_H_

#include "chaff.h"
#include "list.h"
#include "mm/physical.h"

struct MemCache;

/**
 * @name Slab flags
 * @{
 */

#define MEM_SLAB_DMA 1		///< Cache allocates memory using #MEM_DMA instead of #MEM_KERNEL
#define MEM_SLAB_LARGE 2	///< Cache uses large slabs (set automatically) @private

/** @} */

/**
 * Contains information about a cache of objects used by the slab allocator
 */
typedef struct MemCache
{
    ListHead cacheList;         ///< List of all the caches in the system

    ListHead slabsFull;         ///< List of full slabs
    ListHead slabsPartial;      ///< List of partially full slabs
    ListHead slabsEmpty;        ///< List of empty slabs

    unsigned int objectSize;    ///< Size of objects used in this cache
    unsigned int flags;			///< Slab flags
    unsigned int pagesPerSlab;	///< Number of physical pages to allocate per slab
    unsigned int objectsPerSlab;///< Number of objects in each slab

} MemCache;

/**
 * Marks the end of the free list (slab is full)
 */
#define MEM_SLAB_END    ((unsigned int) ~0)

/**
 * Contains information about a slab of objects
 */
typedef struct MemSlab
{
	MemCache * cache;           ///< Cache used by this slab
    ListHead slabList;			///< Entry in the cache slab list

    unsigned int activeObjs;	///< Number of objects in use
    MemPhysPage memory;			///< Physical page of the start of the slab
    unsigned int * freePtr;		///< Pointer to first free object id

} MemSlab;

/**
 * Creates a new slab cache
 *
 * @param size size of objects allocated by the cache
 * @param flags any flags to create the cache with
 * @return pointer to the new cache or NULL on error
 */
MemCache * MemSlabCreate(unsigned int size, unsigned int flags);

/**
 * Destroys a slab cache
 *
 * The slab must have no allocated objects.
 *
 * @param cache cache to destroy
 * @retval true on success
 * @retval false if objects still exist
 */
bool MemSlabDestroy(MemCache * cache);

/**
 * Allocates an object from a slab cache
 *
 * @param cache cache to allocate from
 */
void * MemSlabAlloc(MemCache * cache);

/**
 * Allocates and zeroes out an object from the slab cache
 *
 * Generally, you should not call this on a cache using constructors and destructors
 *
 * @param cache cache to allocate from
 */
void * MemSlabZAlloc(MemCache * cache);

/**
 * Frees an object allocated by MemSlabAlloc() after it has been used
 *
 * @param cache cache the object was originally allocated from
 * @param ptr pointer to object to free
 */
void MemSlabFree(MemCache * cache, void * ptr);

/**
 * Frees all the slabs in the cache which are empty
 *
 * @param cache cache to free slabs from
 * @return number of freed pages
 */
int MemSlabShrink(MemCache * cache);

/**
 * Reserves virtual memory with the given size.
 * 
 * The memory reserved is placed in the high virtual memory zone and cannot be
 * used with physical devices. You generally use this function for large
 * allocations which do not need to be accessed by hardware.
 * 
 * The number of bytes given is rounded up to the next page offset
 * (only multiples of #PAGE_SIZE are allocated).
 * 
 * Memory is not actually allocated with this function, you must do it yourself
 * with MemMapPage() or MemVirtualAlloc().
 * 
 * @param bytes number of bytes to reserve
 * @return virtual memory address
 */
void * MemVirtualReserve(unsigned int bytes);

/**
 * Unreserves memory reserved by MemVirtualReserve()
 * 
 * Do not pass pointers from MemVirtualAlloc() to this function.
 * 
 * @param ptr pointer returned by MemVirtualReserve() to free
 */
void MemVirtualUnReserve(void * ptr);

/**
 * Allocates virtual memory with the given size.
 * 
 * This function is identical to MemVirtualReserve() except it allocates
 * physical memory for the allocated memory (you usually want this function).
 * 
 * @param bytes number of bytes to allcate
 * @return virtual memory address
 */
void * MemVirtualAlloc(unsigned int bytes);

/**
 * Frees memory allocated using MemVirtualAlloc()
 * 
 * Frees the physical memory and unreserves memory allocated using MemVirtualAlloc().
 * Do not pass pointers from MemVirtualReserve() to this function.
 * 
 * @param ptr pointer returned by MemVirtualAlloc() to free
 */
void MemVirtualFree(void * ptr);


/**
 * Maps a page to the given kernel virtual address
 *
 * The address supplied must be in the kernel virtual region (>= 0xF0000000).
 * You should almost always allocate the virtual address for this using MemVirtualReserve().
 *
 * @note Implemented in pageMapping.c
 *
 * @param address address to map to
 * @param page page number to map to that address
 * @retval true the page was successfully mapped
 * @retval false the address given has already been mapped / is unmappable
 */
bool MemMapPage(void * address, MemPhysPage page);

/**
 * Unmaps the page mapped to the given virtual address
 *
 * The address supplied must be in the kernel virtual region (>= 0xF0000000).
 *
 * Do not unmap addresses unless you mapped them or you know what you are doing!
 *
 * @note Implemented in pageMapping.c
 *
 * @param address address to unmap
 * @return the page which was mapped or INVALID_PAGE if nothing is mapped
 */
MemPhysPage MemUnmapPage(void * address);

#endif
