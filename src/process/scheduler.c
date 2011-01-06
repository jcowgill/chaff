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
struct list_head threadQueue = LIST_HEAD_INIT(threadQueue);

//Current value of ESP0 in the TSS
extern void * TssESP0;

//Chooses another thread and runs it
static void DoSchedule()
{
	//Pop next thread from list
	ProcThread * newThread;
	if(list_empty(&threadQueue))
	{
		//No next thread, use kernel idle thread
		newThread = ProcIdleThread;
	}
	else
	{
		//Get new thread
		newThread = list_entry(threadQueue.next, ProcThread, schedQueueEntry);
		list_del_init(&newThread->schedQueueEntry);
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
	INIT_LIST_HEAD(&ProcCurrThread->schedQueueEntry);

	//If there are no other thread to run, just return
	if(list_empty(&threadQueue))
	{
		return;
	}
	else
	{
		//Add thread
		list_add_tail(&ProcCurrThread->schedQueueEntry, &threadQueue);

		//Choose another thread
		DoSchedule(false);
	}
}

//Yields the current thread and blocks it until it is woken up using ProcWakeUp
// If interruptible is true, the block may be interrupted if the thread receives a signal
//Returns true if the block was interrupted
bool ProcYieldBlock(bool interruptable)
{
	//Set scheduler data
	ProcCurrThread->schedInterrupted = 0;
	INIT_LIST_HEAD(&ProcCurrThread->schedQueueEntry);
	ProcCurrThread->state = interruptable ? PTS_INTR : PTS_UNINTR;

	//We don't add ourselves to the scheduler list since we're blocked
	DoSchedule(false);

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
			thread->schedInterrupted = 2;
		}

		//Already on queue
		return;

	case PTS_INTR:
		//Interrupt
		thread->schedInterrupted = 1;
		break;

	case PTS_UNINTR:
		//Only wake if not a signal
		if(isSignal)
		{
			return;
		}

		thread->schedInterrupted = 2;
		break;

	default:
#warning What do i do here?
		return;
	}

	//Wake up + add to queue
	thread->state = PTS_RUNNING;
	list_add_tail(&thread->schedQueueEntry, &threadQueue);
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
	Panic("ProcIntSchedulerExitSelf: DoSchedule(true) returned");
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
	Panic("ProcIntSchedulerExitSelf: DoSchedule(true) returned");
}
