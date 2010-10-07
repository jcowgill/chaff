/*
 * physicalMgr.cpp
 *
 *  Created on: 6 Oct 2010
 *      Author: James
 */

#include "chaff.h"
#include "memmgr.h"

using namespace Chaff;
using namespace Chaff::MemMgr;

//Bitmap pointers
static unsigned int * bitmapStart;
static unsigned int * bitmapEnd;

static unsigned int * bitmapHead;

//Initializes the physical memory manager using the memory info
// in the multiboot information structure
void PhysicalMgr::Init(multiboot_info_t * bootInfo)
{
	//Get memory map pointer
	unsigned int * mmapPtr = reinterpret_cast<unsigned int *>(bootInfo->mmap_addr);

	//1st Pass - find highest memory location to get size of bitmap
	unsigned int * mmapEntry = mmapPtr;
	for(int i = 0; i < bootInfo->mmap_length; ++i)
	{
		//Move on entry
		mmapEntry = mmapEntry + mmapEntry[-1];
	}
}

//Allocates physical pages
// If lower16Meg is true, only pages in the lower 16M of memory will be returned
PhysPage PhysicalMgr::AllocatePages(unsigned int number /* = 1 */, bool lower16Meg /* = false */)
{
	//
}

//Frees 1 physical page allocated by AllocatePage
void PhysicalMgr::FreePages(PhysPage page, unsigned int number /* = 1 */)
{
	//
}
