/*
 * interrupt.h
 *
 *  Copyright 2012 James Cowgill
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
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

	//These only exist if the interrupt happened while in user mode (cs = 0x1B)
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
