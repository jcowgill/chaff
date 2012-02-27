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
#include "mm/physical.h"

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
 * @retval true the page was successfully unmapped
 * @retval false the address given is not mapped / is unmappable
 */
bool MemUnmapPage(void * address);

#endif
