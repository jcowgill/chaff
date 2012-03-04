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
#include "mm/kmemory.h"

//TS bit of CR0 register
#define CR0_TS_BIT (1 << 3)

//Initialization words
#define FPU_INIT_CTRL 0x37F
#define MXCSR_INIT 0x1F80

//Current thread registers
// fpuState must be valid for these
static ProcThread * fpuCurrent = NULL;

//FPU States cache
static MemCache * fpuStateCache;

//Setup FPU state slab allocator
void INIT CpuInitLate()
{
	if(CpuHasFxSave())
	{
		fpuStateCache = MemSlabCreate(CPU_EXTRA_FXSAVE, 0);
	}
	else
	{
		fpuStateCache = MemSlabCreate(CPU_EXTRA_FPU, 0);
	}
}

//Does the requested FPU switch
// TS bit must be cleared before this
// Assumes CPU has an FPU (use CpuHasFpu() before this)
static void DoFpuSwitch()
{
	unsigned short * fpuState;

	//Save old registers
	if(fpuCurrent != NULL)
	{
		fpuState = (unsigned short *) fpuCurrent->fpuState;

		//Actually do the save
		if(CpuHasFxSave())
		{
			asm volatile("fxsave %0":"=m" (*fpuState));
		}
		else
		{
			asm volatile("fnsave %0; fwait":"=m" (*fpuState));
		}
	}

	//Ensure the new thread has a usable FPU state
	if(ProcCurrThread->fpuState == NULL)
	{
		//Allocate fpu state and wipe
		ProcCurrThread->fpuState = MemSlabZAlloc(fpuStateCache);

		//Fill with initial FPU control word
		fpuState = (unsigned short *) ProcCurrThread->fpuState;
		fpuState[0] = FPU_INIT_CTRL;

		//Set MXCSR register and mask
		if(CpuHasFxSave())
		{
			fpuState[12] = MXCSR_INIT;

			if(CpuHasDenormalsAreZero)
			{
				fpuState[13] = 0xFFFF;
			}
		}
	}
	else
	{
		fpuState = (unsigned short *) ProcCurrThread->fpuState;
	}

	//Restore registers
	if(CpuHasFxSave())
	{
		asm volatile("fxrstor %0"::"m"(*fpuState));
	}
	else
	{
		//Restore for standard FPUs
		// frstor generates exceptions, so we use frstor with the default control word
		// and then load the separate word with fldcw afterwards
		unsigned short oldFpuCtrlWord = *fpuState;
		*fpuState = FPU_INIT_CTRL;

		asm volatile("frstor %0; fldcw %1"::"m"(*fpuState), "m"(oldFpuCtrlWord));

		*fpuState = oldFpuCtrlWord;
	}

	//Increment number of fpu switches
	ProcCurrThread->fpuSwitches++;
	fpuCurrent = ProcCurrThread;
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
	if(CpuHasFpu() && ProcCurrThread != fpuCurrent)
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
		if(ProcCurrThread != fpuCurrent)
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
	if(thread == fpuCurrent)
	{
		fpuCurrent = NULL;
	}

	//Free state
	if(thread->fpuState)
	{
		MemSlabFree(fpuStateCache, thread->fpuState);
		thread->fpuState = NULL;
	}
}
