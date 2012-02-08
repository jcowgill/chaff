/*
 * pageFault.c
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
 *  Created on: 10 Dec 2010
 *      Author: James
 */

#include "chaff.h"
#include "inlineasm.h"
#include "process.h"
#include "mm/region.h"
#include "mm/misc.h"
#include "mm/physical.h"
#include "mm/pagingInt.h"

//Page fault handler
void MemPageFaultHandler(IntrContext * intContext)
{
	//Get address causing fault
	void * faultAddress = getCR2();
	unsigned int errorCode = intContext->intrError;

	//Check which error and handle accordingly
	// P  (0) = Set if caused by protection violation, unset if caused by page not present
	// WR (1) = Set if fault was caused by a write
	// US (2) = Set if fault originated in usermode
	// RSVD (3) = Set if any page table reserved bits are set
	// ID (4) = Set if fault was caused by instruction fetch

	//Check reserved error
	if(errorCode & (1 << 4))
	{
		Panic("MemPageFaultHandler: CPU indicated reserved bits have been set");
	}

	//Get region of address
	MemRegion * region;

	if(faultAddress >= KERNEL_VIRTUAL_BASE)
	{
		region = MemRegionFind(MemKernelContext, faultAddress);
	}
	else
	{
		region = MemRegionFind(MemCurrentContext, faultAddress);
	}

	//If null, out of valid area
	if(region != NULL)
	{
		//Check fault cause
		if(errorCode & (1 << 0))
		{
			//Protection violation (user mode accessed supervisor)
			// Or a write to a read-only page

			MemPageTable * table = THIS_PAGE_TABLES + ((unsigned int) faultAddress >> 12);

			//Check if copy-on-write
			if(!(table->writable) && (region->flags & MEM_WRITABLE))
			{
				//Get page ref count
				unsigned int * refCount = MemPhysicalRefCount(table->pageID);

				if(*refCount != 1)
				{
					//Duplicate page first
					unsigned int * basePageAddr = (unsigned int *) ((unsigned int) faultAddress & 0xFFFFF000);
					MemPhysPage newPage = MemPhysicalAlloc(1);

					MemIntMapTmpPage(MEM_TEMPPAGE2, newPage);
						MemCpy(MEM_TEMPPAGE2, basePageAddr, 4096);
					MemIntUnmapTmpPage(MEM_TEMPPAGE2);

					//Update page id and old page's count
					table->pageID = newPage;
					--(*refCount);
				}

				//Make page writable
				table->writable = 1;
				invlpg(faultAddress);
				return;
			}
		}
		else
		{
			//Non-present page
			// Allocate new page for memory if not fixed
			if(!(region->flags & MEM_FIXED))
			{
				//Map page
				unsigned int * basePageAddr = (unsigned int *) ((unsigned int) faultAddress & 0xFFFFF000);
				MemIntMapPage(MemCurrentContext, basePageAddr, MemPhysicalAlloc(1), region->flags);

				//Wipe page
				MemSet(basePageAddr, 0, 4096);
				return;
			}
		}
	}

	//If we're here, it's an error
	if(errorCode & (1 << 2))
	{
		//User mode fault
		ProcSignalSendOrCrash(SIGSEGV);
	}
	else
	{
		//Kernel mode fault
		if((unsigned int) faultAddress < 4096)
		{
			Panic("MemPageFaultHandler: Unable to handle kernel NULL pointer dereference");
		}
		else
		{
			Panic("MemPageFaultHandler: Unable to handle kernel page fault");
#warning ...at address...
		}
	}
}
