/*
 * signal.c
 *
 *  Created on: 12 Jan 2011
 *      Author: James
 */

#include "chaff.h"
#include "process.h"

//Continues a suspended thread
// If the thread isn't suspended, does nothing
static void RemoteContinueThread(ProcThread * thread)
{
	//Continues a remote thread

	//Interrupt thread

#warning TODO
	//Set signal pending
	thread->sigPending |= (1 << (SIGCONT - 1));

	//Remove suspend signals
	thread->sigPending &= ~((1 << (SIGSTOP - 1)) | (1 << (SIGTSTP - 1)) | (1 << (SIGTTIN - 1)) | (1 << (SIGTTOU - 1)));
}

//Suspends the current thread until a RemoteContinueThread is issued
static void SuspendSelf()
{
	//
	#warning TODO
}

//Returns true if the given signal is ignored by a process
// Does NOT check SIGKILL or SIGSTOP for bogus actions
static inline bool SignalIsIgnored(ProcProcess * process, int sigNum)
{
	return process->sigHandlers[sigNum - 1].sa_handler == SIG_IGN ||
	        (process->sigHandlers[sigNum - 1].sa_handler == SIG_DFL && (sigNum == SIGCONT || sigNum == SIGCLD));
}

//Handles custom signal handlers
static void HandleCustomSignal(IntrContext * iContext, ProcSigaction * action, int sigNum)
{
	//Get user mode stack
	// Allocate 14 * 4 bytes for data (see below)
	unsigned int * stack = ((unsigned int *) iContext->esp) - 14;

	//Do memory checks
	#warning TODO User-mode memory checks
	// If checks fail - crash whole process

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

	stack[2] = 0x00EEB858;		//Signal return syscall number replaces EE
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
}

//Sends a signal to the current thread
// If the signal is blocked or ignored - will exit this thread
// May not return
void ProcSignalSendOrCrash(int sigNum)
{
	//Check if ignored or blocked
	if(SignalIsIgnored(ProcCurrProcess, sigNum) ||
		((1 << (sigNum - 1)) & ProcCurrThread->sigBlocked))
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
                //Decrease signal to get bit
                --sigNum;

                //Add to pending signals
                thread->sigPending |= (1 << sigNum);

                //If this is a suspend signal, remove continue signals
                if(sigNum == SIGSTOP)
                {
                	thread->sigPending &= ~(1 << (SIGCONT - 1));
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
                list_for_each_entry(thread, &process->children, threadSibling)
                {
                    ProcSignalSendThread(thread, sigNum);
                }

                break;

            case SIGCONT:
                //Continue all threads, then handle like a normal signal
                list_for_each_entry(thread, &process->children, threadSibling)
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
                    //Decrease signal to get bit
                    --sigNum;

                    //Add to pending signals
                    process->sigPending |= (1 << sigNum);

                    //Wake up an interruptable thread if there are no eligible
                    // running threads
                    ProcThread * eligibleIntr = NULL;

                    list_for_each_entry(thread, &process->children, threadSibling)
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
    thread->sigBlocked &= (1 << (SIGKILL - 1) | 1 << (SIGSTOP - 1));
}

//Sets a signal handling function for the given signal
void ProcSignalSetAction(ProcProcess * process, int sigNum, ProcSigaction newAction)
{
    //Check signal is within limits
    if(sigNum > 0 && sigNum <= SIG_MAX)
    {
        //Force non blocking of SIGKILL and SIGSTOP
        newAction.sa_mask &= (1 << (SIGKILL - 1) | 1 << (SIGSTOP - 1));

        //Copy action to process
        process->sigHandlers[sigNum] = newAction;

		//If the new action is ignore, remove pending signals on all threads
		if(SignalIsIgnored(process, sigNum))
		{
		    int mask = ~(1 << (sigNum - 1));
		
		    //Remove from pending signals
		    process->sigPending &= mask;

		    ProcThread * thread;
            list_for_each_entry(thread, &process->children, threadSibling)
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
    if(sigSet & (SIGKILL - 1))
    {
        //Exit thread
        ProcExitThread(0);
    }

    //Check SIGSTOP
    if(sigSet & (SIGSTOP - 1))
    {
        SuspendSelf();
        goto refreshSigSet;
    }

    do
    {
        //Handle lowest signal
        int sigNum = BitScanForward(sigSet);        //Find signal
        sigSet &= ~(1 << sigNum);                   //Clear bit

        //Get action
        ProcSigaction * action = &ProcCurrProcess->sigHandlers[sigNum];

        //Make signal conatantable
        ++sigNum;

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
                    	if(ProcCurrProcess->sigPending & (1 << (sigNum - 1)))
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

#warning TODO implement signal flags in signalNums.h
}
