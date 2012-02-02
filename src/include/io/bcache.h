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
#include "waitqueue.h"

struct IoDevice;

//State of IoBlocks
typedef enum
{
	//Block is in memory
	IO_BLOCK_OK,

	//Block is being read by another thread (partially read)
	IO_BLOCK_READING,

	//Block is being written by another thread (partially written)
	IO_BLOCK_WRITING,

	//I/O error encounted while accessing block
	// The block is removed from the cache for new requests immediately,
	// existing requests may see this
	IO_BLOCK_ERROR,

} IoBlockState;

//The structure containing information about a block
typedef struct IoBlock
{
	//Block offset
	unsigned long long offset;

	//Block collection references
	ListHead listItem;
	HashItem hItem;

	//Concurrent operation structures
	IoBlockState state;
	ProcWaitQueue waitingThreads;		//Threads waiting for this block to be read
										// This includes threads waiting to write

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
	unsigned int blockSize;

	//Hashtable of blocks (used for lookups)
	HashTable blockTable;

	//List of blocks (used for removing all at end)
	ListHead blockList;

} IoBlockCache;

//Initializes a block cache
// Do not call on an alredy initialized cache
void IoBlockCacheInit(IoBlockCache * cache, int blockSize);

//Destroys a block cache
// Returns false if some blocks are still lockeds
bool IoBlockCacheEmpty(IoBlockCache * cache);

//Reads a block of data from the block cache or reads it from the disk if it isn't there
// Returns a negative value on error or the number of bytes you can read from the block
// A single block is stored in a contingous location in memory but multiple blocks are not
// Block returned by this function are locked in memory until IoBlockCacheUnlock is called
//  Be aware that blocks in the state IO_BLOCK_ERROR which are locked will prevent any
//  sucessful reads or writes until the block is unlocked.
int IoBlockCacheRead(struct IoDevice * device, unsigned long long off, IoBlock ** block);

//Decrements the reference count on a block from the cache
void IoBlockCacheUnlock(struct IoDevice * device, IoBlock * block);

//Uses the block cache to read / copy data into a buffer
// Returns a negative value on error or the number of bytes actually read
// Nothing needs to be unlocked after this
int IoBlockCacheReadBuffer(struct IoDevice * device, unsigned long long off,
		void * buffer, unsigned int length);

//Writes data to disk and to the block cache
// Returns a negative value on error or the number of bytes actually written
// (currently the block cache is always write-through)
int IoBlockCacheWriteBuffer(struct IoDevice * device, unsigned long long off,
		void * buffer, unsigned int length);

#endif /* BCACHE_H_ */
