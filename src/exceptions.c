/*
 * exceptions.c
 *
 *  Created on: 24 Mar 2011
 *      Author: james
 */

#include "chaff.h"
#include "interrupt.h"
#include "process.h"

#warning TODO print address in panic displays

void IntrExceptMathFault(IntrContext * iContext)
{
	//Divide by zero, No math coprocessor, FPU error
	if(iContext->cs == 0x08)
	{
		Panic("IntrExceptMathFault: Math error in kernel space");
	}

	ProcSignalSendOrCrash(SIGFPE);
}

void IntrExceptMathTrap(IntrContext * iContext)
{
	//Overflow (INTO), Bound error (BOUND)
	if(iContext->cs == 0x08)
	{
		PrintLog(Warning, "IntrExceptMathTrap: Math trap in kernel space");
	}
	else
	{
		ProcSignalSendThread(ProcCurrThread, SIGFPE);
	}
}

void IntrExceptDebugTrap(IntrContext * iContext)
{
	//Step, Breakpoint
	if(iContext->cs == 0x08)
	{
		PrintLog(Warning, "IntrExceptDebugTrap: Debug trap (INT 3) in kernel space");
	}
	else
	{
		ProcSignalSendThread(ProcCurrThread, SIGTRAP);
	}
}

void IntrExceptIllOpcodeFault(IntrContext * iContext)
{
	//Invalid opcode
	if(iContext->cs == 0x08)
	{
		Panic("IntrExceptIllOpcodeFault: Invalid opcode in kernel space");
	}

	ProcSignalSendOrCrash(SIGILL);
}

void IntrExceptProtectionFault(IntrContext * iContext)
{
	//Segment not present, Stack fault, General Protection fault
	if(iContext->cs == 0x08)
	{
		Panic("IntrExceptProtectionFault: Protection fault in kernel space");
	}

	ProcSignalSendOrCrash(SIGSEGV);
}

void IntrExceptAlignmentFault(IntrContext * iContext)
{
	//Alignment check
	IGNORE_PARAM iContext;
	ProcSignalSendOrCrash(SIGBUS);
}

void IntrExceptError(IntrContext * iContext)
{
	//Error exceptions
	IGNORE_PARAM iContext;
	Panic("IntrExceptError: Fatal CPU error");
#warning TODO extra info
}
