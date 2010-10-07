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

//External symbol accessing
#define DECLARE_SYMBOL(symb) extern char symb[];
#define GET_SYMBOL(symb) ((void *) &symb)
#define GET_SYMBOL_UINTPTR(symb) ((unsigned int *) &symb)

//General functions
namespace Chaff
{
	//Run when an unrecoverable error occurs.
	// Notifies the user of the error (as fatal log) and promptly hangs
	void NORETURN Panic(const char * msg, ...);

	//Logging levels
	enum LogLevel
	{
		Fatal,			//Fatal / unrecoverable errors (concider using Panic)
		Critical,		//Critical errors (could crash at any time)
		Error,			//An error in a specific area
		Warning,		//A significant abnormal condition
		Notice,			//A significant (but normal) condition
		Info,			//Informational
		Debug,			//Debugging logs
	};

	//Prints something to the kernel log
	void PrintLog(LogLevel level, const char * msg, ...);
}

#endif /* CHAFF_H_ */
