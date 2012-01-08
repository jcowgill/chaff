/*
 * scheduler.c
 *
 *  Created on: 12 Dec 2010
 *      Author: James
 */

#include "chaff.h"
#include "process.h"
#include "processInt.h"

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
	if(ListHeadIsEmpty(&threadQueue))
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
		}

		//Set current thread
		ProcThread * oldThread = ProcCurrThread;
		ProcCurrThread = newThread;
		ProcCurrProcess = newThread->parent;

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
	if(ListHeadIsEmpty(&threadQueue))
	{
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
// If interruptible is true, the block may be interrupted if the thread receives a signal
//Returns true if the block was interrupted
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
// You should only wake up threads you know exactly where they are blocked since the blocker
//  may not expect to have a thread wake up at any time.
// isSignal is used internally for interrupting threads - do not use it
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
// This deallocates the ProcThread and kernel stack only
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
