/*
 * bcache.c
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
 *  Created on: 16 Jan 2012
 *      Author: james
 */

#include "chaff.h"
#include "io/bcache.h"
#include "io/device.h"
#include "errno.h"
#include "mm/check.h"

//Insert into block cache table
static inline bool IoBlockHashInsert(IoBlockCache * cache, IoBlock * block)
{
	return HashTableInsert(&cache->blockTable, &block->hItem, &block->offset, sizeof(unsigned long long));
}

static inline IoBlock * IoBlockHashFind(IoBlockCache * cache, unsigned long long off)
{
	HashItem * item = HashTableFind(&cache->blockTable, &off, sizeof(unsigned long long));

	if(item)
	{
		return HashTableEntry(item, IoBlock, hItem);
	}
	else
	{
		return NULL;
	}
}

//Initializes a block cache
// Do not call on an alredy initialized cache
void IoBlockCacheInit(IoBlockCache * cache, int blockSize)
{
	//Setup cache parameters
	// Check block size
	if((blockSize & (blockSize - 1)) != 0 || blockSize == 0)
	{
		PrintLog(Critical, "IoBlockCacheInit: can only create block cache with power of 2 size");
		return;
	}
	else if(blockSize < 16)
	{
		PrintLog(Warning, "IoBlockCacheInit: low block cache size isn't very efficient");
	}

	cache->blockSize = blockSize;

	//Setup cache structures
	ListHeadInit(&cache->blockList);
}

//Destroys a block cache
// Returns false if some blocks are still lockeds
bool IoBlockCacheEmpty(IoBlockCache * cache)
{
	//Remove each entry from the list
	IoBlock * block;
	IoBlock * tmpBlock;
	bool allUnlocked = true;

	ListForEachEntrySafe(block, tmpBlock, &cache->blockList, listItem)
	{
		//Remove if unlocked
		if(block->refCount == 0)
		{
			HashTableRemoveItem(&cache->blockTable, &block->hItem);
			ListDelete(&block->listItem);

			MFree(block->address);
			MFree(block);
		}
		else
		{
			allUnlocked = false;
		}
	}

	return allUnlocked;
}

//Creates an empty block for the cache (off must be block aligned and must not exist)
static IoBlock * CreateEmptyBlock(IoBlockCache * bCache, unsigned long long off)
{
	// Create new block and memory region
	IoBlock * block = MAlloc(sizeof(IoBlock));

	block->offset = off;
	ListHeadInit(&block->listItem);
	ProcWaitQueueInit(&block->waitingThreads);
	block->refCount = 1;

	block->address = MAlloc(bCache->blockSize);

	// Insert block into hash map
	IoBlockHashInsert(bCache, block);
	return block;
}

//Reads a block of data from the block cache or reads it from the disk if it isn't there
// Returns a negative value on error or the number of bytes you can read from the block
// A single block is stored in a contingous location in memory but multiple blocks are not
// The returned block pointer locked until IoBlockCacheUnlock is called
int IoBlockCacheRead(IoDevice * device, unsigned long long off, IoBlock ** block)
{
	//Get block cache
	IoBlockCache * bCache = device->blockCache;

	//Align off to block boundary
	off &= ~(bCache->blockSize - 1);

	//Lookup block in hash table
	IoBlock * readBlock = IoBlockHashFind(bCache, off);

	if(readBlock == NULL)
	{
		//Read block from disk
		// Require device reader
		if(!device->devOps->read)
		{
			return -ENOSYS;
		}

		// Create new block and memory region
		readBlock = CreateEmptyBlock(bCache, off);
		readBlock->state = IO_BLOCK_READING;

		// Read data from device
		int res = device->devOps->read(device, off, readBlock->address, bCache->blockSize);

		// Wake up threads
		ProcWaitQueueWakeAll(&readBlock->waitingThreads);

		// Check for errors and continue
		if(res != 0)
		{
			//Mark block in error state
			readBlock->state = IO_BLOCK_ERROR;

			//Remove block from cache
			HashTableRemoveItem(&bCache->blockTable, &readBlock->hItem);

			//Unlock block
			IoBlockCacheUnlock(device, readBlock);
			return -EIO;
		}
		else
		{
			//Mark block in ok state
			readBlock->state = IO_BLOCK_OK;
		}
	}
	else
	{
		//Increment item ref count
		readBlock->refCount++;

		//If block is being read, wait until finished
		if(readBlock->state == IO_BLOCK_READING)
		{
			//Wait for read to finish
			ProcWaitQueueWait(&readBlock->waitingThreads, false);

			//Check if the block is now in an error state
			if(readBlock->state == IO_BLOCK_ERROR)
			{
				IoBlockCacheUnlock(device, readBlock);
				return -EIO;
			}
		}
	}

	//Return block
	*block = readBlock;
	return 0;
}

//Decrements the reference count on a block from the cache
void IoBlockCacheUnlock(IoDevice * device, IoBlock * block)
{
	IGNORE_PARAM device;

	//Very simple at the moment
	if(block->refCount > 0)
	{
		block->refCount--;

		if(block->state == IO_BLOCK_ERROR && block->refCount == 0)
		{
			//Erase block
			ListDelete(&block->listItem);
			MFree(block->address);
			MFree(block);
		}
	}
	else
	{
		PrintLog(Warning, "IoBlockCacheUnlock: block already unlocked");
	}
}

//Uses the block cache to read / copy data into a buffer
// Returns a negative value on error or the number of bytes actually read
// Nothing needs to be unlocked after this
int IoBlockCacheReadBuffer(IoDevice * device, unsigned long long off,
		void * buffer, unsigned int length)
{
	//Ignore blank length
	if(length == 0)
	{
		return 0;
	}

	//Get block size
	int blockSize = device->blockCache->blockSize;

	//Loop reading blocks and copying data
	while(length > 0)
	{
		//Read this block
		IoBlock * block;
		int res = IoBlockCacheRead(device, off, &block);

		if(res != 0)
		{
			return res;
		}

		//Determine offset and length in block
		int blockOff = off & (blockSize - 1);				//Offset to read within block
		unsigned int blockLength = blockSize - blockOff;	//Number of bytes to read after offset

		if(blockLength > length)
		{
			blockLength = length;
		}

		//Memory checks
		if(!MemCommitForRead(buffer, blockLength))
		{
			//Unlock and return
			IoBlockCacheUnlock(device, block);
			return -EFAULT;
		}

		//Copy data
		MemCpy(buffer, block->address + blockOff, blockLength);

		//Unlock block
		IoBlockCacheUnlock(device, block);

		//Advance buffer
		off += blockLength;
		length -= blockLength;
		buffer = ((char *) buffer) + blockLength;
	}

	//Finished
	return 0;
}

//Writes data to disk and to the block cache
// Returns a negative value on error or the number of bytes actually written
// (currently the block cache is always write-through)
int IoBlockCacheWriteBuffer(IoDevice * device, unsigned long long off,
		void * buffer, unsigned int length)
{
	//Ignore blank length
	if(length == 0)
	{
		return 0;
	}

	//Require write call
	if(!device->devOps->write)
	{
		return -ENOSYS;
	}

	//Get cache info
	IoBlockCache * bCache = device->blockCache;
	unsigned int blockSize = bCache->blockSize;

	//Loop reading blocks and copying data
	while(length > 0)
	{
		//Determine offset and length in block
		int blockOff = off & (blockSize - 1);				//Offset to write within block
		unsigned int blockLength = blockSize - blockOff;	//Number of bytes to write after offset

		if(blockLength > length)
		{
			blockLength = length;
		}

		IoBlock * block;
		int res;

		//If the block is being partially written, it must be read first
		if(blockOff != 0 || blockLength != blockSize)
		{
			//Read block
			res = IoBlockCacheRead(device, off, &block);

			if(res != 0)
			{
				return res;
			}
		}
		else
		{
			//Replace block or create a new block
			unsigned long long alignedOff = off & ~(blockSize - 1);
			block = IoBlockHashFind(bCache, alignedOff);

			if(block == NULL)
			{
				//Create new block
				block = CreateEmptyBlock(bCache, alignedOff);
				block->state = IO_BLOCK_OK;
			}
		}

		//Wait for block to become avaliable
		while(block->state == IO_BLOCK_READING || block->state == IO_BLOCK_WRITING)
		{
			ProcWaitQueueWait(&block->waitingThreads, false);
		}

		//If block has errored, return
		if(block->state == IO_BLOCK_ERROR)
		{
			IoBlockCacheUnlock(device, block);
			return -EIO;
		}

		//Memory checks
		if(!MemCommitForWrite(block->address + blockOff, blockLength))
		{
			IoBlockCacheUnlock(device, block);
			return -EFAULT;
		}

		//Modify block contents
		block->state = IO_BLOCK_WRITING;
		MemCpy(block->address + blockOff, buffer, blockLength);

		//Commit to disk
		// This always happens since this is a write-through cache
		// Writing is always done from buffer memory (removing user-mode issues)
		res = device->devOps->write(device, off, block->address + blockOff, blockLength);

		if(res == 0)
		{
			block->state = IO_BLOCK_OK;
		}
		else
		{
			block->state = IO_BLOCK_ERROR;

			//Also, remove block from cache
			HashTableRemoveItem(&bCache->blockTable, &block->hItem);
		}

		//Wake up other threads and release block
		ProcWaitQueueWakeAll(&block->waitingThreads);
		IoBlockCacheUnlock(device, block);

		//Return if there was an error
		if(res != 0)
		{
			return res;
		}

		//Advance buffer
		off += blockLength;
		length -= blockLength;
		buffer = ((char *) buffer) + blockLength;
	}

	//Finished
	return 0;
}
