/**
 * @file
 * Physical memory manager
 *
 * Generally you don't need to access these directly, use regions instead.
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

#ifndef MM_PHYSICAL_H_
#define MM_PHYSICAL_H_

/**
 * Type used for physical page identifiers
 */
typedef int MemPhysPage;

/**
 * Page returned on error
 */
#define INVALID_PAGE (-1)

/**
 * Page size
 */
#define PAGE_SIZE 4096

//Allocates physical pages
// This function can return any free page

/**
 * Allocates normal physical pages
 *
 * This function allocates the specified number of physical pages in contiguous physical space.
 *
 * The pages returned will have a reference count of 1.
 *
 * @param number number of pages to allocate
 * @return the first page in the allocated series
 * @bug panics when out of memory
 */
MemPhysPage MemPhysicalAlloc(unsigned int number);

/**
 * Allocates physical pages in the ISA region (< 16M)
 *
 * This function allocates the specified number of physical pages in contiguous physical space.
 *
 * The pages returned will have a reference count of 1.
 *
 * @param number number of pages to allocate
 * @return the first page in the allocated series
 * @bug panics when out of memory
 */
MemPhysPage MemPhysicalAllocISA(unsigned int number);

//Adds a reference to the given page(s)
void MemPhysicalAddRef(MemPhysPage page, unsigned int number);

//Deletes a reference to the given page(s)
void MemPhysicalDeleteRef(MemPhysPage page, unsigned int number);

/**
 * Frees physical pages allocated by MemPhysicalAlloc() and MemPhysicalAllocISA()
 *
 * This sets the reference count of the pages to 0
 *
 * @param page first page in series to free
 * @param number number of contiguous pages to free
 */
void MemPhysicalFree(MemPhysPage page, unsigned int number);

/**
 * Total number of pages in RAM
 */
extern unsigned int MemPhysicalTotalPages;

/**
 * Number of free pages in RAM
 */
extern unsigned int MemPhysicalFreePages;

/**
 * Memory page status structure
 *
 * Contains page reference counts
 */
typedef struct
{
	/**
	 * The number of references to this page (contexts)
	 */
	unsigned int refCount;

} MemPageStatus;

/**
 * Address of global page state table
 */
#define MemPageStateTable ((MemPageStatus *) 0xFF800000)

/**
 * Address of the end of the page state table (after end)
 */
extern MemPageStatus * MemPageStateTableEnd;

/**
 * Returns the number of references to a page
 *
 * @param page page to check
 * @return number of references to this page
 */
static inline unsigned int MemPhysicalRefCount(MemPhysPage page)
{
	return MemPageStateTable[page].refCount;
}

#endif
