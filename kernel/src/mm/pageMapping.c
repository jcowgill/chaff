/*
 * pageMapping.c
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
 *  Created on: 4 Dec 2010
 *      Author: James
 */

#include "chaff.h"
#include "inlineasm.h"
#include "mm/pagingInt.h"
#include "mm/region.h"
#include "mm/physical.h"
#include "mm/kmemory.h"

//Kernel page mapper
bool MemMapPage(void * address, MemPhysPage page)
{
	//Validate address
	unsigned int addr = ((unsigned int) address) & 0xFFFFF000;
	if(addr < MEM_KFIXED_MAX || addr == 0xFFFFC000)
	{
		return false;
	}

	//Get page table entry
	MemPageTable * tableEntry = &MemVirtualPageTables[(addr - MEM_KFIXED_MAX) / 4096];

	//Already mapped?
	if(tableEntry->present)
	{
		return false;
	}
	else
	{
		//Write page information
		tableEntry->rawValue = 0;
		tableEntry->present = 1;
		tableEntry->writable = 1;
		tableEntry->global = 1;
		tableEntry->pageID = page;

		invlpg(address);
		return true;
	}
}

//Kernel page unmapper
bool MemUnmapPage(void * address)
{
	//Validate address
	unsigned int addr = ((unsigned int) address) & 0xFFFFF000;
	if(addr < MEM_KFIXED_MAX)
	{
		return false;
	}

	//Get page table entry
	MemPageTable * tableEntry = &MemVirtualPageTables[(addr - MEM_KFIXED_MAX) / 4096];

	//Already mapped?
	if(tableEntry->present)
	{
		//Wipe value and invalidate
		tableEntry->rawValue = 0;

		invlpg(address);
		return true;
	}
	else
	{
		return false;
	}
}

//Increments the counter for the given page directory
static void IncrementCounter(MemPageDirectory * dir)
{
	//Increment from least significant to most significant
	for(int i = 0; ; i++)
	{
	    //Increment, and it it's too much, go to next one
	    if(++((MemPageTable *) MemPageAddr(dir[i].pageID))->tableCount != 0)
	    {
	        //Finished, exit
	        return;
		}
	}
}

//Decrements the counter for the given page directory
// Returns true if no pages left
static bool DecrementCounter(MemPageDirectory * dir)
{
	//Handle first iteration separately
	if(--((MemPageTable *) MemPageAddr(dir[0].pageID))->tableCount == 0)
	{
		return false;
	}

	//Decrement from second to least significant to most significant
	for(int i = 1; i < 5; ++i)
	{
	    //Decrement, if it was not 0 then go to next one
		// If it was zero, we must "carry" to the next place
		if(((MemPageTable *) MemPageAddr(dir[i].pageID))->tableCount-- != 0)
		{
			return false;
		}
	}
	
	//If we're here, no pages in table
	return true;
}

//Maps user mode pages
void MemIntMapUserPage(MemContext * context, void * address, MemPhysPage page, MemRegionFlags flags)
{
	//Ignore request if no readable, writable or executable flags
	if((flags & (MEM_READABLE | MEM_WRITABLE | MEM_EXECUTABLE)) == 0)
	{
	    return;
	}

	//Verify kernel maps are in the virtual region
	if(address >= KERNEL_VIRTUAL_BASE)
	{
		Panic("MemIntMapUserPage: cannot map kernel pages");
	}

	//Ensure page table for address exists
	unsigned int addr = (unsigned int) address;
	MemPageDirectory * pDir = MemGetPageDirectory(context, addr);

	if(!pDir->present)
	{
		//Create page table
		pDir->pageID = MemPhysicalAlloc(1, MEM_KERNEL);
		pDir->writable = 1;
		pDir->userMode = 1;
		pDir->present = 1;
		
		//Wipe page
		MemSet(MemPageAddr(pDir->pageID), 0, 4096);
	}

	//Get table entry
	MemPageTable * pTable = MemGetPageTable(pDir, addr);

	//Check if we'll be overwriting it
	if(pTable->present)
	{
	    //Print warning and wipe entry
	    PrintLog(Warning, "MemIntMapPage: Request to overwrite page table entry");
	    
	    unsigned int tmpCount = pTable->tableCount;
	    pTable->rawValue = 0;
	    pTable->tableCount = tmpCount;
	    
	    invlpg(address);
	}
	else
	{
		IncrementCounter(pDir);
	}
	
	//Set page properties
	pTable->present = 1;
	pTable->userMode = 1;
	pTable->writable = (flags & MEM_WRITABLE) ? 1 : 0;
	pTable->cacheDisable = (flags & MEM_CACHEDISABLE) ? 1 : 0;
	pTable->pageID = page;
}

//Unmaps a user mode page and returns the page which was unmapped
MemPhysPage MemIntUnmapUserPage(MemContext * context, void * address)
{
	//Refuse kernel mode
	if(address >= KERNEL_VIRTUAL_BASE)
	{
		Panic("MemIntUnmapUserPage: cannot unmap kernel pages");
	}

	//Only unmap if page table for address exists
	unsigned int addr = (unsigned int) address;
	MemPageDirectory * pDir = MemGetPageDirectory(context, addr);

	if(pDir->present)
	{
		//Get table entry
		MemPageTable * pTable = MemGetPageTable(pDir, addr);
		
		//Only unmap if present
		if(pTable->present)
		{
		    MemPhysPage page = pTable->pageID;

	        //Decrement counter
	        if(DecrementCounter(pDir))
	        {
	            //No more pages left in page table - we can destroy it!
	            MemPhysicalFree(pDir->pageID, 1);
	            pDir->rawValue = 0;
			}
			else
			{
		        //Unmap Page
			    unsigned int tmpCount = pTable->tableCount;
			    pTable->rawValue = 0;
			    pTable->tableCount = tmpCount;
	        }

			//Invalidate this entry
		    invlpg(address);
	        
	        return page;
		}
	}
	
	return INVALID_PAGE;
}
