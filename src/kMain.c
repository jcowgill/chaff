/*
 * kMain.c
 *
 *  Created on: 30 Sep 2010
 *      Author: James
 */

#include "chaff.h"
#include "multiboot.h"


void NORETURN kMain(unsigned int mBootCode, multiboot_info_t * mBootInfo)
{
	//
	for(;;);
}

void NORETURN Panic(const char * msg, ...)
{
	//
	char * testChar = (char *) 0xC00B8000;
	*testChar++ = 'T';
	*testChar++ = 7;
	*testChar++ = 'e';
	*testChar++ = 7;
	*testChar++ = 's';
	*testChar++ = 7;
	*testChar++ = 't';
	*testChar++ = 7;
	msg = NULL;

	for(;;);
}

void PrintLog(LogLevel level, const char * msg, ...)
{
	//
}
