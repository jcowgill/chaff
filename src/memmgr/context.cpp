/*
 * context.cpp
 *
 *  Created on: 14 Oct 2010
 *      Author: James
 */

#include "chaff.h"
#include "memmgr.h"

using namespace Chaff;
using namespace Chaff::MemMgr;

static char kernelContextData[sizeof(MContext)];
static PageDirectory kernelDirectoryData[256];
MContext * MContext::kernelContext;

//Initialises the kernel memory context
// PRE SLAB INIT - DO NOT USE NEW
void MContext::InitKernelContext()
{
	//Allocate directory
	kernelContext = new (kernelContextData) MContext();
	kernelContext->directory = kernelDirectoryData;

	//Setup basic pages
	kernelContext->directory[0].present = 1;
	kernelContext->directory[0].global = 1;
	kernelContext->directory[0].hugePage = 1;
	kernelContext->directory[0].writable = 1;

	kernelContext->directory[255].present = 1;
	kernelContext->directory[255].global = 1;
	kernelContext->directory[255].writable = 1;
	kernelContext->directory[255].pageID.writable = 1;


	//Upgrade version
	kernelContext->kernelVersion = 1;
}

//Creates a new blank memory context
MContext * MContext::InitBlank()
{
	//Create blank context
	MContext * newContext = new MContext();

	//Create directory
#warning The page directory must be page aligned
	newContext->directory = new PageDirectory[1024];

	//Setup with blank and kernel mode stuffs
	MemSet(newContext->directory, 0, 768 * sizeof(PageDirectory));
	MemCpy(newContext->directory + 768, kernelContext->directory, 256 * sizeof(PageDirectory));

	//Set kernel version
	newContext->kernelVersion = kernelContext->kernelVersion;

	return newContext;
}

//Clones this memory context
MContext * MContext::Clone()
{
	//
#warning TODO Copy On Write
	return NULL;
}

//Creates a new region of blank memory
MRegion * MContext::CreateRegion(void * startAddress, unsigned int length)
{
	//
}

//Creates a new memory mapped region
MRegion * MContext::CreateRegion(void * startAddress, unsigned int length, IO::FileHandle file,
		unsigned int fileOffset, unsigned int fileSize)
{
	//
}

//Finds the region which contains the given address
// or returns NULL if there isn't one
MRegion * MContext::FindRegion(void * address)
{
	//
}

//Removes the pages in the given region from this context and frees
// their physical memory
void MContext::Erase(void * startAddress, unsigned int length)
{
	//
}

//Switches to this memory context
void MContext::SwitchTo()
{
	//
}

//Deletes this memory context - DO NOT delete the memory context
// which is currently in use!
void MContext::Delete()
{
	//Check deletion of current context
}
