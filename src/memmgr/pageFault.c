/*
 * pageFault.c
 *
 *  Created on: 10 Dec 2010
 *      Author: James
 */

#include "chaff.h"
#include "memmgr.h"
#include "memmgrint.h"
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
					for(int i = 0; i < 1024; ++i)
					{
						*basePageAddr++ = 0;
					}
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
		if(faultAddress < 4096)
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
