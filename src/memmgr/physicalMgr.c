/*
 * physicalMgr.c
 *
 *  Created on: 6 Oct 2010
 *      Author: James
 */

#include "chaff.h"
#include "memmgr.h"

//Beginning of main (non ISA) section
#define physMainStart (MemPageStateTable + 4096)

//Head of the physical state table
static MemPageStatus * physHead = physMainStart;
static MemPageStatus * physHeadISA = MemPageStateTable;

//Page counts
unsigned int MemPhysicalTotalPages = 0;
unsigned int MemPhysicalFreePages = 0;

//Allocates physical pages
PhysPage MemPhysicalAlloc(unsigned int number /* = 1 */)
{
	MemPageStatus * startHead = physHead;

	MemPageStatus * firstFree = NULL;
	unsigned int freeLength;

	//Handle 0 page request
	if(number == 0)
	{
		PrintLog(Error, "MemPhysicalAllocate Request for 0 pages");
		return -1;
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
PhysPage MemPhysicalAllocISA(unsigned int number /* = 1 */)
{
	//Save head at beginning
	MemPageStatus * startHead = physHeadISA;

	MemPageStatus * firstFree = NULL;
	unsigned int freeLength;

	//Handle 0 page request
	if(number == 0)
	{
		PrintLog(Error, "MemPhysicalAllocateISA Request for 0 pages");
		return -1;
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

//Frees 1 physical page allocated by AllocatePage
void MemPhysicalFree(PhysPage page, unsigned int number /* = 1 */)
{
	//Free from bitmap
	for(; number > 0; --number, ++page)
	{
		MemPageStateTable[page].refCount--;
		++MemPhysicalFreePages;
	}
}
