/*
 * cppUtils.h
 *
 *  Created on: 30 Sep 2010
 *      Author: James
 */

#ifndef CPPUTILS_H_
#define CPPUTILS_H_

namespace Chaff
{
	namespace CppUtils
	{
		//Global object initialisation and termination
		// Objects should be initialised as soon as possible so they can be used
		void InitGlobalObjects();
		void TermGlobalObjects();
	}
}

#endif
