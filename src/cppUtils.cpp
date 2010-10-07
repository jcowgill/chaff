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

//GCC stuff to get certain C++ features to work
void __cxa_pure_virtual()
{
	Chaff::Panic("__cxa_pure_virtual called (this should never happen)");
}
