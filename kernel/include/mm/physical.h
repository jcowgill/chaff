/*
 * mm/physical.h
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

#ifndef MM_PHYSICAL_H_
#define MM_PHYSICAL_H_

//Physical Memory Manager
// These functions manage physical memory
// Generally the physical memory manager doesn't need to be accessed
// Use memory regions for allocating memory
//

//Physical page type
typedef int MemPhysPage;

//Page returned when an error occurs
#define INVALID_PAGE (-1)

//Page size
#define PAGE_SIZE 4096

//Allocates physical pages
// This function can return any free page
MemPhysPage MemPhysicalAlloc(unsigned int number);

//Allocates physical pages
// This function will never return any page with an offset > 16M
MemPhysPage MemPhysicalAllocISA(unsigned int number);

//Adds a reference to the given page(s)
void MemPhysicalAddRef(MemPhysPage page, unsigned int number);

//Deletes a reference to the given page(s)
void MemPhysicalDeleteRef(MemPhysPage page, unsigned int number);

//Frees physical pages allocated by AllocatePages or AllocateISAPages
// This forces the page to be freed regardless of reference count
void MemPhysicalFree(MemPhysPage page, unsigned int number);

//Number of pages in total and number of free pages of memory
extern unsigned int MemPhysicalTotalPages;
extern unsigned int MemPhysicalFreePages;

//Memory page statuses
typedef struct
{
	//The number of references to this page (contexts using it)
	unsigned int refCount;

} MemPageStatus;

//Page status area
#define MemPageStateTable ((MemPageStatus *) 0xFF800000)
extern MemPageStatus * MemPageStateTableEnd;		//Address after page state table (this can be NULL)

//Returns a pointer to the reference counter for the specified page
static inline unsigned int MemPhysicalRefCount(MemPhysPage page)
{
	return MemPageStateTable[page].refCount;
}

#endif
