/*
 * mm/check.h
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
 *  Created on: 8 Feb 2012
 *      Author: James
 */

#ifndef MM_CHECK_H_
#define MM_CHECK_H_

#include "chaff.h"

//Memory address validation checks
// These functions check whether memory can be used
//

//Verifies that an area of memory can be read by the kernel or user
// If data >= KERNEL_VIRTUAL_BASE, a kernel check is performed
// For user data, check it with MemCheckUserArea first
bool MemCanRead(void * data, unsigned int length);

//Verifies that an area of memory can be written by the kernel or user
// If data >= KERNEL_VIRTUAL_BASE, a kernel check is performed
// For user data, check it with MemCheckUserArea first
bool MemCanWrite(void * data, unsigned int length);

//Verifies that an area of memory can be read by the kernel or user and commits it
// This function may block
static inline bool MemCommitForRead(void * data, unsigned int length)
{
	//Stub until MemCommit is implemented
	// Since pagefaults currently don't block - nothing more is needed
	return MemCanRead(data, length);
}

//Verifies that an area of memory can be written by the kernel or user and commits it
// This function may block
static inline bool MemCommitForWrite(void * data, unsigned int length)
{
	//Stub until MemCommit is implemented
	// Since pagefaults currently don't block - nothing more is needed
	return MemCanWrite(data, length);
}

//Verifies that the region of memory passed is all in user mode (< 0xC0000000)
// This DOES NOT check if the memory is actually readable / writable
static inline bool MemCheckUserArea(void * data, unsigned int length)
{
	char * cData = (char *) data;
	char * cKernBase = (char *) KERNEL_VIRTUAL_BASE;

	return (cData + length) < cKernBase && (cData < (cData + length));
}

//Combined MemCheckUserArea and MemCommitForRead convinience function
static inline bool MemCommitUserForRead(void * data, unsigned int length)
{
	return MemCheckUserArea(data, length) && MemCommitForRead(data, length);
}

//Combined MemCheckUserArea and MemCommitForWrite convinience function
static inline bool MemCommitUserForWrite(void * data, unsigned int length)
{
	return MemCheckUserArea(data, length) && MemCommitForWrite(data, length);
}

#endif
