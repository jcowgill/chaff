/*
 * cppUtils.cpp
 *
 *  Created on: 30 Sep 2010
 *      Author: James
 */

#include "chaff.h"
#include "cppUtils.h"
#include "list.h"

//Utilities for C++
// This mostly contains symbols required by GCC for freee standing C++ code
//

//C definitions
extern "C" int __cxa_atexit(void (* func)(void *), void * arg, void * dso);
extern "C" void __cxa_finalize(void * dso);
extern "C" void __cxa_pure_virtual();
extern "C" void * memset(void * ptr, int value, size_t length);
extern "C" void * memcpy(void * dest, const void * src, size_t length);
extern "C" void * memmove(void * dest, const void * src, size_t length);
extern "C" int memcmp(const void * ptr1, const void * ptr2, size_t length);

//Constructors symbols
typedef void (* constructCall) ();

DECLARE_SYMBOL(start_ctors);
DECLARE_SYMBOL(end_ctors);

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
	__cxa_finalize(0);
}

//Destructors handling
// Kernel dso handle
void * __dso_handle;

struct DestructorData
{
	DECLARE_LISTENTRY(DestructorData, link);
	void (* func)(void *);
	void * arg;
	void * dso;
};

DECLARE_LISTHEAD(DestructorData, link) destructorHead;

//Registers a function to be called with the given argument when the dso is finalized
// Each shared object uses a different dso handle but calls the same atexit function
int __cxa_atexit(void (* func)(void *), void * arg, void * dso)
{
	//Create entry and add to chain
	DestructorData * data = new DestructorData();
	data->func = func;
	data->arg = arg;
	data->dso = dso;
	destructorHead.InsertLast(data);

	return 0;
}

//Calls all destructors registered with the dso provided
// If dso is NULL, all destructors are called
void __cxa_finalize(void * dso)
{
	DECLARE_LISTITERATOR(DestructorData, link) iter = destructorHead.Begin();

	//Call destructor functions
	while(iter != destructorHead.End())
	{
		//Destruct?
		if(dso == NULL || dso == iter->dso)
		{
			iter->func(iter->arg);

			//Move on and delete the previous
			DestructorData * data = &*iter;
			++iter;
			delete data;
		}
		else
		{
			++iter;
		}
	}
}

//Memset implementation
// Required by __builtin_memset
void * memset(void * ptr, int value, size_t length)
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
void * memcpy(void * dest, const void * src, size_t length)
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
void * memmove(void * dest, const void * src, size_t length)
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

//Memcmp implementation
// Required by __builtin_memcmp
int memcmp(const void * ptr1, const void * ptr2, size_t length)
{
	const char * cPtr1 = reinterpret_cast<const char *>(ptr1);
	const char * cPtr2 = reinterpret_cast<const char *>(ptr2);

	//Compare memory
	for(; length > 0; --length)
	{
		//Same?
		if(*cPtr1 != *cPtr2)
		{
			//Determine and return how it's different
			if(*cPtr1 < *cPtr2)
			{
				return -1;
			}
			else
			{
				return 1;
			}
		}

		//Advance pointers
		++cPtr1;
		++cPtr2;
	}

	//Identical
	return 0;
}

//GCC stuff to get certain C++ features to work
void __cxa_pure_virtual()
{
	Chaff::Panic("__cxa_pure_virtual called (this should never happen)");
}
