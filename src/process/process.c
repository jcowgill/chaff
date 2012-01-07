/*
 * process.c
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

//Process management functions

// Process hash table
static HashTable hTableProcess;
static HashTable hTableThread;

static unsigned int processNextID;
static unsigned int threadNextID;

//Reapers
static void ProcReapProcess(ProcProcess * process);
static void ProcReapThread(ProcThread * thread);
static void ProcDisownChildren(ProcProcess * process);

//Raw thread creator
static ProcThread * ProcCreateRawThread(const char * name, ProcProcess * parent, bool withStack);

//Global processes and threads
ProcProcess ProcKernelProcessData;
ProcThread * ProcIdleThread;
ProcThread * ProcInterruptsThread;

//Initialise global processes and threads
void ProcInit()
{
	//Create kernel process
	// Malloc need this so we must do it with no dynamic memory
	ProcKernelProcessData.hItem.id = 0;
	HashTableInsert(hTableProcess, &ProcKernelProcessData.hItem);

	ProcKernelProcessData.name = "kernel";
	ProcKernelProcessData.memContext = MemKernelContext;

	INIT_LIST_HEAD(&ProcKernelProcessData.threads);
	INIT_LIST_HEAD(&ProcKernelProcessData.processSibling);
	INIT_LIST_HEAD(&ProcKernelProcessData.children);

	//Set as current process
	ProcCurrProcess = ProcKernelProcess;

	//Create idle thread
	ProcIdleThread = ProcCreateKernelThread("idle", ProcIntIdleThread, NULL);

	//Create interrupts thread
	// This needs no stack since it isn't run
	ProcInterruptsThread = ProcCreateRawThread("interrupts", ProcKernelProcess, false);
}

//Gets a process from the given ID or returns NULL if the process doesn't exist
ProcProcess * ProcGetProcessByID(unsigned int pid)
{
	HashItem * item = HashTableFind(hTableProcess, pid);

	if(item)
	{
		return HASHT_ENTRY(item, ProcProcess, hItem);
	}
	else
	{
		return NULL;
	}
}

//Gets a thread from the given ID or returns NULL if the thread doesn't exist
ProcThread * ProcGetThreadByID(unsigned int tid)
{
	HashItem * item = HashTableFind(hTableThread, tid);

	if(item)
	{
		return HASHT_ENTRY(item, ProcThread, hItem);
	}
	else
	{
		return NULL;
	}
}

//Creates a completely empty process from nothing
// The memory context is left blank and must be created manually
// No threads are added to the process either
ProcProcess * ProcCreateProcess(const char * name, ProcProcess * parent)
{
	//Allocate process
	ProcProcess * process = MAlloc(sizeof(ProcProcess));

	//Zero out
	MemSet(process, 0, sizeof(ProcProcess));

	//Allocate process id
	do
	{
		process->hItem.id = processNextID++;
	}
	while(!HashTableInsert(hTableProcess, &process->hItem));

	//Set process parent
	if(parent == NULL)
	{
		//Null parent with no siblings
		process->parent = NULL;
		INIT_LIST_HEAD(&process->processSibling);
	}
	else
	{
		//Add as sibling
		process->parent = parent;
		list_add_tail(&process->processSibling, &parent->children);
	}

	//Init blank heads
	INIT_LIST_HEAD(&process->children);
	INIT_LIST_HEAD(&process->threads);

	//Set name
	process->name = StrDup(name);

	//Other properties are zeroed out above

	return process;
}

//Creates new thread with the given name and process
// The kernel stack is allocated and wiped if withStack is specified
static ProcThread * ProcCreateRawThread(const char * name, ProcProcess * parent, bool withStack)
{
	//Allocate thread
	ProcThread * thread = MAlloc(sizeof(ProcThread));

	//Zero out
	MemSet(thread, 0, sizeof(ProcThread));

	//Allocate thread id
	do
	{
		thread->hItem.id = threadNextID++;
	}
	while(!HashTableInsert(hTableThread, &thread->hItem));

	//Set thread parent
	thread->parent = parent;
	INIT_LIST_HEAD(&thread->threadSibling);
	list_add_tail(&thread->threadSibling, &parent->threads);

	//Set thread name
	thread->name = StrDup(name);

	//Set startup state
	thread->state = PTS_STARTUP;

	//Initialise scheduler head
	INIT_LIST_HEAD(&thread->schedQueueEntry);

	//Allocate kernel stack
	if(withStack)
	{
		thread->kStackBase = MAlloc(PROC_KSTACK_SIZE);
		MemSet(thread->kStackBase, 0, PROC_KSTACK_SIZE);
	}

	//Kernel stack is setup by caller
	return thread;
}

//Creates a new thread in a process
// This is a user-mode function - the start addresses and stack pointer are USER MODE
ProcThread * ProcCreateThread(const char * name, ProcProcess * process,
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
// startAddr is a kernel mode pointer
// arg is a user defined argument to the function
ProcThread * ProcCreateKernelThread(const char * name, int (* startAddr)(void *), void * arg)
{
#warning This causes kernel thread zombies to be created
#warning TODO thread local storage

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

//Waits for a child process to exit
// id
//	  >1, only the given pid
//	  -1, any child process
//	   0, any process from current process group
//	  <1, any process from given (negated) process group
// exitCode = pointer to where to write exit code to (must be kernel mode)
// options = one of the wait options above
//Returns
// the process id on success,
// 0 if WNOHANG was given and there are no waitable processes,
// negative error code on an error
int ProcWaitProcess(int id, unsigned int * exitCode, int options)
{
	ProcProcess * chosenOne = NULL;
	bool found = false;
	bool interrupted = false;

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
		if(list_empty(&ProcCurrProcess->children))
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
			list_for_each_entry(chosenOne, &ProcCurrProcess->children, processSibling)
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

	unsigned int pid = chosenOne->hItem.id;

	//Reap chosen thread and return
	ProcReapProcess(chosenOne);
	return pid;
}

//Waits for a thread sibling to exit
// id
//	  >1, only the given pid
//	  -1, any child thread
// options = one of the wait options above
//Returns
// the thread id on success,
// 0 if WNOHANG was given and there are no waitable threads,
// negative error code on an error
int ProcWaitThread(int id, unsigned int * exitCode, int options)
{
	ProcThread * chosenOne = NULL;
	bool found = false;
	bool interrupted = false;

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
			list_for_each_entry(chosenOne, &ProcCurrProcess->threads, threadSibling)
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

	unsigned int tid = chosenOne->hItem.id;

	//Reap chosen thread and return
	ProcReapThread(chosenOne);
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
		list_for_each_entry(thread, &ProcCurrProcess->threads, threadSibling)
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
		if(ProcCurrProcess->memContext != NULL)
		{
#warning Do we really want this. MemContext ref counting anyone (also update ProcReapProcess)
			MemContextSwitchTo(MemKernelContext);
			MemContextDelete(ProcCurrProcess->memContext);

			ProcCurrProcess->memContext = NULL;
		}

		// Zombieize
		ProcCurrProcess->zombie = true;

		// Notify parent process
		ProcThread * thread;
		list_for_each_entry(thread, &ProcCurrProcess->parent->threads, threadSibling)
		{
			if(thread->state == PTS_INTR && thread->waitMode == PWM_PROCESS)
			{
				//Wake up
				ProcWakeUp(thread);
			}
		}

		// Send SIGCLD signal
		ProcSignalSendProcess(ProcCurrProcess->parent, SIGCLD);

		// Exit thread
		ProcIntSchedulerExitSelf();
	}
}

//Reaps the given process
static void ProcReapProcess(ProcProcess * process)
{
	//Do not reap the current process
	if(ProcCurrProcess == process)
	{
		Panic("ProcReapProcess: Cannot reap the current process");
	}

	//Reap any existing threads
	ProcThread * thread, *threadTmp;
	list_for_each_entry_safe(thread, threadTmp, &process->threads, threadSibling)
	{
		//Reap thread
		ProcReapThread(thread);
	}

	//Any child processes should be inherited by the kernel
	ProcDisownChildren(process);

	//Free process memory context if it still exists
	if(process->memContext != NULL)
	{
		MemContextDelete(process->memContext);
	}

	//Remove as one of the parent's children
	list_del(&process->processSibling);

	//Remove from hashtable
	HashTableRemove(hTableProcess, process->hItem.id);

	//Free name
	MFree(process->name);

	//Free process
	MFree(process);
}

//Disowns this process's children
static void ProcDisownChildren(ProcProcess * process)
{
	ProcProcess * child;
	ProcProcess * childTmp;

	list_for_each_entry_safe(child, childTmp, &process->children, processSibling)
	{
		list_del_init(&child->processSibling);
		list_add_tail(&child->processSibling, &ProcKernelProcess->children);
	}
}

//Exits the current thread with the given error code
// If this is the final thread, this will also exit the current process with status 0
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
		// Set exit code
		ProcCurrThread->exitCode = exitCode;

		// Notify other waiting threads
		ProcThread * thread;
		list_for_each_entry(thread, &ProcCurrProcess->threads, threadSibling)
		{
			if(thread->state == PTS_INTR && thread->waitMode == PWM_THREAD)
			{
				//Wake up thread
				ProcWakeUp(thread);
			}
		}

#warning If we do other stuff here, review ProcExitProcess
		// Exit thread - other stuff is freed when the thread is reaped
		ProcIntSchedulerExitSelf();
	}
}

//Reaps the given thread
static void ProcReapThread(ProcThread * thread)
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

	//Remove from hash table
	HashTableRemove(hTableThread, thread->hItem.id);

	//Remove from thread list
	list_del(&thread->threadSibling);

	//Free kernel stack
	MFree(thread->kStackBase);

	//Free thread name
	MFree(thread->name);

	//Free thread structure
	MFree(thread);
}
