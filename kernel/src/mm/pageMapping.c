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

//Page table for tmp pages
MemPageTable kernelPageTable253[1024] __attribute__((aligned(4096)));

//Increments the counter for the page directory containing this page table
static void IncrementCounter(MemPageTable * table)
{
	//Get first page table in directory
	MemPageTable * firstTable = (MemPageTable *) ((unsigned int) table & 0xFFFFF000);
			
	//Increment from least significant to most significant
	for(;;)
	{
	    //Increment, and it it's too much, go to next one
	    if(firstTable->tableCount++ != 7)
	    {
	        //Finished, exit
	        return;
		}
	    
	    ++firstTable;
	}
}

//Decrements the counter for the page directory containing this page table
// Returns true if no pages left
static bool DecrementCounter(MemPageTable * table)
{
	//Get first page table in directory
	MemPageTable * firstTable = (MemPageTable *) ((unsigned int) table & 0xFFFFF000);

	//Handle first iteration separately
	if(firstTable->tableCount-- > 1)
	{
		return false;
	}

	//Increment from second to least significant to most significant
	for(int i = 1; i < 5; ++i)
	{
	    ++firstTable;
	    
	    //Decrement, if it was not 0 then go to next one
		if(firstTable->tableCount-- != 0)
		{
		    return false;
		}
	}
	
	//If we're here, no pages in table
	return true;
}

void MemIntMapPage(void * address, MemPhysPage page, MemRegionFlags flags)
{
	//Ignore request if no readable, writable or executable flags
	if((flags & (MEM_READABLE | MEM_WRITABLE | MEM_EXECUTABLE)) == 0)
	{
	    return;
	}

	//Ensure page table for address exists
	unsigned int addr = (unsigned int) address;
	MemPageDirectory * pDir = THIS_PAGE_DIRECTORY + (addr >> 22);

	if(!pDir->present)
	{
		//Create page table
		pDir->pageID = MemPhysicalAlloc(1, MEM_KERNEL);
		pDir->writable = 1;

		if(addr < 0xC0000000)
		{
			pDir->userMode = 1;
		}
		else
		{
			pDir->global = 1;		//For when this is interpreted as a page table
		}
		
		pDir->present = 1;
		
		//Wipe page
		MemSet((void *) ((unsigned int) (THIS_PAGE_TABLES + (addr >> 12)) & 0xFFFFF000), 0, 4096);
		
		//If kernel mode, update version and copy entry
		if(addr >= 0xC0000000)
		{
			MemCurrentContext->kernelVersion = ++MemKernelContext->kernelVersion;
            kernelPageDirectory[addr >> 22].rawValue = pDir->rawValue;
		}
	}

	//Get table entry
	MemPageTable * pTable = THIS_PAGE_TABLES + (addr >> 12);

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
		IncrementCounter(pTable);
	}
	
	//Set properties
	pTable->pageID = page;

	//Page is writable
	pTable->writable = (flags & MEM_WRITABLE) ? 1 : 0;

	//Disable cache
	pTable->cacheDisable = (flags & MEM_CACHEDISABLE) ? 1 : 0;

	if(addr < 0xC0000000)
	{
	    //Page is user-mode
	    pTable->userMode = 1;
	}
	else
	{
	    //Kernel mode pages are global
	    pTable->global = 1;
	}
	
	//Page is present
	pTable->present = 1;
}

//Unmaps a page and returns the page which was unmapped
MemPhysPage UnmapPage(void * address)
{
	//Only unmap if page table for address exists
	unsigned int addr = (unsigned int) address;
	MemPageDirectory * pDir = THIS_PAGE_DIRECTORY + (addr >> 22);

	if(pDir->present)
	{
		//Get table entry
		MemPageTable * pTable = THIS_PAGE_TABLES + (addr >> 12);
		
		//Only unmap if present
		if(pTable->present)
		{
		    MemPhysPage page = pTable->pageID;

	        //Decrement counter
	        if(DecrementCounter(pTable))
	        {
	            //No more pages left in page table - we can destroy it!
	            MemPhysicalFree(pDir->pageID, 1);
	            pDir->rawValue = 0;
	            
	            //If kernel mode, update version
				if(addr >= 0xC0000000)
				{
					MemCurrentContext->kernelVersion = ++MemKernelContext->kernelVersion;
		            kernelPageDirectory[addr >> 22].rawValue = 0;
				}
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

void MemIntUnmapPage(void * address)
{
	UnmapPage(address);
}

void MemIntUnmapPageAndFree(void * address)
{
	MemPhysPage page = UnmapPage(address);
	
	if(page != INVALID_PAGE)
	{
	    MemPhysicalDeleteRef(page, 1);
	}
}

void MemIntMapTmpPage(void * address, MemPhysPage page)
{
	//Map page (very simple)
	unsigned int addr = (unsigned int) address;

	if(addr < 0xFF400000 || addr >= 0xFF800000)
	{
		Panic("MemIntMapTmpPage: can only map pages in the temporary page zone");
	}

	//Get page table
	MemPageTable * pTable = THIS_PAGE_TABLES + (addr >> 12);

	//Overwrite?
	if(pTable->present)
	{
		PrintLog(Warning, "MemIntMapTmpPage: Overwriting temporary page table entry");
	}

	//Set properties
	pTable->rawValue = 0;
	pTable->present = 1;
	pTable->global = 1;
	pTable->writable = 1;
	pTable->pageID = page;

	//Invalidate page
	invlpg(address);
}

void MemIntUnmapTmpPage(void * address)
{
	//Unmap page (very simple)
	unsigned int addr = (unsigned int) address;

	if(addr < 0xFF400000 || addr >= 0xFF800000)
	{
		Panic("MemIntUnmapTmpPage: can only unmap pages in the temporary page zone");
	}

	//Get page table
	MemPageTable * pTable = THIS_PAGE_TABLES + (addr >> 12);

	//Set properties
	pTable->rawValue = 0;

	//Invalidate page
	invlpg(address);
}
