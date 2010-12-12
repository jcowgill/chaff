/*
 * interrupt.h
 *
 *  Created on: 11 Dec 2010
 *      Author: James
 */

#ifndef INTERRUPT_H_
#define INTERRUPT_H_

typedef struct
{
	//General purpose registers (from pusha)
	unsigned int edi;
	unsigned int esi;
	unsigned int ebp;
	unsigned int : 32;
	unsigned int ebx;
	unsigned int edx;
	unsigned int ecx;
	unsigned int eax;

	//Segment selectors
	unsigned int ds;
	unsigned int es;
	unsigned int fs;
	unsigned int gs;

	//Interrupt information
	unsigned int intrNum;
	unsigned int intrError;

	//IRET return information
	unsigned int eip;
	unsigned int cs;
	unsigned int eflags;

	//These only exist if the interrupt happened while in user mode (cs = 0x10)
	unsigned int esp;
	unsigned int ss;

} IntrContext;

//Initialise interrupts
void IntrInit();

//Interrupt handler entry point
void IntrHandler(IntrContext iContext);

#endif /* INTERRUPT_H_ */
