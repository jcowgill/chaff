/*
 * cppUtils.cpp
 *
 *  Created on: 30 Sep 2010
 *      Author: James
 */

#include "chaff.h"
#include "cppUtils.h"

//Utilities for C++
//

typedef void (* constructCall) ();

DECLARE_SYMBOL(start_ctors);
DECLARE_SYMBOL(end_ctors);
DECLARE_SYMBOL(start_dtors);
DECLARE_SYMBOL(end_dtors);

//Global object initialisation and termination
// Objects should be initialised as soon as possible so they can be used
void Chaff::CppUtils::InitGlobalObjects()
{
	//Call all static constructors
	for(unsigned int * call = GET_SYMBOL_UINTPTR(start_ctors);
			call < GET_SYMBOL_UINTPTR(end_ctors); ++call)
	{
		((void (*) ()) call)();
	}
}

void Chaff::CppUtils::TermGlobalObjects()
{
	//Call all static destructors
	for(unsigned int * call = GET_SYMBOL_UINTPTR(start_ctors);
			call < GET_SYMBOL_UINTPTR(end_ctors); ++call)
	{
		((void (*) ()) call)();
	}
}

extern "C" void * memset(void * ptr, int value, unsigned int length);
extern "C" void * memcpy(void * dest, const void * src, unsigned int length);
extern "C" void * memmove(void * dest, const void * src, unsigned int length);

//Memset implementation
// Required by __builtin_memset
void * memset(void * ptr, int value, unsigned int length)
{
	char * charPtr = (char *) ptr;

	//Truncate value
	value = (char) value;

	//Only do this extra bit for long lengths
	if(length >= 8)
	{
		//Do first bytes to get it 4 bytes aligned
		for(int i = ((unsigned int) charPtr % 4); i > 0 && length > 0; --i)
		{
			*charPtr++ = value;
			--length;
		}

		//Do middle bytes
		unsigned int valueLong = value | value << 8 | value << 16 | value << 24;

		for(; length >= 4; length -= 4)
		{
			*((unsigned int *) charPtr) = valueLong;
			charPtr += 4;
		}
	}

	//Do end bytes
	for(; length > 0; --length)
	{
		*charPtr++ = value;
	}

	return ptr;
}

//Memcpy implementation
// Required by __builtin_memcpy
void * memcpy(void * dest, const void * src, unsigned int length)
{
	char * destChar = (char *) dest;
	const char * srcChar = (char *) src;

	//Copy everything
	for(; length > 0; --length)
	{
		*destChar++ = *srcChar++;
	}

	return dest;
}

//Memmove implementation
// Required by __builtin_memmove
void * memmove(void * dest, const void * src, unsigned int length)
{
	//Test which is bigger
	if(dest > src)
	{
		char * destChar = ((char *) dest) + length;
		const char * srcChar = ((char *) src) + length;

		//Copy backwards
		for(; length > 0; --length)
		{
			*(--destChar) = *(--srcChar);
		}
	}
	else if(dest < src)
	{
		char * destChar = (char *) dest;
		const char * srcChar = (char *) src;

		//Copy forwards
		for(; length > 0; --length)
		{
			*destChar++ = *srcChar++;
		}
	}

	return dest;
}

//GCC stuff to get certain C++ features to work
void __cxa_pure_virtual()
{
	Chaff::Panic("__cxa_pure_virtual called (this should never happen)");
}
