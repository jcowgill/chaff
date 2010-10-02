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
	void NORETURN Panic(const char * msg);
}

#endif /* CHAFF_H_ */
