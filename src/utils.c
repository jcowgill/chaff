/*
 * utils.c
 *
 *  Created on: 30 Sep 2010
 *      Author: James
 */

#include "chaff.h"
#include "list.h"

//Utility functions
//

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
	const char * cPtr1 = (const char *) ptr1;
	const char * cPtr2 = (const char *) ptr2;

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

//Strdup implementation
char * strdup(const char * s)
{
	//Get string length
	unsigned int sLen = StrLen(s);

	//Allocate memory, copy and return string
	return MemCpy(MAlloc(sLen + 1), s, sLen);
}

//Strlen implementation
unsigned int strlen(const char * s)
{
	const char * strStart = s;

	//Loop until NULL character
	while(*s)
	{
		++s;
	}

	//Return pointer difference
	return s - strStart;
}
