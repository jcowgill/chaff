/*
 * chaff.h
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
 *  Created on: 30 Sep 2010
 *      Author: James
 */

#ifndef CHAFF_H_
#define CHAFF_H_

//Global C Headers
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

//Chaff global header
#define CHAFF_VERSION 1

//Base of kernel
#define KERNEL_VIRTUAL_BASE ((void *) 0xC0000000)

//Non-returning function and stdcall convention
#define NORETURN __attribute__((noreturn))
#define STDCALL __attribute__((stdcall))

//Ignore unused parameter
#define IGNORE_PARAM (void)

//General functions

//Run when an unrecoverable error occurs.
// Notifies the user of the error (as fatal log) and promptly hangs
void NORETURN Panic(const char * msg, ...);

//Logging levels
typedef enum
{
	Fatal,			//Fatal / unrecoverable errors (consider using Panic)
	Critical,		//Critical errors (could crash at any time)
	Error,			//An error in a specific area
	Warning,		//A significant abnormal condition
	Notice,			//A significant (but normal) condition
	Info,			//Informational
	Debug,			//Debugging logs

} LogLevel;

//Prints something to the kernel log
void PrintLog(LogLevel level, const char * msg, ...);

//Kernel heap allocation
void * MAlloc(unsigned int size) __attribute__((alloc_size(1), malloc));
void MFree(void * data);

//Sets the specified number of bytes after the given pointer with the given value
// Returns ptr
#define MemSet __builtin_memset

//Copies the region of memory with the given length from src to dst
// src and dst must not overlap
// Returns ptr
#define MemCpy __builtin_memcpy

//Moves the region of memory with the given length from src to dst
// src and dst are allowed to overlap
// Returns ptr
#define MemMove __builtin_memmove

#define MemCmp __builtin_memcmp

//Duplicates a string using MAlloc
#define StrDup __builtin_strdup

//Determines string length
#define StrLen __builtin_strlen

//Compares 2 zero-terminated strings
#define StrCmp __builtin_strcmp

//Returns the offset of the last 1 bit in the given data
// If data == 0, the result is undefined
#define BitScanForward __builtin_ctz

//Returns the offset of the first 1 bit in the given data
// If data == 0, the result is undefined
#define BitScanReverse __builtin_clz

#endif /* CHAFF_H_ */
