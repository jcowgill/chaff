/*
 * inlineasm.h
 *
 *  Created on: 21 Nov 2010
 *      Author: James
 */

#ifndef INLINEASM_H_
#define INLINEASM_H_

//Control register setters
static inline void setCR3(unsigned int val)
{
	asm ("movl %0, %%cr3"::"r"(val));
}

static inline unsigned int getCR3()
{
	unsigned int val;
	asm ("movl %%cr3, %0":"=r"(val));
	return val;
}

#endif
