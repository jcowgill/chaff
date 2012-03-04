/*
 * slab.c
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
 *  Created on: 29 Feb 2012
 *      Author: James
 */

#include "chaff.h"
#include "list.h"
#include "mm/physical.h"
#include "mm/kmemory.h"

//Cache which stores cache objects
static MemCache rootCache =
{
		.cacheList = LIST_INLINE_INIT(rootCache.cacheList),

		.slabsFull = LIST_INLINE_INIT(rootCache.slabsFull),
		.slabsPartial = LIST_INLINE_INIT(rootCache.slabsPartial),
		.slabsEmpty = LIST_INLINE_INIT(rootCache.slabsEmpty),

		.objectSize = sizeof(MemCache),
		.flags = 0,
		.pagesPerSlab = 1,
		.objectsPerSlab = (PAGE_SIZE - sizeof(MemSlab)) / sizeof(MemCache),
};

//Slab cache storing slab headers for large slabs
static MemCache * rootSlabCache;
static MemCache * kAllocCache[11];

//Initializes the slab allocator
void INIT MemSlabInit()
{
	//Slab header cache
	rootSlabCache = MemSlabCreate(sizeof(MemSlab), 0);

	//MemKAlloc caches
	for(int i = 0; i < 11; i++)
	{
		kAllocCache[i] = MemSlabCreate(8 << i, 0);
	}
}

//Allocate generic memory
void * MemKAlloc(unsigned int bytes)
{
	//Check size
	if(bytes == 0 || bytes > 8 << 10)
	{
		PrintLog(Error, "MemKAlloc: Allocation too large or 0 bytes");
		return NULL;
	}

	unsigned int cacheIndex = 0;

	//Find best cache
	if(bytes > 8)
	{
		cacheIndex = 31 - BitScanForward(bytes);

		//Add 1 of not exactly a power of 2
		if(1 << cacheIndex != bytes)
		{
			cacheIndex++;
		}
	}

	//Allocate using cache
	return MemSlabAlloc(kAllocCache[cacheIndex]);
}

//Allocate and zero memory
void * MemKZAlloc(unsigned int bytes)
{
	void * data = MemKAlloc(bytes);

	//Wipe data
	if(data)
	{
		MemSet(data, 0, bytes);
	}

	return data;
}

//Free MemKAlloc memory
void MemKFree(void * ptr)
{
	//Get slab
	MemPhysPage page = MemVirt2Phys(ptr);
	MemSlab * slab = MemPageStateTable[page].slab;

	if(!slab)
	{
		PrintLog(Error, "MemKFree: invalid pointer (no associated slab)");
		return;
	}

	//Get cache
	MemCache * cache = slab->cache;

	//Free pointer
	MemSlabFree(cache, ptr);
}

//Creates a new slab cache
MemCache * MemSlabCreate(unsigned int size, unsigned int flags)
{
	//Align size
	size = (size + 3) & ~3;

	//Check usage
	if(size >= 8 * PAGE_SIZE)
	{
		//Not that large
		PrintLog(Error, "MemSlabCreate: Cannot create slab with objects over 32KB");
		return NULL;
	}

	//Allocate new object
	MemCache * cache = MemSlabAlloc(&rootCache);

	if(cache)
	{
		//Initialize cache object
		ListHeadInit(&cache->cacheList);
		ListHeadInit(&cache->slabsFull);
		ListHeadInit(&cache->slabsPartial);
		ListHeadInit(&cache->slabsEmpty);

		//Do cache setup
		// Using large slabs?
		unsigned int slabExtra;

		if(size >= (PAGE_SIZE >> 3))
		{
			flags |= MEM_SLAB_LARGE;
			slabExtra = 0;
		}
		else
		{
			flags &= ~MEM_SLAB_LARGE;
			slabExtra = sizeof(MemSlab);
		}

		// Determine best number of pages per slab
		unsigned int bestPages = 8;
		unsigned int bestWastage = ~0;

		for(unsigned int pages = 1; pages < 8; pages++)
		{
			//Calculate wastage
			unsigned int nrObjects = ((pages * PAGE_SIZE) - slabExtra) / size;
			unsigned int wastage = ((pages * PAGE_SIZE) - slabExtra) % size;

			//Enough objects?
			if(nrObjects < 8)
			{
				continue;
			}

			//Allow this number if a certain wastage is obtained
			if(wastage <= pages * 128)
			{
				bestPages = pages;
				break;
			}

			//Better than before?
			if(wastage < bestWastage)
			{
				bestWastage = wastage;
				bestPages = pages;
			}
		}

		//Store details
		cache->objectSize = size;
		cache->flags = flags;
		cache->pagesPerSlab = bestPages;
		cache->objectsPerSlab = ((bestPages * PAGE_SIZE) - slabExtra) / size;

		//Add to cache chain
		ListHeadAddLast(&cache->cacheList, &rootCache.cacheList);
	}

	return cache;
}

//Destroys a slab cache
bool MemSlabDestroy(MemCache * cache)
{
	//Ensure there are no full or partial slabs
	if(!ListEmpty(&cache->slabsFull) || !ListEmpty(&cache->slabsPartial))
	{
		//Cache in use
		return false;
	}

	//Free all empty slabs
	MemSlabShrink(cache);

	//Remove from cache chain
	ListDeleteInit(&cache->cacheList);

	//Free memory
	MemSlabFree(&rootCache, cache);
	return true;
}

//Creates a new empty slab in a cache
static inline MemSlab * CreateSlab(MemCache * cache)
{
	//Create slab
	int zone = cache->flags & MEM_SLAB_DMA ? MEM_DMA : MEM_KERNEL;
	MemPhysPage rawData = MemPhysicalAlloc(cache->pagesPerSlab, zone);

	//Get slab header location
	MemSlab * slab;

	if(cache->flags & MEM_SLAB_LARGE)
	{
		//Allocate separately
		slab = MemSlabAlloc(rootSlabCache);
	}
	else
	{
		//Use end of slab
		slab = MemPhys2Virt(rawData + cache->pagesPerSlab);
		slab--;
	}

	//Set slab for each page
	MemPhysPage endPage = rawData + cache->pagesPerSlab;
	for(MemPhysPage page = rawData; page < endPage; ++page)
	{
		MemPageStateTable[page].slab = slab;
	}

	//Setup free chain
	unsigned int dataPtr = (unsigned int) MemPhys2Virt(rawData);
	for(unsigned int i = 1; i < cache->objectsPerSlab; i++)
	{
		unsigned int newDataPtr = dataPtr + cache->objectSize;

		//Set next free object
		// This is 1 more than the obj
		*((unsigned int *) dataPtr) = newDataPtr;

		//Advance data pointer
		dataPtr = newDataPtr;
	}

	//Add terminator
	*((unsigned int *) dataPtr) = MEM_SLAB_END;

	//Setup slab info
	slab->cache = cache;
	ListHeadInit(&slab->slabList);
	slab->memory = rawData;
	slab->freePtr = 0;
	slab->activeObjs = 0;

	//Add to cache lists
	ListHeadAddLast(&slab->slabList, &cache->slabsEmpty);

	return slab;
}

//Allocates an object from a slab cache
void * MemSlabAlloc(MemCache * cache)
{
	MemSlab * slab;

	//Find slab to allocate from
	if(!ListEmpty(&cache->slabsPartial))
	{
		slab = ListEntry(cache->slabsPartial.next, MemSlab, slabList);
	}
	else if(!ListEmpty(&cache->slabsEmpty))
	{
		slab = ListEntry(cache->slabsEmpty.next, MemSlab, slabList);
	}
	else
	{
		//Create a new slab if there are none left
		slab = CreateSlab(cache);
	}

	//Allocate from chosen slab
	// Update free pointer
	unsigned int * objectPtr = slab->freePtr;
	slab->freePtr = (unsigned int *) *objectPtr;

	// Update active objects
	slab->activeObjs++;

	// Moving lists?
	if(*objectPtr == MEM_SLAB_END)
	{
		//Move slab to full list
		ListDeleteInit(&slab->slabList);
		ListAddBefore(&slab->slabList, &cache->slabsFull);
	}
	else if(slab->activeObjs == 1)
	{
		//Move slab to partial list
		ListDeleteInit(&slab->slabList);
		ListAddBefore(&slab->slabList, &cache->slabsPartial);
	}

	//Return new object
	return objectPtr;
}

//Allocates and zeroes out an object from the slab cache
void * MemSlabZAlloc(MemCache * cache)
{
	//Allocate object
	void * object = MemSlabAlloc(cache);

	//Zero out
	if(object)
	{
		MemSet(object, 0, cache->objectSize);
	}

	return object;
}

//Frees an object allocated by MemSlabAlloc() after it has been used
void MemSlabFree(MemCache * cache, void * ptr)
{
	//Validate pointer
	if(ptr < KERNEL_VIRTUAL_BASE || ptr >= (void *) MEM_KFIXED_MAX)
	{
		PrintLog(Error, "MemSlabFree: invalid pointer given");
		return;
	}

	//Lookup slab this object is in
	MemPhysPage page = MemVirt2Phys(ptr);
	MemSlab * slab = MemPageStateTable[page].slab;

	//Ensure slab uses the same cache
	if(slab->cache != cache)
	{
		PrintLog(Error, "MemSlabFree: pointer given does not belong to the given slab cache");
		return;
	}

	//Update free pointer
	*((unsigned int **) ptr) = slab->freePtr;
	slab->freePtr = ptr;

	//Moving lists?
	if(slab->activeObjs == cache->objectsPerSlab)
	{
		//Move slab to partial list
		ListDeleteInit(&slab->slabList);
		ListAddBefore(&slab->slabList, &cache->slabsPartial);
	}
	else if(slab->activeObjs == 1)
	{
		//Move slab to empty list
		ListDeleteInit(&slab->slabList);
		ListAddBefore(&slab->slabList, &cache->slabsEmpty);
	}

	//Update active objects
	slab->activeObjs--;
}

//Frees all the slabs in the cache which are empty
int MemSlabShrink(MemCache * cache)
{
	int pagesFreed = 0;
	MemSlab * slab, * tmpSlab;

	ListForEachEntrySafe(slab, tmpSlab, &cache->slabsEmpty, slabList)
	{
		//Free memory associated with slab
		MemPhysicalFree(slab->memory, cache->pagesPerSlab);

		//If header is stored off slab, free it as well
		if(cache->flags & MEM_SLAB_LARGE)
		{
			MemSlabFree(rootSlabCache, slab);
		}

		//Add pages freed
		pagesFreed += cache->pagesPerSlab;
	}

	return pagesFreed;
}
