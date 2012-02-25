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

//Contains information about a zone of physical memory
typedef struct MemPhysicalZone
{
	MemPageStatus * headPtr;
	MemPageStatus * start;
	MemPageStatus * end;

} MemPhysicalZone;

//Memory zones (dma, kernel, high)
static MemPhysicalZone zones[3];

//Page counts
unsigned int MemPhysicalTotalPages;
unsigned int MemPhysicalFreePages;

//Sets up the zones using the given total number of pages
void MemPhysicalInit()
{
	//Calculate highest page from end of page state table
	unsigned int highestPage = (MemPageStateTableEnd - MemPageStateTable) / sizeof(MemPageStatus);

	//DMA zone
	if(highestPage <= 0x1000)
	{
		//Set zone to end of memory
		zones[MEM_DMA].end = &MemPageStateTable[highestPage];
	}
	else
	{
		//Set zone to end of DMA region
		zones[MEM_DMA].end = &MemPageStateTable[0x1000];
		zones[MEM_KERNEL].start = &MemPageStateTable[0x1000];
		zones[MEM_KERNEL].headPtr = &MemPageStateTable[0x1000];

		//Kernel zone
		if(highestPage <= MEM_KFIXED_MAX_PAGE)
		{
			//Set zone to end of memory
			zones[MEM_KERNEL].end = &MemPageStateTable[highestPage];
		}
		else
		{
			//Set kernel zone to end of Kernel region
			zones[MEM_KERNEL].end = &MemPageStateTable[MEM_KFIXED_MAX_PAGE];
			zones[MEM_HIGHMEM].start = &MemPageStateTable[MEM_KFIXED_MAX_PAGE];
			zones[MEM_HIGHMEM].headPtr = &MemPageStateTable[MEM_KFIXED_MAX_PAGE];

			//Set high zone to end of memory
			zones[MEM_HIGHMEM].end = &MemPageStateTable[highestPage];
		}
	}
}

//Allocates physical pages
MemPhysPage MemPhysicalAlloc(unsigned int number, int zone)
{
	//Validate parameters
	if(number == 0)
	{
		PrintLog(Error, "MemPhysicalAlloc: Request for 0 pages");
		return INVALID_PAGE;
	}

	if(zone < 0 || zone >= 3)
	{
		PrintLog(Error,"MemPhysicalAlloc: Invalid allocation mode");
		return INVALID_PAGE;
	}

	//Start zones loop
	for(; zone > 0; --zone)
	{
		//Ensure zone exists
		if(zones[zone].end > zones[zone].start)
		{
			//Start bitmap lookup
			MemPageStatus * head = zones[zone].headPtr;

			MemPageStatus * firstFree = NULL;
			unsigned int freeLength;

			do
			{
				//Wrap around head pointer
				if(head >= zones[zone].end)
				{
					head = zones[zone].start;
				}

				//Check if there is a free page
				if(head->refCount == 0)
				{
					if(firstFree == NULL)
					{
						//Set as first free
						firstFree = head;
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

						//Update zone head
						zones[zone].headPtr = head;

						//Return first page in set
						return ((unsigned int) firstFree - (unsigned int) MemPageStateTable) / sizeof(MemPageStatus);
					}
				}
				else
				{
					//No free here
					firstFree = NULL;
				}

				//Move on head
				++head;
			}
			while(head != zones[zone].headPtr);
		}
	}

	//If we're here, we're out of memory!
	Panic("MemPhysicalAlloc: Out of memory");
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
