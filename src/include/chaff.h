/*
 * chaff.h
 *
 *  Created on: 30 Sep 2010
 *      Author: James
 */

#ifndef CHAFF_H_
#define CHAFF_H_

//Chaff global header
#define CHAFF_VERSION 1

//Base of kernel
#define KERNEL_VIRTUAL_BASE ((void *) 0xC0000000)

//Non-returning function and stdcall convention
#define NORETURN __attribute__((noreturn))
#define STDCALL __attribute__((stdcall))

//The null pointer
#define NULL 0

//Boolean type
#define true 1
#define false 0
#define bool _Bool

//Generic size (32 bits)
typedef unsigned long size_t;

//External symbol accessing
#define DECLARE_SYMBOL(symb) extern char symb[];
#define GET_SYMBOL(symb) ((void *) symb)
#define GET_SYMBOL_UINT(symb) ((unsigned int) symb)

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
void * MAlloc(unsigned int size);
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

//Returns the offset of the first 1 bit in the given data
// If data == 0, the result is undefined
#define BitScanReverse __builtin_clz

//Varargs definitions
#define va_start(v,l) __builtin_va_start(v,l)
#define va_arg(v,l)   __builtin_va_arg(v,l)
#define va_end(v)     __builtin_va_end(v)
#define va_copy(d,s)  __builtin_va_copy(d,s)
typedef __builtin_va_list va_list;

#endif /* CHAFF_H_ */
