/*
 * kMain.cpp
 *
 *  Created on: 30 Sep 2010
 *      Author: James
 */

#include "chaff.h"
#include "cppUtils.h"

struct TestCls
{
	TestCls();
};

TestCls::TestCls()
{
	return;
}

TestCls test;

extern "C" void NORETURN kMain()
{
	Chaff::CppUtils::InitGlobalObjects();
	//
	for(;;);
}

void NORETURN Chaff::Panic(const char * msg)
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
