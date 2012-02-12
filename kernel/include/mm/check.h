/**
 * @file
 * Memory address validation checks
 *
 * These functions check whether memory can be used
 *
 * @date February 2012
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

#ifndef MM_CHECK_H_
#define MM_CHECK_H_

#include "chaff.h"

/**
 * Verifies an area of memory can be read
 *
 * This function will allow reads to kernel regions.
 *
 * Use MemCheckUserArea() as well if the data pointer is from user mode.
 * This allows the caller to decide whether the function can be used by user mode.
 *
 * @param data pointer to the area of memory to check
 * @param length length of memory being accessed
 * @retval true if the area of memory can be read
 * @retval false if the area of memory cannot be read
 */
bool MemCanRead(void * data, unsigned int length);

/**
 * Verifies an area of memory can be written to
 *
 * This function will allow writes to kernel regions.
 *
 * Use MemCheckUserArea() as well if the data pointer is from user mode.
 * This allows the caller to decide whether the function can be used by user mode.
 *
 * @param data pointer to the area of memory to check
 * @param length length of memory being accessed
 * @retval true if the area of memory can be written to
 * @retval false if the area of memory cannot be written to
 */
bool MemCanWrite(void * data, unsigned int length);

/**
 * Verifies an area of memory can be read and commits it to memory
 *
 * This function will allow reads to kernel regions.
 *
 * Use MemCheckUserArea() as well if the data pointer is from user mode.
 * This allows the caller to decide whether the function can be used by user mode.
 *
 * This function may block.
 *
 * @param data pointer to the area of memory to check
 * @param length length of memory being accessed
 * @retval true if the area of memory can be read and was committed
 * @retval false if the area of memory cannot be read
 * @bug this doesn't actually commit anything at the moment
 */
static inline bool MemCommitForRead(void * data, unsigned int length)
{
	//Stub until MemCommit is implemented
	// Since pagefaults currently don't block - nothing more is needed
	return MemCanRead(data, length);
}

/**
 * Verifies an area of memory can be written to and commits it to memory
 *
 * This function will allow reads to kernel regions.
 *
 * Use MemCheckUserArea() as well if the data pointer is from user mode.
 * This allows the caller to decide whether the function can be used by user mode.
 *
 * This function may block.
 *
 * @param data pointer to the area of memory to check
 * @param length length of memory being accessed
 * @retval true if the area of memory can be written to and was committed
 * @retval false if the area of memory cannot be written to
 * @bug this doesn't actually commit anything at the moment
 */
static inline bool MemCommitForWrite(void * data, unsigned int length)
{
	//Stub until MemCommit is implemented
	// Since pagefaults currently don't block - nothing more is needed
	return MemCanWrite(data, length);
}

/**
 * Verifies that the area of memory passed is in user mode
 *
 * This does not check if the memory is actually readable / writable
 *
 * @param data pointer to the area of memory to check
 * @param length length of memory being accessed
 * @retval true if the area of memory is entirely in user mode
 * @retval false if the area of memory is partially in kernel mode
 */
static inline bool MemCheckUserArea(void * data, unsigned int length)
{
	char * cData = (char *) data;
	char * cKernBase = (char *) KERNEL_VIRTUAL_BASE;

	return (cData + length) < cKernBase && (cData < (cData + length));
}

/**
 * Combined version of MemCheckUserArea() and MemCommitForRead()
 *
 * This is designed for situations where you know the pointer must only be from user mode.
 *
 * This function may block.
 *
 * @param data pointer to the area of memory to check
 * @param length length of memory being accessed
 * @retval true if the area of memory can be read and was committed
 * @retval false if the area of memory cannot be read
 */
static inline bool MemCommitUserForRead(void * data, unsigned int length)
{
	return MemCheckUserArea(data, length) && MemCommitForRead(data, length);
}

/**
 * Combined version of MemCheckUserArea() and MemCommitForWrite()
 *
 * This is designed for situations where you know the pointer must only be from user mode.
 *
 * This function may block.
 *
 * @param data pointer to the area of memory to check
 * @param length length of memory being accessed
 * @retval true if the area of memory can be written to and was committed
 * @retval false if the area of memory cannot be written to
 */
static inline bool MemCommitUserForWrite(void * data, unsigned int length)
{
	return MemCheckUserArea(data, length) && MemCommitForWrite(data, length);
}

#endif
