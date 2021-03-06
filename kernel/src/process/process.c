/*
 * process.c
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
 *  Created on: 19 Dec 2010
 *      Author: James
 */

#include "chaff.h"
#include "process.h"
#include "processInt.h"
#include "htable.h"
#include "timer.h"
#include "errno.h"
#include "io/iocontext.h"
#include "mm/region.h"
#include "cpu.h"
#include "mm/kmemory.h"

//Process management functions

//Process and thread hash tables
static HashTable hTableProcess, hTableThread;

//Simple hash table manipulators
static inline bool ProcHashInsert(ProcProcess * process)
{
	return HashTableInsert(&hTableProcess, &process->hItem, &process->pid, sizeof(unsigned int));
}

static inline bool ThreadHashInsert(ProcThread * thread)
{
	return HashTableInsert(&hTableThread, &thread->hItem, &thread->tid, sizeof(unsigned int));
}

#define PHASH_INSERT(table, field, item) \
	HashTableInsert(&(table), &(item)->hItem, &(item)->field, sizeof(unsigned int))
#define PHASH_FIND(table, field, id) \
	HashTableFind(&(table), &(item)->field, sizeof(unsigned int))

//Next ids to use
static unsigned int processNextID;
static unsigned int threadNextID;

//Changes parent of all children to the kernel
static void ProcDisownChildren(ProcProcess * process);

//Raw thread creator
static ProcThread * ProcCreateRawThread(const char * name, ProcProcess * parent, bool withStack);

//Global processes and threads
ProcProcess ProcKernelProcessData;
ProcThread * ProcIdleThread;

//Process and thread caches
static MemCache * cacheProcess;
static MemCache * cacheThread;

//Initialise global processes and threads
void INIT ProcInit()
{
	//Create SLAB caches
	cacheProcess = MemSlabCreate(sizeof(ProcProcess), 0);
	cacheThread = MemSlabCreate(sizeof(ProcThread), 0);

	//Create kernel process
	// Malloc need this so we must do it with no dynamic memory
	ProcKernelProcessData.pid = 0;
	ProcHashInsert(&ProcKernelProcessData);

	ProcKernelProcessData.name = "kernel";
	ProcKernelProcessData.memContext = MemKernelContext;

	ListHeadInit(&ProcKernelProcessData.threads);
	ListHeadInit(&ProcKernelProcessData.processSibling);
	ListHeadInit(&ProcKernelProcessData.children);

	//Set as current process
	ProcCurrProcess = ProcKernelProcess;

	//Create idle thread
	ProcIdleThread = ProcCreateKernelThread("idle", ProcIntIdleThread, NULL);

	//Create Orphan Reaper Thread
	ProcIntReaperInit();
}

//Gets a process from the given ID or returns NULL if the process doesn't exist
ProcProcess * ProcGetProcessByID(unsigned int pid)
{
	HashItem * item = HashTableFind(&hTableProcess, &pid, sizeof(unsigned int));

	if(item)
	{
		return HashTableEntry(item, ProcProcess, hItem);
	}
	else
	{
		return NULL;
	}
}

//Gets a thread from the given ID or returns NULL if the thread doesn't exist
ProcThread * ProcGetThreadByID(unsigned int tid)
{
	HashItem * item = HashTableFind(&hTableThread, &tid, sizeof(unsigned int));

	if(item)
	{
		return HashTableEntry(item, ProcThread, hItem);
	}
	else
	{
		return NULL;
	}
}

//Creates a completely empty process from nothing
ProcProcess * ProcCreateProcess(const char * name, ProcProcess * parent)
{
	//Allocate process (and zero out)
	ProcProcess * process = MemSlabZAlloc(cacheProcess);

	//Allocate process id
	do
	{
		process->pid = processNextID++;
	}
	while(!ProcHashInsert(process));

	//Set process parent
	if(parent == NULL)
	{
		//Null parent with no siblings
		process->parent = NULL;
		ListHeadInit(&process->processSibling);
	}
	else
	{
		//Add as sibling
		process->parent = parent;
		ListHeadAddLast(&process->processSibling, &parent->children);
	}

	//Init blank heads
	ListHeadInit(&process->children);
	ListHeadInit(&process->threads);

	//Set name
	process->name = StrDup(name, 255);

	//Other properties are zeroed out above

	return process;
}

//Creates new thread with the given name and process
static ProcThread * ProcCreateRawThread(const char * name, ProcProcess * parent, bool withStack)
{
	//Allocate thread (and zero out)
	ProcThread * thread = MemSlabZAlloc(cacheThread);

	//Allocate thread id
	do
	{
		thread->tid = threadNextID++;
	}
	while(!ThreadHashInsert(thread));

	//Set thread parent
	thread->parent = parent;
	ListHeadInit(&thread->threadSibling);
	ListHeadAddLast(&thread->threadSibling, &parent->threads);

	//Set thread name
	thread->name = StrDup(name, 255);

	//Set startup state
	thread->state = PTS_STARTUP;

	//Initialise lists
	ListHeadInit(&thread->schedQueueEntry);
	ListHeadInit(&thread->waitQueue);

	//Give thread a valid tls descriptor
	thread->tlsDescriptor = PROC_NULL_TLS_DESCRIPTOR;

	//Allocate kernel stack
	if(withStack)
	{
		thread->kStackBase = MemPhys2Virt(MemPhysicalAlloc(1, MEM_KERNEL));
		MemSet(thread->kStackBase, 0, PROC_KSTACK_SIZE);
	}

	//Kernel stack is setup by caller
	return thread;
}

//Creates a new thread in a process
ProcThread * ProcCreateUserThread(const char * name, ProcProcess * process,
								void (* startAddr)(), void * stackPtr)
{
	//Create raw thread
	ProcThread * thread = ProcCreateRawThread(name, process, true);

	//Setup kernel stack
	unsigned int * kStackPointer = (unsigned int *) ((unsigned int) thread->kStackBase + PROC_KSTACK_SIZE);
	kStackPointer -= 12;

	kStackPointer[0] = 0;		//Initial edi
	kStackPointer[1] = 0;		//Initial esi
	kStackPointer[2] = 0;		//Initial ebx
	kStackPointer[3] = 0;		//Initial ebp
	kStackPointer[4] = (unsigned int) ProcIntUserThreadEntry;	//Kernel entry point (this performs the switch to user mode)
	kStackPointer[5] = 0;		//Discarded
	kStackPointer[6] = 0;		//Discarded
	kStackPointer[7] = (unsigned int) startAddr;				//User start address
	kStackPointer[8] = 0x1B;	//User code selector
	kStackPointer[9] = 0x202;	//Initial EFLAGS
	kStackPointer[10] = (unsigned int) stackPtr;				//User stack pointrt
	kStackPointer[11] = 0x23;	//User data selector

	thread->kStackPointer = kStackPointer;

	//Return thread
	return thread;
}

//Creates a new kernel thread
ProcThread * ProcCreateKernelThread(const char * name, int (* startAddr)(void *), void * arg)
{
	//Create raw thread
	ProcThread * thread = ProcCreateRawThread(name, ProcKernelProcess, true);

	//Setup kernel stack
	unsigned int * kStackPointer = (unsigned int *) ((unsigned int) thread->kStackBase + PROC_KSTACK_SIZE);
	kStackPointer -= 9;

	kStackPointer[0] = 0;		//Initial edi
	kStackPointer[1] = 0;		//Initial esi
	kStackPointer[2] = 0;		//Initial ebx
	kStackPointer[3] = 0;		//Initial ebp
	kStackPointer[4] = (unsigned int) startAddr;		//Kernel start address
	kStackPointer[5] = 0;		//Discarded
	kStackPointer[6] = 0;		//Discarded
	kStackPointer[7] = (unsigned int) ProcIntKernelThreadReturn;		//Return address from kernel thread
	kStackPointer[8] = (unsigned int) arg;				//Function argument

	thread->kStackPointer = kStackPointer;

	//Return thread
	return thread;
}

//Forks the current process creating a new process which runs at the given location
ProcProcess * ProcFork(void (* startAddr)(), void * userStackPtr)
{
	//Cannot be done by kernel process
	if(ProcCurrProcess == ProcKernelProcess)
	{
		PrintLog(Error, "ProcFork: attempt to fork kernel process");
		return NULL;
	}

	//Create the new process
	ProcProcess * newProc = ProcCreateProcess(ProcCurrProcess->name, ProcCurrProcess);

	//Copy signal handlers
	MemCpy(newProc->sigHandlers, ProcCurrProcess->sigHandlers, sizeof(newProc->sigHandlers));

	//Clone memory context
	newProc->memContext = MemContextClone();

	//Clone IO context
	newProc->ioContext = IoContextClone(ProcCurrProcess->ioContext);

	//Create new thread
	ProcThread * newThread = ProcCreateUserThread(ProcCurrThread->name, newProc, startAddr, userStackPtr);

	//Copy blocked signals and tls descriptor
	newThread->sigBlocked = ProcCurrThread->sigBlocked;
	newThread->tlsDescriptor = ProcCurrThread->tlsDescriptor;

	//Start thread and return new process
	ProcWakeUp(newThread);
	return newProc;
}

//Waits for a child process to exit
int ProcWaitProcess(int id, unsigned int * exitCode, int options)
{
	ProcProcess * chosenOne = NULL;
	bool found = false;
	bool interrupted = false;

	//Not kernel mode
	if(ProcCurrProcess == ProcKernelProcess)
	{
		PrintLog(Error, "ProcWaitProcess: kernel threads cannot wait on other processes");
		return -EPERM;
	}

	//Check valid ids
	if(id < 0 && id != -1)
	{
		return -EINVAL;
	}

	//Process groups not implemented yet
	if(id == 0 || id < -1)
	{
#warning Implement process groups
		return -ENOSYS;
	}

	//ID must be a child process
	if(id > 0)
	{
		chosenOne = ProcGetProcessByID(id);

		if(chosenOne == NULL || chosenOne->parent != ProcCurrProcess)
		{
			return -ECHILD;
		}
	}
	else
	{
		//There must be a child process
		if(ListEmpty(&ProcCurrProcess->children))
		{
			//Noone else
			return -ECHILD;
		}
	}

	//Start check loop
	for(;;)
	{
		//Zombied?
		if(id > 0)
		{
			//Check our process
			found = chosenOne->zombie;
		}
		else
		{
			//Find zombie process
			ListForEachEntry(chosenOne, &ProcCurrProcess->children, processSibling)
			{
				if(chosenOne->zombie)
				{
					//Found a process
					found = true;
					break;
				}
			}
		}

		//Break if we've found one
		if(found)
		{
			break;
		}

		//Allowed to wait
		if(options & WNOHANG)
		{
			return 0;
		}

		if(interrupted)
		{
			return -EINTR;
		}

		//Block
		ProcCurrThread->waitMode = PWM_PROCESS;
		if(ProcYieldBlock(true))
		{
			//Interrupted - but recheck anyway
			interrupted = true;
		}
		ProcCurrThread->waitMode = PWM_NONE;
	}

	//Extract process data
	if(exitCode != NULL)
	{
		*exitCode = chosenOne->exitCode;
	}

	unsigned int pid = chosenOne->pid;

	//Reap chosen thread and return
	ProcIntReapProcess(chosenOne);
	return pid;
}

//Waits for a thread sibling to exit
int ProcWaitThread(int id, unsigned int * exitCode, int options)
{
	ProcThread * chosenOne = NULL;
	bool found = false;
	bool interrupted = false;

	//Not kernel mode
	if(ProcCurrProcess == ProcKernelProcess)
	{
		PrintLog(Error, "ProcWaitThread: kernel threads cannot wait on other threads");
		return -EPERM;
	}

	//Check valid ids
	if(id < 0 && id != -1)
	{
		return -EINVAL;
	}

	//ID must be a thread sibling
	if(id > 0)
	{
		chosenOne = ProcGetThreadByID(id);

		if(chosenOne == NULL || chosenOne->parent != ProcCurrProcess)
		{
			return -ESRCH;
		}
	}
	else
	{
		//Must have some other threads
		if(ProcCurrProcess->threads.next == ProcCurrProcess->threads.prev)
		{
			//Noone else
			return -ESRCH;
		}
	}

	//Start check loop
	for(;;)
	{
		//Zombied?
		if(id > 0)
		{
			//Check our thread
			found = (chosenOne->state == PTS_ZOMBIE);
		}
		else
		{
			//Find zombie thread
			ListForEachEntry(chosenOne, &ProcCurrProcess->threads, threadSibling)
			{
				if(chosenOne->state == PTS_ZOMBIE)
				{
					//Found a thread
					found = true;
					break;
				}
			}
		}

		//Break if we've found one
		if(found)
		{
			break;
		}

		//Allowed to wait
		if(options & WNOHANG)
		{
			return 0;
		}

		if(interrupted)
		{
			return -EINTR;
		}

		//Block
		ProcCurrThread->waitMode = PWM_THREAD;
		if(ProcYieldBlock(true))
		{
			//Interrupted - but recheck anyway
			interrupted = true;
		}
		ProcCurrThread->waitMode = PWM_NONE;
	}

	//Extract thread data
	if(exitCode != NULL)
	{
		*exitCode = chosenOne->exitCode;
	}

	unsigned int tid = chosenOne->tid;

	//Reap chosen thread and return
	ProcIntReapThread(chosenOne);
	return tid;
}

//Exits the current process with the given error code
void NORETURN ProcExitProcess(unsigned int exitCode)
{
	//Set exit code
	ProcCurrProcess->exitCode = exitCode;

	//If this is not the last thread, kill the others and then kill
	if(ProcCurrProcess->threads.next != ProcCurrProcess->threads.prev)
	{
		ProcThread * thread;
		ListForEachEntry(thread, &ProcCurrProcess->threads, threadSibling)
		{
			if(thread != ProcCurrThread)
			{
				ProcSignalSendThread(thread, SIGKILL);
			}
		}

		ProcExitThread(0);
	}
	else
	{
		//Exit this process and this thread
		// Clear alarm
		TimerSetAlarm(0);

		// Disown children
		ProcDisownChildren(ProcCurrProcess);

		// Free memory context
		if(ProcCurrProcess->memContext)
		{
			MemContextSwitchTo(MemKernelContext);
			MemContextDeleteReference(ProcCurrProcess->memContext);

			ProcCurrProcess->memContext = NULL;
		}

		// Free io context
		if(ProcCurrProcess->ioContext)
		{
			IoContextDeleteReference(ProcCurrProcess->ioContext);
			ProcCurrProcess->ioContext = NULL;
		}

		// Zombieize
		ProcCurrProcess->zombie = true;
		ProcCurrThread->state = PTS_ZOMBIE;

		// If the owner is the kernel, auto-reap
		if(ProcCurrProcess->parent == ProcKernelProcess)
		{
			ProcIntReaperAdd(ProcCurrThread);
		}
		else
		{
			// Notify parent process
			ProcThread * thread;
			ListForEachEntry(thread, &ProcCurrProcess->parent->threads, threadSibling)
			{
				if(thread->state == PTS_INTR && thread->waitMode == PWM_PROCESS)
				{
					//Wake up
					ProcWakeUp(thread);
				}
			}

			// Send SIGCLD signal
			ProcSignalSendProcess(ProcCurrProcess->parent, SIGCLD);
		}

		// Exit thread
		ProcIntSelfExit();
	}
}

//Reaps the given process
void ProcIntReapProcess(ProcProcess * process)
{
	//Do not reap running process
	if(!process->zombie)
	{
		Panic("ProcReapProcess: Cannot reap running process");
	}

	//Reap any existing threads
	ProcThread * thread, *threadTmp;
	ListForEachEntrySafe(thread, threadTmp, &process->threads, threadSibling)
	{
		//Reap thread
		ProcIntReapThread(thread);
	}

	//Any child processes should be inherited by the kernel
	ProcDisownChildren(process);

	//Remove as one of the parent's children
	ListDelete(&process->processSibling);

	//Remove from hashtable
	HashTableRemoveItem(&hTableProcess, &process->hItem);

	//Free name
	MemKFree(process->name);

	//Free process
	MemSlabFree(cacheProcess, process);
}

//Disowns this process's children
static void ProcDisownChildren(ProcProcess * process)
{
	ProcProcess * child;
	ProcProcess * childTmp;

	ListForEachEntrySafe(child, childTmp, &process->children, processSibling)
	{
		ListDeleteInit(&child->processSibling);
		ListHeadAddLast(&child->processSibling, &ProcKernelProcess->children);
	}
}

//Exits the current thread with the given error code
void NORETURN ProcExitThread(unsigned int exitCode)
{
	//If this is the last thread, pass on to ProcExitProcess
	if(ProcCurrProcess->threads.next == ProcCurrProcess->threads.prev)
	{
		//The exit code is already set
		ProcExitProcess(ProcCurrProcess->exitCode);
	}
	else
	{
		//Exit this thread
		// Zombieize
		ProcCurrThread->state = PTS_ZOMBIE;
		
		// Set exit code
		ProcCurrThread->exitCode = exitCode;

		// Auto-reap kernel threads
		if(ProcCurrProcess == ProcKernelProcess)
		{
			ProcIntReaperAdd(ProcCurrThread);
		}
		else
		{
			// Notify other waiting threads
			ProcThread * thread;
			ListForEachEntry(thread, &ProcCurrProcess->threads, threadSibling)
			{
				if(thread->state == PTS_INTR && thread->waitMode == PWM_THREAD)
				{
					//Wake up thread
					ProcWakeUp(thread);
				}
			}
		}

		//!!! If we do other stuff here, review ProcExitProcess

		// Exit thread - other stuff is freed when the thread is reaped
		ProcIntSelfExit();
	}
}

//Reaps the given thread
void ProcIntReapThread(ProcThread * thread)
{
	//Do not reap the current thread
	if(ProcCurrThread == thread)
	{
		Panic("ProcReapThread: Cannot reap the current thread");
	}

	//Must be zombie thread
	if(thread->state != PTS_ZOMBIE)
	{
		Panic("ProcReapThread: Cannot reap a thread which is still running");
	}

	//Free FPU state
	CpuFreeFpuState(ProcCurrThread);

	//Remove from hash table
	HashTableRemoveItem(&hTableThread, &thread->hItem);

	//Remove from thread list
	ListDelete(&thread->threadSibling);

	//Free kernel stack
	MemPhysicalFree(MemVirt2Phys(thread->kStackBase), 1);

	//Free thread name
	MemKFree(thread->name);

	//Free thread structure
	MemSlabFree(cacheThread, thread);
}
