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
	struct list_head threads;				//Head (0 threads = Zombie Process)

	//Name of process
	// This must be unique to this process since it will be MFreed
	const char * name;

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

	//Name of thread
	// This must be unique to this process since it will be MFreed
	const char * name;

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

#ifndef NO_SCHED_EXTERN

//Currently running process and thread
// These MUST NOT BE MODIFIED
extern ProcProcess * const ProcCurrProcess;
extern ProcThread * const ProcCurrThread;

#endif

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
ProcProcess * ProcCreateProcess(const char * name, ProcProcess * parent);

//Exits the current process with the given error code
void NORETURN ProcExitProcess(unsigned int exitCode);

//Creates a new thread in a process
ProcThread * ProcCreateThread(const char * name, ProcProcess * process, void * startAddr, void * stackPtr);

//Creates a new kernel thread
ProcThread * ProcCreateKernelThread(const char * name, void * startAddr);

//Exits the current thread with the given error code
// If this is the final thread, this will also exit the current process with status 0
void NORETURN ProcExitThread(unsigned int exitCode);

#endif /* PROCESS_H_ */