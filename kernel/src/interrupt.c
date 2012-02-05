/*
 * interrupt.c
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

#include "chaff.h"
#include "memmgr.h"
#include "interrupt.h"
#include "inlineasm.h"
#include "exceptions.h"
#include "timer.h"
#include "process.h"

//Interrupt gate structure
typedef struct
{
	unsigned int offLow : 16;
	unsigned int destSegment : 16;

	unsigned int /* unused */ : 8;
	unsigned int gateInfo: 8;

	unsigned int offHigh : 16;

} IntrGateStruct;

//Interrupt entry points
extern unsigned int IntrISRList;		//List of entry points for ISRs 0 to 47
void Isr66();

//Gate info values
#define INTR_KERNEL_GATE 0x8E
#define INTR_USER_GATE 0xEE

//Interrupt descriptor table
static IntrGateStruct idt[0x43] __attribute__((aligned(8)));

//Interrupt dispatch table
typedef struct SIntrDispatchEntry
{
	void (* handler)(IntrContext *);
	int flags;
	struct SIntrDispatchEntry * next;

} IntrDispatchEntry;

static IntrDispatchEntry IntrDispatchTable[0x30] =
{
	{ IntrExceptMathFault, 0, NULL },		//Divide by 0
	{ IntrExceptDebugTrap, 0, NULL },		//Debug Step
	{ IntrExceptError, 0, NULL },			//NMI
	{ IntrExceptDebugTrap, 0, NULL },		//Debug Breakpoint
	{ IntrExceptMathTrap, 0, NULL },		//Overflow (INTO)
	{ IntrExceptMathTrap, 0, NULL },		//Bound range exceeded (BOUND)
	{ IntrExceptIllOpcodeFault, 0, NULL },	//Illegal opcode
	{ IntrExceptMathFault, 0, NULL },		//No math coprocessor
	{ IntrExceptError, 0, NULL },			//Double Fault
	{ IntrExceptProtectionFault, 0, NULL }, //Coprocessor segment overrun
	{ IntrExceptError, 0, NULL },			//Invalid TSS
	{ IntrExceptProtectionFault, 0, NULL }, //Segment not present
	{ IntrExceptProtectionFault, 0, NULL },	//Stack segment fault
	{ IntrExceptProtectionFault, 0, NULL },	//GPF
	{ MemPageFaultHandler, 0, NULL },		//Page fault
	{ IntrExceptError, 0, NULL },			//Reserved
	{ IntrExceptMathFault, 0, NULL },		//Floating Point Exception
	{ IntrExceptAlignmentFault, 0, NULL },	//Alignment Check
	{ IntrExceptMathFault, 0, NULL },		//SMID Exception
};

//Initialise interrupts
void IntrInit()
{
	//Setup descriptor table
	int i;

	// 0 to 2 (kernel only)
	for(i = 0; i <= 2; ++i)
	{
		idt[i].offLow = (&IntrISRList)[i] & 0xFFFF;
		idt[i].offHigh = (&IntrISRList)[i] >> 16;
		idt[i].destSegment = 0x08;
		idt[i].gateInfo = INTR_KERNEL_GATE;
	}

	// 3 to 5 (user and kernel)
	for(; i <= 5; ++i)
	{
		idt[i].offLow = (&IntrISRList)[i] & 0xFFFF;
		idt[i].offHigh = (&IntrISRList)[i] >> 16;
		idt[i].destSegment = 0x08;
		idt[i].gateInfo = INTR_USER_GATE;
	}

	// 6 to 47 (kernel only)
	for(; i <= 47; ++i)
	{
		idt[i].offLow = (&IntrISRList)[i] & 0xFFFF;
		idt[i].offHigh = (&IntrISRList)[i] >> 16;
		idt[i].destSegment = 0x08;
		idt[i].gateInfo = INTR_KERNEL_GATE;
	}

	// 66 Syscall
	idt[0x42].offLow = (unsigned int) Isr66 & 0xFFFF;
	idt[0x42].offHigh = (unsigned int) Isr66 >> 16;
	idt[0x42].destSegment = 0x08;
	idt[0x42].gateInfo = INTR_USER_GATE;

	//Load idt
	lidt(sizeof(IntrGateStruct) * 0x43, idt);

	//Reprogram PIC
	outb(0x20, 0x11);		//Begin initialization on both PICs
	outb(0xA0, 0x11);

	outb(0x21, 0x20);		//Set remapping addresses
	outb(0xA1, 0x28);

	outb(0x21, 4);			//This tells the PICs how they are joined together
	outb(0xA1, 2);

	outb(0x21, 1);			//Set how PIC is notified of a completed interrupt
	outb(0xA1, 1);

	outb(0x21, 0xFB);		//Disable all IO interrupts until later (IRQ 2 is enabled for cascading interrupt)
	outb(0xA1, 0xFF);
}

//Handle interrupts
void IntrHandler(IntrContext iContext)
{
	//Get interrupt number and dispatch
	if(iContext.intrNum < 0x30)
	{
		//Get dispatch table entry
		IntrDispatchEntry * intrEntry = &IntrDispatchTable[iContext.intrNum];

		//Execute handler chains
		do
		{
			if(intrEntry->handler == NULL)
			{
				break;
			}

			intrEntry->handler(&iContext);
			intrEntry = intrEntry->next;
		}
		while(intrEntry);

		//If it's a hardware interrupt, allow them to be fired again
		if(iContext.intrNum >= 0x20)
		{
			//Slave PIC
			if(iContext.intrNum >= 0x28)
			{
				outb(0xA0, 0x20);
			}

			//Master PIC
			outb(0x20, 0x20);
		}
	}
	else if(iContext.intrNum == 0x42)
	{
		//Dispatch as system call
#warning Implement Syscalls
	}
	else
	{
		//Invalid interrupt
		Panic("Invalid interrupt encountered: %u", iContext.intrNum);
	}

	//Post-interrupt user mode handlers
	if(iContext.cs == 0x1B)
	{
		//Yield if run out of time
		if(TimerShouldPreempt())
		{
			ProcYield();
		}

		//Handle signals
		ProcSignalHandler(&iContext);
	}
}

//Interrupt registering
// Registers the given IRQ with an interrupt handler
bool IntrRegister(int irq, int flags, void (* handler)(IntrContext *))
{
	//Check IRQ range
	if(irq < 0 || irq > 15)
	{
		PrintLog(Error, "IntrRegister: Attempt to register IRQ out of range");
		return false;
	}

	//If handler is empty, quickly process
	IntrDispatchEntry * intrEntry = &IntrDispatchTable[0x20 + irq];

	if(IntrDispatchTable[0x20 + irq].handler != NULL)
	{
		//Refuse if entry is in used and someone doesn't want to share
		if(!(flags & INTR_SHARED)  || !(intrEntry->flags & INTR_SHARED))
		{
			//Interrupt rejected
			return false;
		}

		//Allocate new entry
		IntrDispatchEntry * newEntry = MAlloc(sizeof(IntrDispatchEntry));

		//Set entry chain properties
		newEntry->next = intrEntry->next;
		intrEntry->next = newEntry;

		//We're dealing with this entry now
		intrEntry = newEntry;
	}
	else
	{
		//New handler, enable in PIC
		if(irq > 8)
		{
			outb(0xA1, inb(0xA1) & ~(1 << (irq - 8)));
		}
		else
		{
			outb(0x21, inb(0x21) & ~(1 << irq));
		}

		intrEntry->next = NULL;
	}

	//Copy data to entry
	intrEntry->handler = handler;
	intrEntry->flags = flags;
	return true;
}

// Unregisters an IRQ
bool IntrUnRegister(int irq, void (* handler)(IntrContext *))
{
	//Check IRQ range
	if(irq < 0 || irq > 15)
	{
		PrintLog(Error, "IntrUnRegister: Attempt to unregister IRQ out of range");
		return false;
	}

	//Find handler in IRQ specified
	IntrDispatchEntry * intrEntry = &IntrDispatchTable[0x20 + irq];
	IntrDispatchEntry * prevEntry = NULL;

	// Search through handlers list
	while(intrEntry && intrEntry->handler != handler)
	{
		prevEntry = intrEntry;
		intrEntry = intrEntry->next;
	}

	//If the entry is NULL, the handler does not exist
	if(intrEntry == NULL)
	{
		return false;
	}

	//If previous entry is NULL, copy next entry to first
	if(prevEntry == NULL)
	{
		//Is there another entry?
		if(intrEntry->next)
		{
			IntrDispatchEntry * nextEntry = intrEntry->next;

			//Copy next entry contents to first
			MemCpy(intrEntry, nextEntry, sizeof(IntrDispatchEntry));

			//Free unused entry
			MFree(nextEntry);
		}
		else
		{
			//No more entries, wipe dispatch table
			MemSet(intrEntry, 0, sizeof(IntrDispatchEntry));

			//Block in PIC
			if(irq > 8)
			{
				outb(0xA1, inb(0xA1) | (1 << (irq - 8)));
			}
			else
			{
				outb(0x21, inb(0x21) | (1 << irq));
			}
		}
	}
	else
	{
		//Join previous's next pointer to this entry's next pointer
		prevEntry->next = intrEntry->next;

		//Free current entry
		MFree(intrEntry);
	}

	return true;
}
