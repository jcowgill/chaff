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
// In the bitmap - 1 is allocated, 0 is not allocated

//Start of bitmap
#define bitmapStart (reinterpret_cast<unsigned int *>(KERNEL_VIRTUAL_BASE))
#define bitmapStartMain (reinterpret_cast<unsigned int *>(KERNEL_VIRTUAL_BASE) + (0x1000000 / 32))

//End of bitmap
static unsigned int * bitmapEnd;

//Next part of bitmap to allocate from
static unsigned int * bitmapHeadMain;
static unsigned int * bitmapHeadISA;

//Page counters
static unsigned int totalPages;
static unsigned int freePages;

//Fills a continuous region of the bitmap from startPage to endPage
// If update total is true, update the total otherwise, update the free memory
static void FillBitmap(PhysPage startPage, PhysPage endPage, bool allocate)
{
	//Ensure pages are in correct order
	if(startPage >= endPage)
	{
		return;
	}

	//Find start and end pointers
	unsigned int * startPtr = bitmapStart + (startPage / 32);
	unsigned int * endPtr = bitmapStart + (endPage / 32);

	//Construct bitmask for allocating at the start and freeing at the end
	unsigned int startAllocMask = 0xFFFFFFFF >> (startPage % 32);
	unsigned int endFreeMask = 0xFFFFFFFF >> (endPage % 32);

	//Handle same pointers
	if(startPtr == endPtr)
	{
		//Get new mask using XOR
		unsigned int mask = startAllocMask ^ endFreeMask;

		//Modify the bitmap
		if(allocate)
		{
			*startPtr |= mask;
		}
		else
		{
			*startPtr &= ~mask;
		}
	}
	else
	{
		//Modify start and end pointers
		if(allocate)
		{
			*startPtr |= startAllocMask;
			*endPtr |= ~endFreeMask;
		}
		else
		{
			*startPtr &= ~startAllocMask;
			*endPtr &= endFreeMask;
		}

		//Modify 4 byte sections in the middle
		for(++startPtr; startPtr < endPtr; ++startPtr)
		{
			*startPtr = allocate ? 0xFFFFFFFF : 0;
		}
	}
}

static inline void FillBitmapUpdTotal(PhysPage startPage, PhysPage endPage, bool allocate)
{
	FillBitmap(startPage, endPage, allocate);

	if(allocate)
	{
		totalPages -= endPage - startPage;
	}
	else
	{
		totalPages += endPage - startPage;
	}
}

static inline void FillBitmapUpdFree(PhysPage startPage, PhysPage endPage, bool allocate)
{
	FillBitmap(startPage, endPage, allocate);

	if(allocate)
	{
		freePages -= endPage - startPage;
	}
	else
	{
		freePages += endPage - startPage;
	}
}

//Checks weather all the pages in the given range are free
static inline bool RangeIsFree(PhysPage startPage, unsigned int length)
{
	//Check length
	if(length == 0)
	{
		return true;
	}

	//Get end page
	PhysPage endPage = startPage + length;

	//Get bitmap pointers
	unsigned int * startPtr = bitmapStart + (startPage / 32);
	unsigned int * endPtr = bitmapStart + (endPage / 32);

	//Construct bitmasks for checking
	unsigned int startMask = 0xFFFFFFFF >> (startPage % 32);
	unsigned int endMask = 0xFFFFFFFF >> (endPage % 32);

	//Handle differently if both are the same
	if(startPtr == endPtr)
	{
		//New mask is created using XOR
		startMask = startMask ^ endMask;

		//All pages in range must be 0
		return (*startPtr & startMask) == 0;
	}
	else
	{
		//Check first pages
		if((*startPtr & startMask) != 0)
		{
			//Not free!
			return false;
		}

		//Check middle pages
		for(++startPtr; startPtr < endPtr; ++startPtr)
		{
			//Must be 0
			if(*startPtr != 0)
			{
				return false;
			}
		}

		//Check last pages
		return (*endPtr & ~endMask) == 0;
	}
}

//Iterates over each mmap entry in <in> and stores them in <entryVar>
// The length of the memory map is passed in <len>
// <entryVar> is a multiboot_memory_map_t
// All other vars MUST be unsigned longs
#define MMAP_FOREACH(entryVar, in, len) \
	for(multiboot_memory_map_t * entryVar = reinterpret_cast<multiboot_memory_map_t *>(in); \
		reinterpret_cast<unsigned long>(entryVar) < (in) + (len); \
		entryVar = reinterpret_cast<multiboot_memory_map_t *>( \
			reinterpret_cast<unsigned long>(entryVar) + entryVar->size + sizeof(entryVar->size)))

//Initializes the physical memory manager using the memory info
// in the multiboot information structure
void PhysicalMgr::Init(multiboot_info_t * bootInfo)
{
	//Must have memory info avaliable
	if(bootInfo->flags & MULTIBOOT_INFO_MEM_MAP)
	{
		//Display info
		PrintLog(Info, "Reading physical memory map...\n");

		//1st Pass - find highest memory location to get size of bitmap
	    unsigned long highAddr = 0;
	    
		MMAP_FOREACH(mmapEntry, bootInfo->mmap_addr, bootInfo->mmap_length)
		{
		    //Only map available regions
		    if(mmapEntry->type == MULTIBOOT_MEMORY_AVAILABLE)
		    {
		        //Ignore regions starting in 64 bit
		        // + if region ends in 64 bit - use full highAddr
		        if((mmapEntry->addr >> 32) != 0)
		        {
		            continue;
				}
				else
				{
				    //Regions which end in 64 bits raise the address to the maximum
					if(mmapEntry->addr + mmapEntry->len >= 0xFFFFFFFF)
					{
						highAddr = 0xFFFFFFFF;
					}
					else
					{
						//Update only if this is higher
						if(static_cast<unsigned long>(mmapEntry->addr + mmapEntry->len) > highAddr)
						{
							highAddr = static_cast<unsigned long>(mmapEntry->addr + mmapEntry->len);
						}
					}
				}
			}
		}

		//Determine end location
		// 4 bytes stores 32 pages or 0x20000 data bytes
		// We round this up
		bitmapEnd = bitmapStart + ((highAddr + 0x1FFFF) / 0x20000);

		//Allocate everything
		MemSet(bitmapStart, 0xFF, bitmapEnd - bitmapStart);
		totalPages = (bitmapEnd - bitmapStart) * 8;

		//2nd pass - free available parts of memory
		MMAP_FOREACH(mmapEntry, bootInfo->mmap_addr, bootInfo->mmap_length)
		{
		    //Only free available regions
		    if(mmapEntry->type == MULTIBOOT_MEMORY_AVAILABLE && (mmapEntry->addr >> 32) == 0)
		    {
		    	//Check if region flows into 64 bit region
		    	if(mmapEntry->addr + mmapEntry->len > 0xFFFFFFFF)
		    	{
			    	FillBitmapUpdTotal((mmapEntry->addr + 4095) / 4096, 0x100000000ULL / 4096, false);
		    	}
		    	else
		    	{
					//Mark region as free
					FillBitmapUpdTotal((mmapEntry->addr + 4095) / 4096,
							(mmapEntry->addr + mmapEntry->len + 4095) / 4096, false);
		    	}
			}
		}

		//Reallocate already allocated stuff
		FillBitmapUpdTotal(0xA0, 0x100, true);	//ROM Area
		freePages = totalPages;
		FillBitmapUpdFree(0, ((bitmapEnd - bitmapStart) + 4095) / 4096, true);	//Bitmap
		FillBitmapUpdFree((unsigned int) KERNEL_VIRTUAL_BASE / 4096,
				((unsigned int) KERNEL_VIRTUAL_BASE / 4096) + 0x400, true);	//Kernel 4MB

		//Set heads to the beginning
		bitmapHeadISA = bitmapStart;
		if(bitmapEnd > bitmapStartMain)
		{
			bitmapHeadMain = bitmapStartMain;
		}
		else
		{
			bitmapHeadMain = NULL;
		}

		//Print info
		PrintLog(Info, " Total Memory = %uK", totalPages);
	}
	else
	{
		Panic("Memory information not given to OS");
	}
}

//Allocates physical pages
// If lower16Meg is true, only pages in the lower 16M of memory will be returned
PhysPage PhysicalMgr::AllocatePages(unsigned int number /* = 1 */)
{
	//Save head at beginning
	unsigned int * startHead = bitmapHeadMain;

	//Handle 0 page request
	if(number == 0)
	{
		PrintLog(Error, "PhysicalMgr::AllocatePages Request for 0 pages");
		return -1;
	}

	//Go though bitmap to find a page starting with the head
	do
	{
		//Ensure head is in the bitmap
		if(bitmapHeadMain >= bitmapEnd)
		{
			//Restore to beginning
			bitmapHeadMain = bitmapStartMain;
		}

		//Check if there is a free page
		if(*bitmapHeadMain != 0xFFFFFFFF)
		{
			unsigned int firstFree;

			//If blank, use first page
			if(*bitmapHeadMain == 0)
			{
				firstFree = 0;
			}
			else
			{
				//Yes - somewhere...
				firstFree = BitScanReverse(~*bitmapHeadMain);
			}

			unsigned int thisPage = (bitmapHeadMain - bitmapStart) * 32 + firstFree;

			//Check subsequent pages
			if(RangeIsFree(thisPage, number - 1))
			{
				//Set allocated
				FillBitmapUpdFree(thisPage, number, true);

				//Return page
				return thisPage;
			}
		}

		//Move on head
		++bitmapHeadMain;
	}
	while(bitmapHeadMain != startHead);

	//Out of main memory - use ISA region
	return AllocateISAPages(number);
}

//Allocates physical pages
// This function will never return any page with an offset > 16M
PhysPage PhysicalMgr::AllocateISAPages(unsigned int number /* = 1 */)
{
	//Save head at beginning
	unsigned int * startHead = bitmapHeadISA;

	//Handle 0 page request
	if(number == 0)
	{
		PrintLog(Error, "PhysicalMgr::AllocateISAPages Request for 0 pages");
		return -1;
	}

	//Go though bitmap to find a page starting with the head
	do
	{
		//Ensure head is in the bitmap
		if(bitmapHeadISA >= bitmapStartMain || bitmapHeadISA >= bitmapEnd)
		{
			//Restore to beginning
			bitmapHeadISA = bitmapStart;
		}

		//Check if there is a free page
		if(*bitmapHeadISA != 0xFFFFFFFF)
		{
			unsigned int firstFree;

			//If blank, use first page
			if(*bitmapHeadISA == 0)
			{
				firstFree = 0;
			}
			else
			{
				//Yes - somewhere...
				firstFree = BitScanReverse(~*bitmapHeadISA);
			}

			unsigned int thisPage = (bitmapHeadISA - bitmapStart) * 32 + firstFree;

			//Check subsequent pages
			if(RangeIsFree(thisPage, number - 1))
			{
				//Set allocated
				FillBitmapUpdFree(thisPage, number, true);

				//Return page
				return thisPage;
			}
		}

		//Move on head
		++bitmapHeadISA;
	}
	while(bitmapHeadISA != startHead);

	//Out of memory!
	Panic("Out of memory");
}

//Frees 1 physical page allocated by AllocatePage
void PhysicalMgr::FreePages(PhysPage page, unsigned int number /* = 1 */)
{
	//Free from bitmap
	for(; number > 0; --number, ++page)
	{
		*(bitmapStart + (page / 32)) &= ~(0x80000000 >> (page % 32));
		++freePages;
	}
}

//Returns the number of pages in memory
unsigned int PhysicalMgr::GetTotalPages()
{
	return totalPages;
}

//Returns the number of free pages available
unsigned int PhysicalMgr::GetFreePages()
{
	return freePages;
}
