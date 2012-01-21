/*
 * process.h
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

#ifndef PROCESS_H_
#define PROCESS_H_

//Process, scheduler and signal functions
//

#include "list.h"
#include "htable.h"
#include "memmgr.h"
#include "interrupt.h"
#include "signalNums.h"

//Thread state
typedef enum
{
	PTS_STARTUP,		//Thread has been created by has never been on the scheduler queue
	PTS_RUNNING,		//Thread is running or queued to be run

	PTS_INTR,			//Interruptible wait state
	PTS_UNINTR,			//Uninterruptible wait state

	PTS_ZOMBIE,			//Thread has ended and is not a zombie

} ProcThreadState;

//Thread wait mode
typedef enum
{
	PWM_NONE,			//Not waiting
	PWM_PROCESS,		//Waiting for a process
	PWM_THREAD,			//Waiting for a thread

} ProcWaitMode;

//Process and thread structures
struct SProcProcess;
struct SProcThread;

typedef struct SProcSigaction
{
	void (* sa_handler)(int);
	ProcSigSet sa_mask;
	int sa_flags;

} ProcSigaction;

struct SProcProcess
{
	//Hash map item
	HashItem hItem;

	//Process and thread hierarchy
	struct SProcProcess * parent;
	ListHead processSibling;		//Sibling
	ListHead children;				//Head
	ListHead threads;				//Head

	//Zombie status
	bool zombie;

	//Name of process
	// This must be unique to this process since it will be MFreed
	char * name;

	//Process exit code - this is read only
	unsigned int exitCode;

	//Process memory context
	MemContext * memContext;

	//Process signal handlers
	/*
	 * Signal handlers are process wide
	 *
	 * There are no signals which kill or stop a particular thread
	 *  When 1 thread or 1 process (no difference) receives SIGKILL or SIGSTOP, the entire process
	 *  is stopped or killed. Anyone for a thr_kill, thr_stop, thr_cont syscall?
	 *
	 * When a signal is sent to a process, only 1 thread will handle it
	 * 	This thread is any random thread which is not blocking the signal
	 * 	The signal will be sent when another signal is unblocked
	 *
	 * 	Threads need to read the process pending and thread pending signals as well
	 */

	struct SProcSigaction sigHandlers[SIG_MAX];

	//Process wide pending signals
	ProcSigSet sigPending;

	//Process alarm
	ListHead * alarmPtr;
};

struct SProcThread
{
	//Hash map item
	HashItem hItem;

	//Parent process
	struct SProcProcess * parent;
	ListHead threadSibling;			//Sibling

	//Exit code
	unsigned int exitCode;

	//Name of thread
	// This must be unique to this process since it will be MFreed
	char * name;

	//Thread state
	ProcThreadState state;
	ProcWaitMode waitMode;

	//Scheduler stuff (don't change this)
	ListHead schedQueueEntry;				//Place in scheduler queue
											// (this is used by the reaper when thread is a zombie)
	int schedInterrupted;					//Weather thread was interrupted or not
	void * kStackPointer;					//Current pointer to kernel stack
	void * kStackBase;						//Base of kernel stack
	unsigned long long tlsDescriptor;		//Thread local storage descriptor for this thread

	//Signal masks
	ProcSigSet sigPending;
	ProcSigSet sigBlocked;
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

//Forks the current process creating a new process which runs at the given location
// This can ONLY be called from a syscall of a process - and the parameters should
// be obtained from the interrupt context
// Returns the new process
//  (the new process is already started)
ProcProcess * ProcFork(void (* startAddr)(), void * userStackPtr);

#define WNOHANG 1		//Causes ProcWait not to block

//Waits for a child process to exit
// id
//	  >1, only the given pid
//	  -1, any child process
//	   0, any process from current process group
//	  <1, any process from given (negated) process group
// exitCode = pointer to where to write exit code to (must be kernel mode)
// options = one of the wait options above
//Kernel threads cannot wait on any process
//Returns
// the process id on success,
// 0 if WNOHANG was given and there are no waitable processes,
// negative error code on an error
int ProcWaitProcess(int id, unsigned int * exitCode, int options);

//Waits for a thread sibling to exit
// id
//	  >1, only the given pid
//	  -1, any child process
// options = one of the wait options above
//Kernel threads cannot wait on any thread
//Returns
// the thread id on success,
// 0 if WNOHANG was given and there are no waitable threads,
// negative error code on an error
int ProcWaitThread(int id, unsigned int * exitCode, int options);

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

//Thread Local Storage

//Null descriptor - prevents all use of the descriptor
// tlsDescriptor is initialized to this
#define PROC_NULL_TLS_DESCRIPTOR (0x0040F20000000000ULL)

//Base descriptor - used as base before modification in ProcTlsCreateDescriptor
#define PROC_BASE_TLS_DESCRIPTOR (0x00C0F60000000000ULL)

//Creates a tls descriptor using the given base pointer as an expand down segment
// Returns PROC_NULL_TLS_DESCRIPTOR is an invalid basePtr is given
unsigned long long ProcTlsCreateDescriptor(unsigned int basePtr) __attribute__((pure));

//Signal functions
// Signal action structure is above

//Sends a signal to the current thread
// If the signal is blocked or ignored - will exit this thread
// May not return
void ProcSignalSendOrCrash(int sigNum);

//Send a signal to the given thread
// This will not redirect SIGKILL, SIGSTOP or SIGCONT to the whole process however
void ProcSignalSendThread(ProcThread * thread, int sigNum);

//Send a signal to the given process
void ProcSignalSendProcess(ProcProcess * process, int sigNum);

//Changes the signal mask for a given thread
// how is one of SIG_BLOCK, SIG_UNBLOCK or SIG_SETMASK
//
// In signal sets, the bit for a signal corresponds to the signal - 1,
//  eg SIGHUP (number 1) uses bit 0
void ProcSignalSetMask(ProcThread * thread, int how, ProcSigSet signalSet);

//Sets a signal handling function for the given signal
void ProcSignalSetAction(ProcProcess * process, int sigNum, ProcSigaction newAction);

//Waits for a signal to be sent to this thread and returns when that happens
// If SIGKILL is sent to this thread, this function will return but a later ProcSignalHandler will kill the thread
// When in this function, the thread will receive process wide signals if there are no running threads to receive them
static inline void ProcSignalWait()
{
    ProcYieldBlock(true);
}

//Returns true if a signal is pending on the given thread
// This also checks process wide signals.
static inline bool ProcSignalIsPending(ProcThread * thread)
{
    return (thread->sigPending | thread->parent->sigPending) & ~thread->sigBlocked;
}

//Delivers pending signals on the current thread
void ProcSignalHandler(IntrContext * iContext);

//Called to restore the state of the thread after a signal handler has executed
void ProcSignalReturn(IntrContext * iContext);

#endif /* PROCESS_H_ */
