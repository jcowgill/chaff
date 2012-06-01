/**
 * @file
 * Global kernel functions and declarations
 *
 * @date September 2010
 * @author James Cowgill
 * @ingroup Util
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

#ifndef CHAFF_H_
#define CHAFF_H_

//Global C Headers
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <limits.h>

/**
 * Version of chaff being used
 */
#define CHAFF_VERSION 1

/**
 * Offset of the start of the kernel region of virtual space
 */
#define KERNEL_VIRTUAL_BASE ((void *) 0xC0000000)

/**
 * Function does not return
 */
#define NORETURN __attribute__((noreturn))

/**
 * Function uses the stdcall calling convention
 */
#define STDCALL __attribute__((stdcall))

/**
 * Function or variable is "hidden" from other modules (not placed in global symbol table)
 */
#define PRIVATE __attribute__((visibility("hidden")))

/**
 * Function / Variable is used in init code only
 *
 * Drivers MUST NOT use / call any function marked with this
 */
#define INIT PRIVATE __attribute__((section(".init")))

/**
 * Ignores an unused paremeter
 *
 * Prevents compiler warning about unused parameters (eg when implementing interfaces)
 *
 * Example:
 * @code
 * int someFunction(int param1, int param2)
 * {
 * 	IGNORE_PARAM param2;
 * 	return param1;
 * }
 * @endcode
 */
#define IGNORE_PARAM (void)

/**
 * @name Formatted Output
 * @{
 */

/**
 * Brings down the operating system as the result of an unrecoverable error
 *
 * @param format format of message to display before halting the system
 * @param ... any extra parameters used by the message
 */
void NORETURN Panic(const char * format, ...);

/**
 * Levels of logging which can be passed to PrintLog()
 */
typedef enum
{
	/**
	 * Fatal or unrecoverable errors (consider using Panic())
	 */
	Fatal,

	/**
	 * Critical errors (could crash at any time)
	 */
	Critical,

	/**
	 * Generic error
	 */
	Error,

	/**
	 * A significant abnormal condition
	 */
	Warning,

	/**
	 * A significant (but normal) condition
	 */
	Notice,

	/**
	 * Informational message
	 */
	Info,

	/**
	 * Debug message
	 */
	Debug,

} LogLevel;

/**
 * Prints a message to the kernel log using a va_list
 *
 * See PrintLog() for the format of the @b format argument
 *
 * @param level significance of the message
 * @param format format of message to display
 * @param args a va_list containing the format arguments
 */
void PrintLogVarArgs(LogLevel level, const char * format, va_list args)
	__attribute__((format(printf, 2, 0)));

/**
 * Prints a message to the kernel log
 *
 * The @b format argument is similar to the standard C printf however
 * you cannot use the floating point formats.
 *
 * %[flags][width][.precision]type
 *
 * Flags:
 * 	- @b +		numbers always start with a + or minus
 * 	- @b space	print a space infront of positive numbers
 * 	- @b -		left align
 * 	- @b #		Alternate form - prints 0, 0x or 0X infront of types o,x,X
 * 	- @b 0		Pad with zeros instead of spaces
 *
 * Width:
 * 	Minimum width
 *
 * Precision:
 * 	Strings - Maximum width
 * 	Numbers - Minimum width (does not include signs like width does)
 *
 * Type:
 * 	- @b d/i	signed int
 * 	- @b u		unsigned int
 * 	- @b x/X	unsigned int in hexadecimal
 * 	- @b o		unsigned int in octal
 * 	- @b s		null-terminated string
 * 	- @b c		character
 * 	- @b p		pointer (eg 0x01234ABCD)
 * 	- @b %		literal percent sign
 *
 * @param level significance of the message
 * @param format format of message to display
 * @param ... format arguments
 */
static inline void PrintLog(LogLevel level, const char * format, ...)
	__attribute__((format(printf, 2, 3)));
static inline void PrintLog(LogLevel level, const char * format, ...)
{
	//Pass to varargs version
	va_list args;
	va_start(args, format);
	PrintLogVarArgs(level, format, args);
	va_end(args);
}

/**
 * Generates a formatted string using a va_list
 *
 * See PrintLog() for the format of the @b format argument
 *
 * @param buffer buffer to store final string to
 * @param bufSize size of the buffer passed
 * @param format format of the message
 * @param args a va_list containing the format arguments
 */
void SPrintFVarArgs(char * buffer, int bufSize, const char * format, va_list args)
	__attribute__((format(printf, 3, 0)));

/**
 * Generates a formatted string
 *
 * See PrintLog() for the format of the @b format argument
 *
 * @param buffer buffer to store final string to
 * @param bufSize size of the buffer passed
 * @param format format of the message
 * @param ... format arguments
 */
static void SPrintF(char * buffer, int bufSize, const char * format, ...)
	__attribute__((format(printf, 3, 4)));
static inline void SPrintF(char * buffer, int bufSize, const char * format, ...)
{
	//Pass to varargs version
	va_list args;
	va_start(args, format);
	SPrintFVarArgs(buffer, bufSize, format, args);
	va_end(args);
}

/** @} */

/**
 * Sets @a count bytes of @a data to the specified value
 *
 * @param data pointer to the data to set
 * @param value the value to copy into each byte of @a data
 * @param count number of bytes to copy
 * @return @a data
 */
#define MemSet __builtin_memset

/**
 * Copies the source region to the destination region
 *
 * The regions must not overlap
 *
 * @param dest destination memory region
 * @param src source memory region
 * @param count number of bytes to copy
 * @return @a dest
 */
#define MemCpy __builtin_memcpy

/**
 * Copies the source region to the destination region allowing overlapping
 *
 * @param dest destination memory region
 * @param src source memory region
 * @param count number of bytes to copy
 * @return @a dest
 */
#define MemMove __builtin_memmove

/**
 * Compares two regions of memory
 *
 * @param ptr1 first region of memory to compare
 * @param ptr2 second region of memory to compare
 * @param count number of bytes to compare
 * @return 0, if the regions of memory are identical
 * 		>0, the first byte that doesn't match is greater in ptr1
 * 		<0, the first byte that doesn't match is smaller in ptr1
 */
#define MemCmp __builtin_memcmp

/**
 * Duplicates a null-terminated string using MAlloc()
 *
 * @param str null-terminated string to duplicate
 * @param maxLen maximum length to duplicate
 * @return a pointer to the new string
 */
char * StrDup(const char * str, unsigned int maxLen);

/**
 * Returns the length of a null-terminated string
 *
 * @param str null-terminated string to get length of
 * @param maxLen maximum length to return
 * @return the length of the string
 */
unsigned int StrLen(const char * str, unsigned int maxLen);

/**
 * Compares two null-terminated strings
 *
 * @param ptr1 first string to compare
 * @param ptr2 second string to compare
 * @return 0, if the strings are identical
 * 		>0, the first byte that doesn't match is greater in ptr1
 * 		<0, the first byte that doesn't match is smaller in ptr1
 */
#define StrCmp __builtin_strcmp

/**
 * Counts the number of trailing zeros in an integer
 *
 * This returns the offset of the first 1 bit from the LSB
 * If the input is zero, the result is undefined
 *
 * @param num number to scan
 * @return number of trailing zeros
 */
#define BitScanForward __builtin_ctz

/**
 * Counts the number of leading zeros in an integer
 *
 * This returns the offset of the first 1 bit from the MSB
 * If the input is zero, the result is undefined
 *
 * The offset is measured from the OTHER end to BitScanForward()
 *
 * @param num number to scan
 * @return number of leading zeros
 */
#define BitScanReverse __builtin_clz

#endif /* CHAFF_H_ */
