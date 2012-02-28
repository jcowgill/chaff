/*
 * vmalloc.c
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
 *  Created on: 28 Feb 2012
 *      Author: James
 */

#include "chaff.h"
#include "mm/kmemory.h"
#include "mm/physical.h"

//Kernel Virtual Allocator
// This isn't very efficient, some day it may be improved

//Information about the allocation of the virtual page
typedef struct VirtualPage
{
	//1 if allocated
	unsigned allocated : 1;

	//1 if this is the first page in an allocated section
	unsigned firstPage : 1;

} VirtualPage;

//Number of pages managed by the system
#define VIRT_PAGES 0xFFFC

//Allocation data about each page
static VirtualPage allocData[VIRT_PAGES];

//Reserves virtual memory with the given size.
void * MemVirtualReserve(unsigned int bytes)
{
	//Check for 0 bytes
	if(bytes == 0)
	{
		PrintLog(Error, "MemVirtualReserve: request for 0 bytes");
		return NULL;
	}

	//Convert to pages
	unsigned int pages = (bytes + PAGE_SIZE - 1) / PAGE_SIZE;

	unsigned int firstPage;
	unsigned int pagesSoFar = 0;

	//Start allocation
	for(unsigned int i = 0; i < VIRT_PAGES; i++)
	{
		//Unallocated?
		if(allocData[i].allocated == 0)
		{
			//Add to page count
			if(pagesSoFar++ == 0)
			{
				firstPage = i;
			}

			//Enough?
			if(pagesSoFar == pages)
			{
				//Mark first as allocated
				allocData[firstPage].allocated = 1;
				allocData[firstPage].firstPage = 1;

				//Mark others allocated
				for(; i > firstPage; --i)
				{
					allocData[i].allocated = 1;
				}

				//Return pointer
				return (void *) (firstPage * PAGE_SIZE + 0xF0000000);
			}
		}
		else
		{
			//Clear pages found
			pagesSoFar = 0;
		}
	}

	//Nothing found
	PrintLog(Critical, "MemVirtualReserve: out of virtual memory");
	return NULL;
}

//Unreserves memory reserved by MemVirtualReserve()
static void DoUnreserve(void * ptr, bool freePages)
{
	//Ignore NULL
	if(ptr == NULL)
	{
		PrintLog(Warning, "MemVirtualUnReserve: attempt to free NULL pointer");
		return;
	}

	//Find index in data table
	unsigned int index = ((unsigned int) ptr - 0xF0000000) / PAGE_SIZE;

	//Verify we're unreserving a first page
	if(allocData[index].firstPage == 0)
	{
		PrintLog(Error, "MemVirtualUnReserve: invalid pointer provided");
		return;
	}

	allocData[index].firstPage = 0;

	//Deallocate until another firstPage is encounted
	for(; index < VIRT_PAGES; ++index)
	{
		//If this is a first page or unallocated, exit
		if(allocData[index].firstPage || allocData[index].allocated == 0)
		{
			break;
		}

		//Free memory
		allocData[index].allocated = 0;

		//Free physical memory
		if(freePages)
		{
			MemPhysicalFree(MemUnmapPage((void *) (index * PAGE_SIZE + 0xF0000000)), 1);
		}
	}
}

//Unreserves memory reserved by MemVirtualReserve()
void MemVirtualUnReserve(void * ptr)
{
	DoUnreserve(ptr, false);
}

//Allocates virtual memory with the given size.
void * MemVirtualAlloc(unsigned int bytes)
{
	//Reserve memory
	void * data = MemVirtualReserve(bytes);

	//Allocate physical pages
	for(unsigned int off = 0; off < bytes; off += PAGE_SIZE)
	{
		MemMapPage((char *) data + off, MemPhysicalAlloc(1, MEM_HIGHMEM));
	}

	return data;
}

//Frees memory allocated using MemVirtualAlloc()
void MemVirtualFree(void * ptr)
{
	DoUnreserve(ptr, true);
}
