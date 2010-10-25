/*
 * context.cpp
 *
 *  Created on: 14 Oct 2010
 *      Author: James
 */

#include "chaff.h"
#include "memmgr.h"

//Kernel page directory data
PageDirectory kernelPageDirectory[1024] __attribute__((aligned(4096)));
PageTable kernelPageTable255[1024] __attribute__((aligned(4096)));

//Kernel context
MemContext MemKernelContextData = { LIST_SINIT(MemKernelContextData.regions), 0, INVALID_PAGE};

//Creates a new blank memory context
MemContext * MemContextInit()
{
#if 0
	//Allocate new context
	MemContext * newContext = MAlloc(sizeof(MemContext));

	//Allocate directory
#warning This must be page aligned (idea - use manaul page alloc and temp map it to an address)
	newContext->directory = MAlloc(sizeof(PageDirectory) * 1024);

	//Wipe user area
	MemSet(newContext->directory, 0, sizeof(PageDirectory) * 768);

	//Copy kernel area
	MemCpy(newContext->directory + 768, kernelPageDirectory, sizeof(PageDirectory) * 255);
	newContext->kernelVersion = MemKernelContext->kernelVersion;

	//Setup final page
	newContext->directory[1023] = 0;
	newContext->directory[1023].present = 1;
	newContext->directory[1023].writable = 1;
	newContext->directory[1023].pageID = newContext->directory;
	
	//Return context
	return newContext;
#endif
}
