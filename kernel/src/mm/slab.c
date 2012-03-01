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

//Constructor for rootCache
static void RootCacheConstruct(void * object, struct MemCache * cache);

//Cache which stores cache objects
// Also acts as the root cache for traversing the cache list
static MemCache rootCache =
{
		.cacheList = LIST_INLINE_INIT(rootCache.cacheList),

		.slabsFull = LIST_INLINE_INIT(rootCache.slabsFull),
		.slabsPartial = LIST_INLINE_INIT(rootCache.slabsPartial),
		.slabsEmpty = LIST_INLINE_INIT(rootCache.slabsEmpty),

		.ctor = RootCacheConstruct,
		.dtor = NULL,

		.objectSize = sizeof(MemCache),
		.flags = 0,
};

//Creates a new slab cache
MemCache * MemSlabCreate(unsigned int size, unsigned int flags,
		MemSlabCallback ctor, MemSlabCallback dtor)
{
	//Allocate new object
	MemCache * cache = MemSlabAlloc(&rootCache);

	if(cache)
	{
		//Setup cache info
		cache->ctor = ctor;
		cache->dtor = dtor;
		cache->objectSize = size;
		cache->flags = flags;

#warning TODO calculate other fields

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

//Allocates an object from a slab cache
void * MemSlabAlloc(MemCache * cache)
{
	//
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
	//
}

//Frees all the slabs in the cache which are empty
int MemSlabShrink(MemCache * cache)
{
	//
}

static void RootCacheConstruct(void * object, struct MemCache * cache)
{
	//Setup object
	MemCache * newCache = object;

	ListHeadInit(&newCache->cacheList);
	ListHeadInit(&newCache->slabsFull);
	ListHeadInit(&newCache->slabsPartial);
	ListHeadInit(&newCache->slabsEmpty);
}
