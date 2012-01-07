/*
 * process.h
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

	//Process signal handlers
#warning Info found
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
	struct list_head * alarmPtr;
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
	ProcWaitMode waitMode;

	//Scheduler stuff (don't change this)
	struct list_head schedQueueEntry;		//Place in scheduler queue
	int schedInterrupted;					//Weather thread was interrupted or not
	void * kStackPointer;					//Current pointer to kernel stack
	void * kStackBase;						//Base of kernel stack

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

#define WNOHANG 1		//Causes ProcWait not to block

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
int ProcWaitProcess(int id, unsigned int * exitCode, int options);

//Waits for a thread sibling to exit
// id
//	  >1, only the given pid
//	  -1, any child process
// options = one of the wait options above
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
