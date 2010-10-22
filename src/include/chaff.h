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

//Non-returning function
#define NORETURN __attribute__((noreturn))

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
#define GET_SYMBOL(symb) ((void *) &symb)
#define GET_SYMBOL_UINTPTR(symb) ((unsigned int *) &symb)

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

//Sets the specified number of bytes after the given pointer with the given value
// Returns ptr
inline void * MemSet(void * ptr, unsigned char value, size_t length)
{
	__builtin_memset(ptr, value, length);
	return ptr;
}

//Copies the region of memory with the given length from src to dst
// src and dst must not overlap
// Returns ptr
inline void * MemCpy(void * ptr, const void * src, size_t length)
{
	__builtin_memcpy(ptr, src, length);
	return ptr;
}

//Moves the region of memory with the given length from src to dst
// src and dst are allowed to overlap
// Returns ptr
inline void * MemMove(void * ptr, const void * src, size_t length)
{
	__builtin_memmove(ptr, src, length);
	return ptr;
}

inline int MemCmp(const void * ptr1, const void * ptr2, size_t length)
{
	return __builtin_memcmp(ptr1, ptr2, length);
}

//Returns the offset of the first 1 bit in the given data
// If data == 0, the result is undefined
inline int BitScanReverse(unsigned int data)
{
	return __builtin_clz(data);
}

#endif /* CHAFF_H_ */
