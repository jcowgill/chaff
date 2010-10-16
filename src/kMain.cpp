/*
 * kMain.cpp
 *
 *  Created on: 30 Sep 2010
 *      Author: James
 */

#include "chaff.h"
#include "cppUtils.h"
#include "multiboot.h"

extern "C" void NORETURN kMain(unsigned int mBootCode, multiboot_info_t * mBootInfo);

void NORETURN kMain(unsigned int mBootCode, multiboot_info_t * mBootInfo)
{
	Chaff::CppUtils::InitGlobalObjects();
	//
	for(;;);
}

void NORETURN Chaff::Panic(const char * msg, ...)
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

void Chaff::PrintLog(LogLevel level, const char * msg, ...)
{
	//
}
