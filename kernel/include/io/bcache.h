/**
 * @file
 * Block cache functions
 *
 * @date January 2012
 * @author James Cowgill
 */

/*
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
 */

#ifndef BCACHE_H_
#define BCACHE_H_

#include "chaff.h"
#include "list.h"
#include "htable.h"
#include "waitqueue.h"

struct IoDevice;

/**
 * State a block is in
 */
typedef enum
{
	/// Block is in memory and is not being written
	IO_BLOCK_OK,

	///Block is being read by another thread (partially read)
	IO_BLOCK_READING,

	///Block is being written by another thread (partially written)
	IO_BLOCK_WRITING,

	/**
	 * I/O error encounted while accessing block
	 *
	 * This block is removed from the cache for new requests immediately.
	 * Existing requests may see this state.
	 */
	IO_BLOCK_ERROR,

} IoBlockState;

/**
 * Structure containing information about a block
 */
typedef struct IoBlock
{
	///Offset of block in device
	unsigned long long offset;

	///All blocks list
	ListHead listItem;

	///Item in block hash table
	HashItem hItem;

	///Block state
	IoBlockState state;

	///Wait queue containing blocks waiting for an operation on this block to complete.
	ProcWaitQueue waitingThreads;

	/**
	 * Block reference count
	 *
	 * This is incremented for blocks in use (threads have valid pointers to the block).
	 *
	 * The block can only be removed (or moved) from memory if this is 0.
	 */
	unsigned int refCount;

	///Address of block data
	char * address;

} IoBlock;

/**
 * A cache of blocks associated with a device
 */
typedef struct IoBlockCache
{
	///Size of blocks in cache
	unsigned int blockSize;

	///Hashtable of blocks (used for lookups)
	HashTable blockTable;

	///List of all blocks (used for removing all at end)
	ListHead blockList;

} IoBlockCache;

/**
 * Initializes a block cache
 *
 * Do not call more than once on a cache
 *
 * @param cache cache to initialize
 * @param blockSize size of each block in the cache
 */
void IoBlockCacheInit(IoBlockCache * cache, int blockSize);

/**
 * Empties all the blocks from the cache
 *
 * @param cache cache to empty
 * @retval true all the blocks in the cache were destroyed
 * @retval false if some blocks were locked
 */
bool IoBlockCacheEmpty(IoBlockCache * cache);

/**
 * Reads a block of data from the block cache or the device
 *
 * If you just want to read data from the disk into a buffer, use IoBlockCacheReadBuffer().
 *
 * Each block's data is stored in a contiguous location in memory but
 * adjacent blocks may not be stored in adjacent parts of memory.
 *
 * Blocks returned are locked by incrementing their reference counts
 * until unlocked by IoBlockCacheUnlock() (which you must call).
 *
 * Blocks returned will not be in an error state but blocking while holding a
 * lock on a block may cause the data in the block and / or its state to change.
 * Holding locks on blocks in the #IO_BLOCK_ERROR state will prevent successful
 * reads until the block is unlocked.
 *
 * @param[in] device device to read data from
 * @param[in] off offset in device to read from
 * @param[out] block outputs a pointer to the block (unmodified if this function returns an error)
 * @retval 0 the block was read successfully
 * @retval <0 an error occurred
 */
int IoBlockCacheRead(struct IoDevice * device, unsigned long long off, IoBlock ** block);

/**
 * Decrements the reference count on a block
 *
 * @param device device the block is resident on
 * @param block block to decrement
 */
void IoBlockCacheUnlock(struct IoDevice * device, IoBlock * block);

/**
 * Reads data from the block cache / device into a memory buffer
 *
 * If an error occurs, some data be already have been read into the buffer.
 *
 * @param device device to read from
 * @param off offset within device to read
 * @param buffer buffer to read into (this can be user mode)
 * @param length number of bytes to read
 * @retval 0 all data read successfully
 * @retval <0 error code
 */
int IoBlockCacheReadBuffer(struct IoDevice * device, unsigned long long off,
		void * buffer, unsigned int length);

/**
 * Writes data to the block cache and disk
 *
 * @param device device to write to
 * @param off offset within device to write
 * @param buffer buffer to read data from (this can be user mode)
 * @param length number of bytes to write
 * @retval 0 all data read successfully
 * @retval <0 error code
 */
int IoBlockCacheWriteBuffer(struct IoDevice * device, unsigned long long off,
		void * buffer, unsigned int length);

#endif /* BCACHE_H_ */
