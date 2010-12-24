/*
 * interrupt.c
 *
 *  Created on: 11 Dec 2010
 *      Author: James
 */

#include "chaff.h"
#include "memmgr.h"
#include "interrupt.h"
#include "inlineasm.h"

//Interrupt gate structure
typedef struct
{
	unsigned int offLow : 16;
	unsigned int destSegment : 16;

	unsigned int gateInfo: 16;

	unsigned int offHigh : 16;

} IntrGateStruct;

//Interrupt entry points
extern unsigned int * IntrISRList;		//List of entry points for ISRs 0 to 47
void Isr66();

//Gate info values
#define INTR_KERNEL_GATE 0x8E
#define INTR_USER_GATE 0xEE

//Interrupt descriptor table
static IntrGateStruct idt[0x43] __attribute__((aligned(8)));

//Interrupt dispatch table
static void (* IntrDispatchTable[0x30])(IntrContext *) =
	{
		NULL
	};

//Initialise interrupts
void IntrInit()
{
	//Setup descriptor table
	int i;

	IntrDispatchTable[14] = MemPageFaultHandler;

	// 0 to 2 (kernel only)
	for(i = 0; i <= 2; ++i)
	{
		idt[i].offLow = IntrISRList[i] & 0xFFFF;
		idt[i].offHigh = IntrISRList[i] >> 16;
		idt[i].destSegment = 0x08;
		idt[i].gateInfo = INTR_KERNEL_GATE;
	}

	// 3 to 5 (user and kernel)
	for(; i <= 5; ++i)
	{
		idt[i].offLow = IntrISRList[i] & 0xFFFF;
		idt[i].offHigh = IntrISRList[i] >> 16;
		idt[i].destSegment = 0x08;
		idt[i].gateInfo = INTR_USER_GATE;
	}

	// 6 to 47 (kernel only)
	for(; i <= 47; ++i)
	{
		idt[i].offLow = IntrISRList[i] & 0xFFFF;
		idt[i].offHigh = IntrISRList[i] >> 16;
		idt[i].destSegment = 0x08;
		idt[i].gateInfo = INTR_KERNEL_GATE;
	}

	// 66 Syscall
	idt[i].offLow = (unsigned int) Isr66 & 0xFFFF;
	idt[i].offHigh = (unsigned int) Isr66 >> 16;
	idt[i].destSegment = 0x08;
	idt[i].gateInfo = INTR_USER_GATE;

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

	outb(0x21, 0xFF);		//Disable all IO interrupts until later
	outb(0xA1, 0xFF);
}

//Handle interrupts
void IntrHandler(IntrContext iContext)
{
	//Get interrupt number and dispatch
	if(iContext.intrNum < 0x30)
	{
		//Dispatch using dispatch table
		IntrDispatchTable[iContext.intrNum](&iContext);

		//If it's a hardware interrupt, allow them to be fired again
		if(iContext.intrNum >= 0x20)
		{
			//Slave PIC
			if(iContext.intrNum >= 0x28)
			{
				outb(0xA0, 0x20);
			}

			//Master PIC
			outb(0xA0, 0x20);
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

	//Handle signals
#warning Check for signals here
}
