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

//Process management functions

// Process hash table
static HashTable hTableProcess;
static HashTable hTableThread;

//Reapers
static void ProcReapProcess(ProcProcess * process);
static void ProcReapThread(ProcThread * thread);
static void ProcDisownChildren(ProcProcess * process);

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
ProcProcess * ProcCreateProcess(const char * name, ProcProcess * parent)
{
	//
}

//Creates a new thread in a process
ProcThread * ProcCreateThread(const char * name, ProcProcess * process,
								void * startAddr, void * stackPtr)
{
	//
}

//Creates a new kernel thread
ProcThread * ProcCreateKernelThread(const char * name, void * startAddr)
{
	//
}

//Exits the current process with the given error code
void NORETURN ProcExitProcess(unsigned int exitCode)
{
	//If this is not the last thread, kill the others and then kill
	if(ProcCurrProcess->children.next != ProcCurrProcess->children.prev)
	{
#warning Send SIGKILL to all other threads

		ProcExitThread(0);
	}
	else
	{
		//Exit this process and this thread
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
	list_for_each_entry_safe(thread, threadTmp, process->threads, threadSibling)
	{
		//Reap thread
		ProcReapThread(thread);
	}

	//Any child processes should be inherited by init
	ProcDisownChildren(process);

	//

}

//Disowns this process's children
static void ProcDisownChildren(ProcProcess * process)
{
	ProcProcess * child, *childTmp;
	list_for_each_entry_safe(child, childTmp, process->children, processSibling)
	{
#warning child should inherit from init
	}
}

//Exits the current thread with the given error code
// If this is the final thread, this will also exit the current process with status 0
void NORETURN ProcExitThread(unsigned int exitCode)
{
	//If this is the last thread, pass on to ProcExitProcess
	if(ProcCurrProcess->children.next == ProcCurrProcess->children.prev)
	{
		//The exit code is already set
		ProcExitProcess(ProcCurrProcess->exitCode);
	}
	else
	{
		//Exit this thread
		// Notify other threads
#warning Notify others

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
