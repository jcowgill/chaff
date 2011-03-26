/*
 * pageFault.c
 *
 *  Created on: 10 Dec 2010
 *      Author: James
 */

#include "chaff.h"
#include "memmgr.h"
#include "memmgrInt.h"
#include "inlineasm.h"
#include "process.h"

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
	MemContext * context = ProcCurrProcess->memContext;
	MemRegion * region = MemRegionFind(context, faultAddress);

	//If null, out of valid area
	if(region != NULL)
	{
		//Check fault cause
		if(errorCode & (1 << 0))
		{
			//Protection violation (user mode accessed supervisor)
			// Or a write to a read-only page

			PageTable * table = THIS_PAGE_TABLES + ((unsigned int) faultAddress >> 12);

			//Check if copy-on-write
			if(!(table->writable) && (region->flags & MEM_WRITABLE))
			{
				//Get page ref count
				unsigned int * refCount = MemIntPhysicalRefCount(table->pageID);

				if(*refCount != 1)
				{
					//Duplicate page first
					unsigned int * basePageAddr = (unsigned int *) ((unsigned int) faultAddress & 0xFFFFF000);
					PhysPage newPage = MemPhysicalAlloc(1);

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
				MemIntMapPage(context, basePageAddr, MemPhysicalAlloc(1), region->flags);

				//Read page worth of data
				if(region->flags & MEM_MAPPEDFILE)
				{
#warning After mapping page, read data if memory mapped
					//Ensure ALL the page is replaced
				}
				else
				{
					//Wipe page
					MemSet(basePageAddr, 0, 4096);
				}

				return;
			}
		}
	}

	//If we're here, it's an error
	if(errorCode & (1 << 2))
	{
		//User mode fault
		ProcSignalSendThread(ProcCurrThread, SIGSEGV);
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
