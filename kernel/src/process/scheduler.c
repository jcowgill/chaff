/*
 * scheduler.c
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
 *  Created on: 12 Dec 2010
 *      Author: James
 */

#include "chaff.h"
#include "process.h"
#include "processInt.h"
#include "timer.h"
#include "mm/region.h"
#include "cpu.h"

//Scheduler functions

//Currently running process and thread
ProcProcess * ProcCurrProcess;
ProcThread * ProcCurrThread;

//Thread queue
static ListHead threadQueue = LIST_INLINE_INIT(threadQueue);

//Special pointers and values
extern void * TssESP0;				//Kernel stack
extern unsigned long long GdtTLS;	//Current TLS descriptor

//Chooses another thread and runs it
static void DoSchedule()
{
	//Pop next thread from list
	ProcThread * newThread;
	if(ListEmpty(&threadQueue))
	{
		//No next thread, use kernel idle thread
		newThread = ProcIdleThread;
	}
	else
	{
		//Get new thread
		newThread = ListEntry(threadQueue.next, ProcThread, schedQueueEntry);
		ListDeleteInit(&newThread->schedQueueEntry);
	}

	//Perform context switch
	if(ProcCurrThread != newThread)
	{
		//Set top of kernel stack in TSS if it's a user mode thread
		if(newThread->parent == ProcKernelProcess)
		{
			TssESP0 = NULL;
		}
		else
		{
			TssESP0 = ((char *) newThread->kStackBase) + PROC_KSTACK_SIZE;

			//Set TLS descriptor
			GdtTLS = newThread->tlsDescriptor;

			//Switch page directories if in a different process
			if(newThread->parent != ProcCurrProcess)
			{
				MemContextSwitchTo(newThread->parent->memContext);
			}

			//Reset thread quantum for user mode thread
			TimerResetQuantum();
		}

		//Set current thread
		ProcThread * oldThread = ProcCurrThread;
		ProcCurrThread = newThread;
		ProcCurrProcess = newThread->parent;

		//FPU Task Switch
		CpuTaskSwitched();

		//Last thing - switch stack
		// Note this function may not return to this position if a new thread is being run
		ProcIntSchedulerSwap(newThread->kStackPointer, &oldThread->kStackPointer);
	}
}

//Yields the current thread temporarily so that other threads can run
void ProcYield()
{
	//Wipe scheduler data
	ProcCurrThread->schedInterrupted = 0;
	ListHeadInit(&ProcCurrThread->schedQueueEntry);

	//If there are no other thread to run, just return
	if(ListEmpty(&threadQueue))
	{
		//Give myself another full quantum
		TimerResetQuantum();
		return;
	}
	else
	{
		//Add thread
		ListHeadAddLast(&ProcCurrThread->schedQueueEntry, &threadQueue);

		//Choose another thread
		DoSchedule(false);
	}
}

//Yields the current thread and blocks it until it is woken up using ProcWakeUp
bool ProcYieldBlock(bool interruptable)
{
	//Return immediately if interruptable and a signal is pending
	if(interruptable && ProcSignalIsPending(ProcCurrThread))
	{
		return true;
	}

	//Set scheduler data
	ProcCurrThread->schedInterrupted = 0;
	ListHeadInit(&ProcCurrThread->schedQueueEntry);
	ProcCurrThread->state = interruptable ? PTS_INTR : PTS_UNINTR;

	//We don't add ourselves to the scheduler list since we're blocked
	DoSchedule();

	//Return whether interrupted
	return ProcCurrThread->schedInterrupted == 1;
}

//Wakes up a thread from a block
void ProcWakeUpSig(ProcThread * thread, bool isSignal)
{
	//Determine what to do depending on thread state
	switch(thread->state)
	{
	case PTS_STARTUP:
		//Just add to queue
		thread->schedInterrupted = 0;
		break;

	case PTS_RUNNING:
		//If this is an uninteruptible wake up, ensure the wake is uninterruptible
		if(!isSignal)
		{
			thread->schedInterrupted = 0;
		}

		//Already on queue
		return;

	case PTS_INTR:
		//Interruptable wake up
		if(isSignal)
		{
			thread->schedInterrupted = 1;
		}
		else
		{
			thread->schedInterrupted = 0;
		}

		break;

	case PTS_UNINTR:
		//Only wake if not a signal
		if(isSignal)
		{
			return;
		}

		thread->schedInterrupted = 0;
		break;

	case PTS_ZOMBIE:
		//Cannot wake zombie
		PrintLog(Critical, "ProcWakeUpSig: Attempt to wake up zombie thread");
		return;

	default:
		//WTF
		Panic("ProcWakeUpSig: Illegal thread state");
		return;
	}

	//Wake up + add to queue
	thread->state = PTS_RUNNING;
	ListHeadAddLast(&thread->schedQueueEntry, &threadQueue);
}

//Removes the current thread from scheduler existence
void NORETURN ProcIntSchedulerExitSelf()
{
	//Set as zombie
	ProcCurrThread->state = PTS_ZOMBIE;

	//Reschedule
	DoSchedule();

	//If we get here, something's gone very wrong
	Panic("ProcIntSchedulerExitSelf: DoSchedule() returned");
}

//Exits the boot code to continue running threads as normal
void NORETURN ProcExitBootMode()
{
	//DoSchedule requires a current thread
	// We use the interrupts thread - shouldn't really be used but i don't care
	// so there
	ProcCurrThread = ProcInterruptsThread;

	//Reschedule
	DoSchedule();

	//If we get here, something's gone very wrong
	Panic("ProcIntSchedulerExitSelf: DoSchedule() returned");
}
