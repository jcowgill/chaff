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

//Initializes a block cache
// Do not call on an alredy initialized cache
void IoBlockCacheInit(IoBlockCache * cache, int blockSize)
{
	//
}

//Destroys a block cache
void IoBlockCacheEmpty(IoBlockCache * cache)
{
	//
}

//Reads a block of data from the block cache or reads it from the disk if it isn't there
// Returns a negative value on error or the number of bytes you can read from the block
// A single block is stored in a contingous location in memory but multiple blocks are not
// The returned block pointer locked until IoBlockCacheUnlock is called
int IoBlockCacheRead(IoDevice * device, unsigned long long off, IoBlock ** block)
{
	//
}

//Decrements the reference count on a block from the cache
void IoBlockCacheUnlock(IoDevice * device, IoBlock * block)
{
	//
}

//Writes data to disk and to the block cache
// Returns a negative value on error or the number of bytes actually written
// (currently the block cache is always write-through)
int IoBlockCacheWrite(IoDevice * device, unsigned long long off, void * buffer, int length)
{
	//
}
