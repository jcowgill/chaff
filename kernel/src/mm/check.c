/*
 * check.c
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
 *  Created on: 8 Feb 2012
 *      Author: james
 */

#include "chaff.h"
#include "process.h"
#include "mm/check.h"
#include "mm/region.h"

//Performs the user mode memory checks on the current memory context
static bool MemChecks(unsigned int addr, unsigned int length, unsigned int flagsReqd)
{
	MemContext * context;

	//Get correct checking context
	if(addr >= 0xC0000000)
	{
		context = MemKernelContext;
	}
	else
	{
		context = ProcCurrProcess->memContext;
	}

	//Find first region
	MemRegion * region = MemRegionFind(context, (void *) addr);

	while(region != NULL && (region->flags & flagsReqd) == flagsReqd)
	{
		//Calculate data left
		unsigned int end = region->start + region->length;
		unsigned int lengthLeft = end - addr;

		if(lengthLeft >= length)
		{
			//Passed (since we known this region is readable)
			return true;
		}

		//Move to next region if it is continuous
		if(region->listItem.next == &context->regions)
		{
			//This is the last region, fail
			return false;
		}

		region = ListEntry(region->listItem.next, MemRegion, listItem);

		if (region->start == end)
		{
			//Not continuous, fail
			return false;
		}

		//Update parameters
		addr = region->start;
		length -= lengthLeft;
	}

	return false;
}

//Verifies that an area of memory can be read by the kernel or user
// If data >= KERNEL_VIRTUAL_BASE, a kernel check is performed
// For user data, check it with MemCheckUserArea first
bool MemCanRead(void * data, unsigned int length)
{
	return MemChecks((unsigned int) data, length, MEM_READABLE);
}

//Verifies that an area of memory can be written by the kernel or user
// If data >= KERNEL_VIRTUAL_BASE, a kernel check is performed
// For user data, check it with MemCheckUserArea first
bool MemCanWrite(void * data, unsigned int length)
{
	return MemChecks((unsigned int) data, length, MEM_WRITABLE);
}
