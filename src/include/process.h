/*
 * process.h
 *
 *  Created on: 12 Dec 2010
 *      Author: James
 */

#ifndef PROCESS_H_
#define PROCESS_H_

//Process, scheduler and signal function
//

#include "list.h"
#include "htable.h"
#include "memmgr.h"

//Thread state
typedef enum
{
	PTS_STARTUP,		//Thread has been created by has never been on the scheduler queue
	PTS_RUNNING,		//Thread is running or queued to be run

	PTS_INTR,			//Interruptible wait state
	PTS_UNINTR,			//Uninterruptible wait state

	PTS_ZOMBIE,			//Thread has ended and is not a zombie

} ProcThreadState;

//Process and thread structures
struct SProcProcess;
struct SProcThread;

struct SProcProcess
{
	//Hash map item
	HashItem hItem;

	//Process and thread hierarchy
	struct SProcProcess * parent;
	struct list_head processSibling;		//Sibling
	struct list_head children;				//Head
	struct list_head threads;				//Head

	//Zombie status
	bool zombie;

	//Name of process
	// This must be unique to this process since it will be MFreed
	char * name;

	//Process exit code - this is read only
	unsigned int exitCode;

	//Process memory context
	MemContext * memContext;

};

struct SProcThread
{
	//Hash map item
	HashItem hItem;

	//Parent process
	struct SProcProcess * parent;
	struct list_head threadSibling;			//Sibling

	//Exit code
	unsigned int exitCode;

	//Name of thread
	// This must be unique to this process since it will be MFreed
	char * name;

	//Thread state
	ProcThreadState state;

	//Scheduler stuff (don't change this)
	struct list_head schedQueueEntry;		//Place in scheduler queue
	int schedInterrupted;					//Weather thread was interrupted or not
	void * kStackPointer;					//Current pointer to kernel stack
	void * kStackBase;						//Base of kernel stack

};

#define PROC_KSTACK_SIZE 0x1000		//4KB Kernel Stack

typedef struct SProcProcess ProcProcess;
typedef struct SProcThread ProcThread;

//Scheduler functions

//Currently running process and thread
// These MUST NOT BE MODIFIED
extern ProcProcess * ProcCurrProcess;
extern ProcThread * ProcCurrThread;

//Yields the current thread temporarily so that other threads can run
void ProcYield();

//Yields the current thread and blocks it until it is woken up using ProcWakeUp
// If interruptible is true, the block may be interrupted if the thread receives a signal
//Returns true if the block was interrupted
bool ProcYieldBlock(bool interruptable);

//Used to interrupt threads - do not use
void ProcWakeUpSig(ProcThread * thread, bool isSignal);

//Wakes up a thread from a block
// You should only wake up threads you know exactly where they are blocked since the blocker
//  may not expect to have a thread wake up at any time.
static inline void ProcWakeUp(ProcThread * thread)
{
	ProcWakeUpSig(thread, false);
}


//Process management functions
ProcProcess * ProcGetProcessByID(unsigned int pid);
ProcThread * ProcGetThreadByID(unsigned int tid);

//Creates a completely empty process from nothing
// The memory context is left blank and must be created manually
// No threads are added to the process either
ProcProcess * ProcCreateProcess(const char * name, ProcProcess * parent);

//Exits the current process with the given error code
void NORETURN ProcExitProcess(unsigned int exitCode);

//Creates a new thread in a process
// This is a user-mode function - the start addresses and stack pointer are USER MODE
// You must wake up the created thread with ProcWakeUp
ProcThread * ProcCreateThread(const char * name, ProcProcess * process, void (* startAddr)(), void * stackPtr);

//Creates a new kernel thread
// startAddr is a kernel mode pointer
// arg is a user defined argument to the function
// You must wake up the created thread with ProcWakeUp
ProcThread * ProcCreateKernelThread(const char * name, int (* startAddr)(void *), void * arg);

//Exits the current thread with the given error code
// If this is the final thread, this will also exit the current process with status 0
// This works for both user and kernel threads
void NORETURN ProcExitThread(unsigned int exitCode);

//Global processes and threads
extern ProcProcess ProcKernelProcessData;
extern ProcThread * ProcIdleThread;
extern ProcThread * ProcInterruptsThread;
#define ProcKernelProcess (&ProcKernelProcessData)

//BOOT CODE
//Initialise global processes and threads
void ProcInit();

//Exits the boot code to continue running threads as normal
void NORETURN ProcExitBootMode();

#endif /* PROCESS_H_ */
