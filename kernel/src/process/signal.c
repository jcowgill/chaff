/*
 * signal.c
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
 *  Created on: 12 Jan 2011
 *      Author: James
 */

#include "chaff.h"
#include "process.h"

#define SIGNAL_INDEX(num) ((num) - 1)

//Continues a suspended thread
// If the thread isn't suspended, does nothing
static void RemoteContinueThread(ProcThread * thread)
{
	//Continues a remote thread
	if(thread->state != PTS_INTR)
	{
		//Already running
		return;
	}

	//Set signal pending
	thread->sigPending |= (1 << SIGNAL_INDEX(SIGCONT));

	//Remove suspend signals
	thread->sigPending &= ~((1 << SIGNAL_INDEX(SIGSTOP)) | (1 << SIGNAL_INDEX(SIGTSTP)) |
			(1 << SIGNAL_INDEX(SIGTTIN)) | (1 << SIGNAL_INDEX(SIGTTOU)));

	//Interrupt thread
	ProcWakeUpSig(thread, true);
}

//Suspends the current thread until a RemoteContinueThread is issued
static void SuspendSelf()
{
	//Block all signals except SIGKILL and SIGCONT
	ProcSigSet maskBefore = ProcCurrThread->sigBlocked;
	ProcSignalSetMask(ProcCurrThread, SIG_SETMASK, (~0) & ~(1 << SIGNAL_INDEX(SIGCONT)));

	//Perform interruptable wait
	for(;;)
	{
		if(!ProcYieldBlock(true))
		{
			PrintLog(Error, "ProcSignalHandler: Process woken up while not waiting");
			#warning TODO print pid
		}

		//Ignore SIGSTOP
		ProcCurrThread->sigPending &= ~(1 << SIGNAL_INDEX(SIGSTOP));

		//Exit on SIGKILL or SIGCONT
		if(ProcCurrThread->sigPending & ((1 << SIGNAL_INDEX(SIGKILL)) | (1 << SIGNAL_INDEX(SIGCONT))))
		{
			break;
		}
	}

	//Restore signal mask
	ProcSignalSetMask(ProcCurrThread, SIG_SETMASK, maskBefore);
}

//Returns true if the given signal is ignored by a process
// Does NOT check SIGKILL or SIGSTOP for bogus actions
static inline bool SignalIsIgnored(ProcProcess * process, int sigNum)
{
	return process->sigHandlers[SIGNAL_INDEX(sigNum)].sa_handler == SIG_IGN ||
	        (process->sigHandlers[SIGNAL_INDEX(sigNum)].sa_handler == SIG_DFL && (sigNum == SIGCONT || sigNum == SIGCLD));
}

//Handles custom signal handlers
static void HandleCustomSignal(IntrContext * iContext, ProcSigaction * action, int sigNum)
{
	//Get user mode stack
	// Allocate 14 * 4 bytes for data (see below)
	unsigned int * stack = ((unsigned int *) iContext->esp) - 14;

	//Do memory checks
	if(!MemUserCanReadWrite(stack, 14 * sizeof(unsigned int)))
	{
		ProcExitProcess(-SIGSEGV);
		return;
	}

	//Note: stack refers to the TOP of the stack

	// First add return address and signal parameter
	stack[0] = (unsigned int) &stack[2];
	stack[1] = sigNum;

	// Add signal return code
	/*
	 * Code Below (8 bytes)
	 * -----
	 * 58               pop eax
	 * B8 EE000000		mov eax, 0xEE
	 * CD42				int 42h
	 */
#warning Replace EE with signal return syscall number
	stack[2] = 0x00EEB858;
	stack[3] = 0x42CD0000;

	// Save registers
	stack[4] = iContext->esp;
	stack[5] = iContext->eflags;
	stack[6] = iContext->eip;
	stack[7] = iContext->eax;
	stack[8] = iContext->ecx;
	stack[9] = iContext->edx;
	stack[10] = iContext->ebx;
	stack[11] = iContext->ebp;
	stack[12] = iContext->esi;
	stack[13] = iContext->edi;

	//Set the context pointers
	iContext->esp = (unsigned int) stack;
	iContext->eip = (unsigned int) action->sa_handler;

	//DF should be unset when entering the signal function
	iContext->eflags &= ~(1 << 10);
}

//Called to restore the state of the thread after a signal handler has executed
void ProcSignalReturn(IntrContext * iContext)
{
	//Only allowed from user mode
	if(iContext->cs != 0x1B)
	{
	    PrintLog(Error, "ProcSignalReturn: Can only return from user-mode signals");
	    return;
	}

	//Get user stack pointer
	unsigned int * stack = (unsigned int *) iContext->esp;

	//Do memory checks
	if(!MemUserCanRead(stack, 12 * sizeof(unsigned int)))
	{
		//Cannot read from stack
		ProcSignalSendOrCrash(SIGSEGV);
		return;
	}

	//Stack is at position 2 in HandleCustomSignal

	//Restore registers
	iContext->esp = stack[2];
	iContext->eflags = (stack[3] & 0xCFF) | 0x200;
		//This prevents privileged flags from being set
		// and forces IF to be set
	iContext->eip = stack[4];
	iContext->eax = stack[5];
	iContext->ecx = stack[6];
	iContext->edx = stack[7];
	iContext->ebx = stack[8];
	iContext->ebp = stack[9];
	iContext->esi = stack[10];
	iContext->edi = stack[11];
}

//Sends a signal to the current thread
// If the signal is blocked or ignored - will exit this thread
// May not return
void ProcSignalSendOrCrash(int sigNum)
{
	//Check if ignored or blocked
	if(SignalIsIgnored(ProcCurrProcess, sigNum) ||
		((1 << SIGNAL_INDEX(sigNum)) & ProcCurrThread->sigBlocked))
	{
	    //Kill self
	    ProcExitProcess(-sigNum);
	}
	else
	{
	    ProcSignalSendThread(ProcCurrThread, sigNum);
	}
}

//Send a signal to the given thread
// This will not redirect SIGKILL, SIGSTOP or SIGCONT to the whole process however
void ProcSignalSendThread(ProcThread * thread, int sigNum)
{
    //Check signal is within limits
    if(sigNum > 0 && sigNum <= SIG_MAX)
    {
        //If this is a SIGCONT, continue the thread
        if(sigNum == SIGCONT)
        {
            RemoteContinueThread(thread);

            //SIGCONT, when sent to threads, never notifies anyone
        }
        else
        {
            //Ignore request is the handler is SIG_IGN
            // Force if SIGKILL, SIGSTOP
            if(!SignalIsIgnored(thread->parent, sigNum) || sigNum == SIGKILL || sigNum == SIGSTOP)
            {
                //Add to pending signals
                thread->sigPending |= (1 << SIGNAL_INDEX(sigNum));

                //If this is a suspend signal, remove continue signals
                if(sigNum == SIGSTOP)
                {
                	thread->sigPending &= ~(1 << SIGNAL_INDEX(SIGCONT));
                }

                //If the thread has pending signals, wake up
                if(thread->state == PTS_INTR && ProcSignalIsPending(thread))
                {
                    ProcWakeUpSig(thread, true);
                }
            }
        }
    }
}

//Send a signal to the given process
void ProcSignalSendProcess(ProcProcess * process, int sigNum)
{
    //Check signal is within limits
    if(sigNum > 0 && sigNum <= SIG_MAX)
    {
        ProcThread * thread;

        //Some signals are special and handled separately
        switch(sigNum)
        {
            case SIGKILL:
                //Set exit code
                process->exitCode = -SIGKILL;

            case SIGSTOP:
                //Sent to all threads instead of the process
                ListForEachEntry(thread, &process->children, threadSibling)
                {
                    ProcSignalSendThread(thread, sigNum);
                }

                break;

            case SIGCONT:
                //Continue all threads, then handle like a normal signal
            	ListForEachEntry(thread, &process->children, threadSibling)
                {
                	RemoteContinueThread(thread);
                }

                //Fallthrough to default

            default:
                //Normal process signal

                //Ignore request is the handler is SIG_IGN
                // Force if SIGKILL, SIGSTOP
                if(!SignalIsIgnored(process, sigNum))
                {
                    //Add to pending signals
                    process->sigPending |= (1 << SIGNAL_INDEX(sigNum));

                    //Wake up an interruptable thread if there are no eligible
                    // running threads
                    ProcThread * eligibleIntr = NULL;

                    ListForEachEntry(thread, &process->children, threadSibling)
                    {
                        //Check thread
                        if(!(thread->sigBlocked & (1 << sigNum)))
                        {
                            //Eligible, is running?
                            if(thread->state == PTS_RUNNING)
                            {
                                //Good, this will handle the signal
                                break;
                            }
                            else if(eligibleIntr == NULL && thread->state == PTS_INTR)
                            {
                                //Interruptible, save if we can't find a running one
                                eligibleIntr = thread;
                            }
                        }
                    }

                    //Wake eligible intr if exists
                    if(eligibleIntr)
                    {
                        ProcWakeUpSig(eligibleIntr, true);
                    }
                }
                break;
        }
    }
}

//Changes the signal mask for a given thread
// how is one of SIG_BLOCK, SIG_UNBLOCK or SIG_SETMASK
//
// In signal sets, the bit for a signal corresponds to the signal - 1,
//  eg SIGHUP (number 1) uses bit 0
void ProcSignalSetMask(ProcThread * thread, int how, ProcSigSet signalSet)
{
    //Set mask
    switch(how)
    {
        case SIG_BLOCK:
            thread->sigBlocked |= signalSet;
            break;

        case SIG_UNBLOCK:
            thread->sigBlocked &= ~signalSet;
            break;

        case SIG_SETMASK:
            thread->sigBlocked = signalSet;
            break;
    }

    //Force SIGKILL and SIGSTOP to be unblocked
    thread->sigBlocked &= (1 << SIGNAL_INDEX(SIGKILL) | 1 << SIGNAL_INDEX(SIGSTOP));
}

//Sets a signal handling function for the given signal
void ProcSignalSetAction(ProcProcess * process, int sigNum, ProcSigaction newAction)
{
    //Check signal is within limits
    if(sigNum > 0 && sigNum <= SIG_MAX)
    {
        //Force non blocking of SIGKILL and SIGSTOP
        newAction.sa_mask &= (1 << SIGNAL_INDEX(SIGKILL) | 1 << SIGNAL_INDEX(SIGSTOP));

        //Copy action to process
        process->sigHandlers[SIGNAL_INDEX(sigNum)] = newAction;

		//If the new action is ignore, remove pending signals on all threads
		if(SignalIsIgnored(process, sigNum))
		{
		    int mask = ~(1 << SIGNAL_INDEX(sigNum));
		
		    //Remove from pending signals
		    process->sigPending &= mask;

		    ProcThread * thread;
		    ListForEachEntry(thread, &process->children, threadSibling)
            {
                thread->sigPending &= mask;
            }
		}
    }
}

//Delivers pending signals on the current thread
void ProcSignalHandler(IntrContext * iContext)
{
	ProcSigSet sigSet;

	//Must be user-mode
	if(iContext->cs != 0x1B)
	{
	    PrintLog(Error, "ProcSignalHandler: Can only handle user-mode signals");
	    return;
	}

refreshSigSet:

    //Get valid pending signals
    sigSet = (ProcCurrThread->sigPending | ProcCurrProcess->sigPending)
                            & ~(ProcCurrThread->sigBlocked);

    //If there are no signals we're done
    if(sigSet == 0)
    {
        return;
    }

    //Check SIGKILL
    if(sigSet & SIGNAL_INDEX(SIGKILL))
    {
        //Exit thread
        ProcExitThread(0);
    }

    //Check SIGSTOP
    if(sigSet & SIGNAL_INDEX(SIGSTOP))
    {
        SuspendSelf();
        goto refreshSigSet;
    }

    do
    {
        //Handle lowest signal
        int sigNum = BitScanForward(sigSet) + 1;

        //Clear bit from masks
        int sigSetMask = ~(1 << SIGNAL_INDEX(sigNum));
        sigSet &= sigSetMask;
        ProcCurrThread->sigPending &= sigSetMask;
        ProcCurrProcess->sigPending &= sigSetMask;

        //Get action
        ProcSigaction * action = &ProcCurrProcess->sigHandlers[SIGNAL_INDEX(sigNum)];

        //Do action
        switch((int) action->sa_handler)
        {
            case (int) SIG_IGN:
                //Ignore signal
                continue;

            case (int) SIG_DFL:
                //Default action
                switch(sigNum)
                {
                    //Ignore
                    // When changing, update SignalIsIgnored
                    case SIGCLD:
                    case SIGCONT:
                        continue;

                    //Suspend
                    case SIGTSTP:
                    case SIGTTIN:
                    case SIGTTOU:
                    	//Process / thread?
                    	if(ProcCurrProcess->sigPending & (1 << SIGNAL_INDEX(sigNum)))
                    	{
                    		//Whole process signal
                    		ProcSignalSendProcess(ProcCurrProcess, sigNum);
                    	}
                    	
                    	//Suspend self
                	    SuspendSelf();
                     	goto refreshSigSet;

                    //Dump core
                    case SIGQUIT:
                    case SIGILL:
                    case SIGABRT:
                    case SIGTRAP:
                    case SIGBUS:
                    case SIGFPE:
                    case SIGSEGV:

                    //Terminate process
                    default:
                        //Dump core is not implemented, kill self instead
                        ProcExitProcess(-sigNum);
                }


            default:
                //User mode function
                HandleCustomSignal(iContext, action, sigNum);
                return;
        }
        
    } while(sigSet != 0);

#warning TODO implement signal flags in signalNums.h (and update whats implemented)
}
