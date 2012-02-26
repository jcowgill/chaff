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
 * Maps a page to the given kernel virtual address
 *
 * The address supplied must be in the kernel virtual region (>= 0xF0000000).
 * You should almost always allocate the virtual address for this using @death.
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
