/*
 * cpu.c
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
 *  Created on: 18 Feb 2012
 *      Author: James
 */

#include "chaff.h"
#include "inlineasm.h"
#include "process.h"
#include "cpu.h"

#define CR0_TS_BIT (1 << 3)
#define FPU_INIT_CTRL 0x37F

//Current thread registers
// fpuState must be valid for these
ProcThread * CpuFpuCurrRegisters = NULL;

//Does the requested FPU switch
// TS bit must be cleared before this
static void DoFpuSwitch()
{
	//Use new FXSAVE?
	if(CpuHasFxSave())
	{
		//Save old registers
		if(CpuFpuCurrRegisters != NULL)
		{
			asm volatile("fxsave %0":"=m" (CpuFpuCurrRegisters->fpuState));
		}

		//Check if the new thread has an FPU state
		if(ProcCurrThread->fpuState == NULL)
		{
			//Allocate fpu state
#warning Needs to be 16 byte aligned
			ProcCurrThread->fpuState = MAlloc(CPU_EXTRA_FXSAVE);

			//Wipe and fill with initial FPU control
			MemSet(ProcCurrThread->fpuState, 0, CPU_EXTRA_FXSAVE);
			*((unsigned int *) ProcCurrThread->fpuState) = FPU_INIT_CTRL;
		}

		//Restore registers
		asm volatile("fxrstor %0"::"m"(ProcCurrThread->fpuState));
	}
	else if(CpuHasFpu())
	{
		//Save old registers
		if(CpuFpuCurrRegisters != NULL)
		{
			asm volatile("fnsave %0; fwait":"=m" (CpuFpuCurrRegisters->fpuState));
		}

		//Check if the new thread has an FPU state
		if(ProcCurrThread->fpuState == NULL)
		{
			//Allocate fpu state
			ProcCurrThread->fpuState = MAlloc(CPU_EXTRA_FPU);

			//Wipe and fill with initial FPU control
			MemSet(ProcCurrThread->fpuState, 0, CPU_EXTRA_FPU);
			*((unsigned short *) ProcCurrThread->fpuState) = FPU_INIT_CTRL;
		}

		//Restore registers
		// frstor generates exceptions, so we use frstor with the default control word
		// and then load the separate word with fldcw afterwards
		unsigned short oldFpuCtrlWord = *((unsigned short *) ProcCurrThread->fpuState);
		*((unsigned short *) ProcCurrThread->fpuState) = FPU_INIT_CTRL;

		asm volatile("frstor %0; fldcw %1"::"m"(ProcCurrThread->fpuState), "m"(oldFpuCtrlWord));

		*((unsigned short *) ProcCurrThread->fpuState) = oldFpuCtrlWord;
	}
	else
	{
		return;
	}

	//Increment number of fpu switches
	ProcCurrThread->fpuSwitches++;
}

//No Math Coprocessor Exception
void CpuNoFpuException(struct IntrContext * intrContext)
{
	//Not allowed in kernel mode
	if(intrContext->cs == 0x08)
	{
		Panic("CpuNoFpuException: NM FPU exception in kernel mode - the kernel cannot use FPU or SSE");
	}

	//Do switch or raise exception
	if(CpuHasFpu() && ProcCurrThread != CpuFpuCurrRegisters)
	{
		//Clear TS bit
		setCR0(getCR0() & ~CR0_TS_BIT);

		//Do Switch
		DoFpuSwitch();
	}
	else
	{
		//No FPU
		ProcSignalSendOrCrash(SIGFPE);
	}
}

//New task selected
void CpuTaskSwitched()
{
	//Only do FPU stuff if there is an FPU
	if(CpuHasFpu())
	{
		//Clear TS bit
		setCR0(getCR0() & ~CR0_TS_BIT);

		//Are our registers already in the CPU?
		if(ProcCurrThread != CpuFpuCurrRegisters)
		{
			//Should we update immediately?
			if(ProcCurrThread->fpuSwitches >= CPU_FPU_SWITCH_THRESHOLD)
			{
				//Do Switch
				DoFpuSwitch();
			}
			else
			{
				//Set TS bit
				setCR0(getCR0() | CR0_TS_BIT);
			}
		}
	}
}

//Frees the FPU / SSE state for a thread when it is exited
void CpuFreeFpuState(ProcThread * thread)
{
	//If this thread owns the current registers, clear them
	if(thread == CpuFpuCurrRegisters)
	{
		CpuFpuCurrRegisters = NULL;
	}

	//Free state
	if(thread->fpuState)
	{
		MFree(thread->fpuState);
		thread->fpuState = NULL;
	}
}
