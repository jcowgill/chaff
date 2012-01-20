/*
 * bcache.h
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
 *  Created on: 14 Jan 2012
 *      Author: james
 */

#ifndef BCACHE_H_
#define BCACHE_H_

//Block Cache functions
//

#include "chaff.h"
#include "list.h"
#include "htable.h"

struct IoDevice;

//The structure containing information about a block
typedef struct
{
	//Block offset
	unsigned long long offset;

	//Block collection references
	ListHead listItem;
	HashItem hItem;

	//Number of users of this block
	// The block can only be removed (or moved) from memory if this is 0
	unsigned int refCount;

	//Address of block data
	char * address;

} IoBlock;

//The cache of blocks associated with a device
typedef struct IoBlockCache
{
	//Size of blocks in cache
	int blockSize;

	//Hashtable of blocks (used for lookups)
	HashTable blockTable;

	//List of blocks (used for removing all at end)
	ListHead blockList;

} IoBlockCache;

//Initializes a block cache
// Do not call on an alredy initialized cache
void IoBlockCacheInit(IoBlockCache * cache, int blockSize);

//Destroys a block cache
void IoBlockCacheEmpty(IoBlockCache * cache);

//Reads a block of data from the block cache or reads it from the disk if it isn't there
// Returns a negative value on error or the number of bytes you can read from the block
// A single block is stored in a contingous location in memory but multiple blocks are not
// The returned block pointer locked until IoBlockCacheUnlock is called
int IoBlockCacheRead(struct IoDevice * device, unsigned long long off, IoBlock ** block);

//Decrements the reference count on a block from the cache
void IoBlockCacheUnlock(struct IoDevice * device, IoBlock * block);

//Uses the block cache to read /copy data into a buffer
// Returns a negative value on error or the number of bytes actually read
// Nothing needs to be unlocked after this
int IoBlockCacheReadBuffer(struct IoDevice * device, unsigned long long off,
		void * buffer, unsigned int length);

//Writes data to disk and to the block cache
// Returns a negative value on error or the number of bytes actually written
// (currently the block cache is always write-through)
int IoBlockCacheWrite(struct IoDevice * device, unsigned long long off, void * buffer, unsigned int length);

#endif /* BCACHE_H_ */
