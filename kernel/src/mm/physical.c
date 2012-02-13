/*
 * physicalMgr.c
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
 *  Created on: 6 Oct 2010
 *      Author: James
 */

#include "chaff.h"
#include "mm/physical.h"

//Beginning of main (non ISA) section
#define physMainStart (MemPageStateTable + 4096)

//Head of the physical state table
static MemPageStatus * physHead = physMainStart;
static MemPageStatus * physHeadISA = MemPageStateTable;

//Page counts
unsigned int MemPhysicalTotalPages = 0;
unsigned int MemPhysicalFreePages = 0;

//Allocates physical pages
MemPhysPage MemPhysicalAlloc(unsigned int number)
{
	MemPageStatus * startHead = physHead;

	MemPageStatus * firstFree = NULL;
	unsigned int freeLength;

	//Handle 0 page request
	if(number == 0)
	{
		PrintLog(Error, "MemPhysicalAllocate Request for 0 pages");
		return INVALID_PAGE;
	}

	//Check for high memory
	if(MemPageStateTableEnd >= physMainStart)
	{
		//Go through bitmap looking for area big enough
		do
		{
			//Ensure head is in the bitmap
			if(physHead >= MemPageStateTableEnd)
			{
				//Restore to beginning
				physHead = physMainStart;
			}

			//Check if there is a free page
			if(physHead->refCount == 0)
			{
				if(firstFree == NULL)
				{
					//Set as first free
					firstFree = physHead;
					freeLength = 1;
				}
				else
				{
					//Set as another free
					++freeLength;
				}

				//Enough?
				if(freeLength == number)
				{
					//Decrement free pages
					MemPhysicalFreePages -= number;

					//Increment refcounts
					for(MemPageStatus * page = firstFree; number > 0; ++page, --number)
					{
						page->refCount = 1;
					}

					return ((unsigned int) firstFree - (unsigned int) MemPageStateTable) / sizeof(MemPageStatus);
				}
			}
			else
			{
				//No free here
				firstFree = NULL;
			}

			//Move on head
			++physHead;
		}
		while(physHead != startHead);
	}

	//Out of main memory - use ISA region
	return MemPhysicalAllocISA(number);
}

//Allocates physical pages
// This function will never return any page with an offset > 16M
MemPhysPage MemPhysicalAllocISA(unsigned int number)
{
	//Save head at beginning
	MemPageStatus * startHead = physHeadISA;

	MemPageStatus * firstFree = NULL;
	unsigned int freeLength;

	//Handle 0 page request
	if(number == 0)
	{
		PrintLog(Error, "MemPhysicalAllocateISA Request for 0 pages");
		return INVALID_PAGE;
	}

	//Go through bitmap looking for area big enough
	do
	{
		//Ensure head is in the bitmap
		if(physHeadISA >= physMainStart || physHeadISA >= MemPageStateTableEnd)
		{
			//Restore to beginning
			physHeadISA = MemPageStateTable;
		}

		//Check if there is a free page
		if(physHeadISA->refCount == 0)
		{
			if(firstFree == NULL)
			{
				//Set as first free
				firstFree = physHeadISA;
				freeLength = 0;
			}
			else
			{
				//Set as another free
				++freeLength;
			}

			//Enough?
			if(freeLength == number)
			{
				//Decrement free pages
				MemPhysicalFreePages -= number;

				//Increment refcounts
				for(MemPageStatus * page = firstFree; number > 0; ++page, --number)
				{
					page->refCount = 1;
				}

				return ((unsigned int) firstFree - (unsigned int) MemPageStateTable) / sizeof(MemPageStatus);
			}
		}
		else
		{
			//No free here
			firstFree = NULL;
		}

		//Move on head
		++physHeadISA;
	}
	while(physHeadISA != startHead);

	//Out of memory!
	Panic("Out of memory");
}

//Adds a reference to the given page(s)
void MemPhysicalAddRef(MemPhysPage page, unsigned int number)
{
	//Increment counter
	for(; number > 0; --number, ++page)
	{
		MemPageStateTable[page].refCount++;
	}
}

//Deletes a reference to the given page(s)
void MemPhysicalDeleteRef(MemPhysPage page, unsigned int number)
{
	//Decrement counter
	for(; number > 0; --number, ++page)
	{
		//If it can be decremented, decrement it
		if(MemPageStateTable[page].refCount > 0)
		{
			MemPageStateTable[page].refCount--;

			//If it's zero, increment free page count
			if(MemPageStateTable[page].refCount == 0)
			{
				++MemPhysicalFreePages;
			}
		}
	}
}

//Frees physical pages allocated by AllocatePage
void MemPhysicalFree(MemPhysPage page, unsigned int number)
{
	//Free from bitmap
	for(; number > 0; --number, ++page)
	{
		MemPageStateTable[page].refCount = 0;
		++MemPhysicalFreePages;
	}
}
