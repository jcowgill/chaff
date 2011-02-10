/*
 * malloc.c
 *
 *  Created on: 23 Dec 2010
 *      Author: James
 */

#include "chaff.h"
#include "memmgr.h"
#include "memmgrInt.h"

//Simple Malloc and Free implementation
// This is a simple heap allocator - probably be improved in the future
//

#define HEAP_START ((void *) 0xD0000000)
#define HEAP_EXTRAALOC 0x2000		//8 KB - amount to allocate when run out of memory

//The 8-byte header located before every allocations
typedef struct s_mHeader
{
	struct s_mHeader * ptr;	//next block if on free list
	unsigned int size;		//size of this block (in nunits, not bytes)

} mHeader;

static mHeader base = {&base, 0};	//Start list is 0 bytes and points to self
static mHeader * freep = &base;		//Base is the first start of the free list

//Heap Region
static MemRegion heapRegion;

//Initializes the memory allocator regions
void MAllocInit()
{
	//Initialize heap
	MemIntRegionCreate(MemKernelContext, HEAP_START, 0, MEM_READABLE | MEM_WRITABLE, &heapRegion);
}

//rawAlloc is used internally to allocate more memory for kMalloc from the system
// new memory is always committed
static mHeader * rawAlloc(unsigned int nunits)
{
	//Round up bytes
	unsigned int bytes = ((nunits * sizeof(mHeader)) + HEAP_EXTRAALOC - 1) & ~(HEAP_EXTRAALOC - 1);

	//Get pointer to memory
	mHeader * memory = (mHeader *) (heapRegion.length + (unsigned int) HEAP_START);

	//Resize region
	MemRegionResize(&heapRegion, heapRegion.length + bytes);

	//Set memory size (this will commit the first page also)
	memory->size = bytes / sizeof(mHeader);

	//Add block to frees
	MFree(memory + 1);
	return freep;
}

//Allocates a block of memory of the given size
// Memory is allocated to 4 BYTE ALIGNMENT (ie not 8)
// Size is rounded up to the nearest 4 bytes
void * MAlloc(unsigned int size)
{
	mHeader * prevp;
	mHeader * cBlock;
	unsigned int nunits;
	void * retVal;

	//Check for 0 size
	if(size == 0)
	{
		PrintLog(Warning, "MAlloc: attempt to allocate 0 bytes");
		return NULL;
	}

	//Convert size to nunits (1 nunit = 8 bytes, rounding up)
	// also, allocate 1 extra unit for the header
	nunits = ((size + sizeof(mHeader) - 1) / sizeof(mHeader)) + 1;

	//Search through the free pointer blocks for a block big enough
	prevp = freep;
	cBlock = prevp->ptr;

	for(;;)
	{
		//Block big enough?
		if(cBlock->size >= nunits)
		{
			//Does the block fit exactly?
			if(cBlock->size == nunits)
			{
				//Yes, we remove the block from the linked list
				prevp->ptr = cBlock->ptr;
			}
			else
			{
				//No, we shorten the size of the block
				// (we use the memory at the end)
				cBlock->size -= nunits;

				//Change cBlock to our new block and set it's size
				cBlock += cBlock->size;
				cBlock->size = nunits;
			}

			//Set next pointer to start at
			freep = prevp;

			//Return actual memory
			retVal = cBlock + 1;
			break;
		}
		else if(cBlock == freep)
		{
			//We've wrapped around the list and checked all free areas
			// Allocate more memory from the system
			cBlock = rawAlloc(nunits);
			if(cBlock == NULL)
			{
				//Out of memory
				retVal = NULL;
				break;
			}
		}

		//Move to next free block
		prevp = cBlock;
		cBlock = cBlock->ptr;
	}

	return retVal;
}

//Frees the given block of memory previously allocated with MAlloc
void MFree(void * ptr)
{
	mHeader * dataHeader = ((mHeader *) ptr) - 1;
	mHeader * cBlock;

#ifdef DEBUG
	//Wipe data with 0xFE for debugging
	MemSet(ptr, 0xFE, dataHeader->size * sizeof(mHeader));
#endif

	//Find position to place the freed block
	for(cBlock = freep; ; cBlock = cBlock->ptr)
	{
		if((dataHeader > cBlock && dataHeader < cBlock->ptr) ||
				//We're after the current block but before the next block
			(cBlock >= cBlock->ptr &&
				(dataHeader > cBlock || dataHeader < cBlock->ptr)))
				//About to wrap around, we're after the end or before the start
		{
			//Found position, insert after cBlock
			break;
		}
	}

	//Join with surrounding free blocks. Collapse if possible

	// Set our next block pointer
	if(dataHeader + dataHeader->size == cBlock->ptr)
	{
		//There is a free block immediately after this one
		// We extend this block to cover the next region
		dataHeader->size += cBlock->ptr->size;
		dataHeader->ptr = cBlock->ptr->ptr;
	}
	else
	{
		//Used space is after this block, the cBlock contains the pointer
		// to the next free block.
		dataHeader->ptr = cBlock->ptr;
	}

	// Update previous block's pointer
	if(cBlock + cBlock->size == dataHeader)
	{
		//There is a free block before this one
		// The previous block engulfs this block
		cBlock->size += dataHeader->size;
		cBlock->ptr = dataHeader->ptr;
	}
	else
	{
		//There is used space before this block, set cBlock's pointer
		// to this block
		cBlock->ptr = dataHeader;
	}

	//Update global free pointer
	freep = dataHeader;
}
