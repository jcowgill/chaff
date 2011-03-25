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

#define INTR_SHARED 1		//Allows interrupt to have multiple interrupt handlers

//Interrupt registering
// Registers the given IRQ with an interrupt handler
// The order shared interrupt handlers are executed in could be any order
bool IntrRegister(int irq, int flags, void (* handler)(IntrContext *));

// Unregisters an IRQ
bool IntrUnRegister(int irq, void (* handler)(IntrContext *));

#endif /* INTERRUPT_H_ */
