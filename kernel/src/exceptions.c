/*
 * exceptions.c
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
 *  Created on: 24 Mar 2011
 *      Author: james
 */

#include "chaff.h"
#include "interrupt.h"
#include "process.h"

#warning TODO print address in panic displays

void IntrExceptMathFault(IntrContext * iContext)
{
	//Divide by zero, FPU error
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
