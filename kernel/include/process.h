/**
 * @file
 * Process, scheduler and signal management
 *
 * @date December 2010
 * @author James Cowgill
 * @ingroup Proc
 */

/*
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
 */

#ifndef PROCESS_H_
#define PROCESS_H_

#include "list.h"
#include "htable.h"
#include "interrupt.h"
#include "signalNums.h"
#include "secContext.h"

/**
 * Thread states
 */
typedef enum
{
	PTS_STARTUP,		///< Thread has been created but has never been on the scheduler queue
	PTS_RUNNING,		///< Thread is running or queued to be run

	PTS_INTR,			///< Interruptible wait state
	PTS_UNINTR,			///< Uninterruptible wait state

	PTS_ZOMBIE,			///< Thread has ended and has not been reaped

} ProcThreadState;

/**
 * Thread wait mode
 *
 * Determines what a thread is waiting for
 */
typedef enum
{
	PWM_NONE,			///< Not waiting
	PWM_PROCESS,		///< Waiting for a process
	PWM_THREAD,			///< Waiting for a thread

} ProcWaitMode;

struct ProcProcess;
struct ProcThread;
struct IoContext;
struct MemContext;

/**
 * Contains information about actions taken as a result of a signal
 */
typedef struct ProcSigaction
{
	void (* sa_handler)(int);	///< Signal handler (user mode pointer)
	ProcSigSet sa_mask;			///< Signal mask to apply while handling signal
	int sa_flags;				///< Flags applying to signal

} ProcSigaction;

/**
 * A process on the system, containing a memory context, IO context and a number of threads
 */
typedef struct ProcProcess
{
	/**
	 * Process ID
	 */
	unsigned int pid;

	/**
	 * Item in the process hash map
	 */
	HashItem hItem;

	/** @name Process Relationships @{ */
	struct ProcProcess * parent;	///< Parent process
	ListHead processSibling;		///< Entry in the sibling list
	ListHead children;				///< Head of the process children list
	ListHead threads;				///< Head of the threads list
	/** @} */

	/**
	 * True if the process is zombified
	 */
	bool zombie;

	/**
	 * Name of this process (null terminated)
	 */
	char * name;

	/**
	 * Exit code of the process
	 */
	unsigned int exitCode;

	/**
	 * Process memory context
	 *
	 * This is not setup by ProcCreateProcess()
	 */
	struct MemContext * memContext;

	/**
	 * Process IO context
	 *
	 * This is not setup by ProcCreateProcess()
	 */
	struct IoContext * ioContext;

	/**
	 * Process security context
	 */
	SecContext secContext;

	/**
	 * Process signal handlers
	 */
	ProcSigaction sigHandlers[SIG_MAX];

	/**
	 * Process wide pending signals set
	 */
	ProcSigSet sigPending;

	/**
	 * Process wide alarm pointer
	 */
	ListHead * alarmPtr;

} ProcProcess;

/**
 * A thread of execution which can be added to the scheduler and run
 */
typedef struct ProcThread
{
	/**
	 * Thread ID
	 */
	unsigned int tid;

	/**
	 * Hash map item
	 */
	HashItem hItem;

	/**
	 * Parent process
	 */
	struct ProcProcess * parent;

	/**
	 * Entry in thread sibling list
	 */
	ListHead threadSibling;

	/**
	 * Thread exit code
	 */
	unsigned int exitCode;

	/**
	 * Name of thread
	 */
	char * name;

	/**
	 * State of thread
	 */
	ProcThreadState state;

	/**
	 * Thread wait mode (only used while in ProcWaitProcess() / ProcWaitThread())
	 */
	ProcWaitMode waitMode;

	/** @name Scheduler Fields @{ */
	ListHead schedQueueEntry;			///< Position in scheduler queue
										///< Also used by process reaper after the thread is a zombie
	int schedInterrupted;				///< Weather thread was interrupted or not
	void * kStackPointer;				///< Current pointer to kernel stack
	void * kStackBase;					///< Base of kernel stack
	unsigned long long tlsDescriptor;	///< Thread local storage descriptor for this thread
	/** @} */

	/**
	 * Stores a pointer to the FPU / SSE state of a thread
	 */
	void * fpuState;

	/**
	 * The number of FPU switches this thread has made
	 *
	 * When this is high, the scheduler switches the state immediately instead of lazily switching
	 */
	unsigned char fpuSwitches;

	/**
	 * Pending signal set (thread local)
	 */
	ProcSigSet sigPending;

	/**
	 * Blocked signals mask
	 */
	ProcSigSet sigBlocked;

	/**
	 * Current wait queue (see ProcWaitQueueWait())
	 */
	ListHead waitQueue;

} ProcThread;

/**
 * 4KB Kernel Stack
 */
#define PROC_KSTACK_SIZE 0x1000

/**
 * @name Scheduler Functions
 * @{
 */

/**
 * Currently running process
 *
 * Do not modify
 */
extern ProcProcess * ProcCurrProcess;

/**
 * Currently running thread
 *
 * Do not modify
 */
extern ProcThread * ProcCurrThread;

/**
 * Yields the current thread so that other threads can run
 *
 * Does nothing if there are no other threads to run.
 *
 * This does not guarantee that interrupts will be handled before it returns.
 */
void ProcYield();

/**
 * Blocks the current thread until it is later woken up with ProcWakeUp()
 *
 * If @a interruptable is true, the block may be interrupted if a
 * signal is received on the current thread or process.
 *
 * @param interruptable true if the block is interruptable
 * @retval true the block was interrupted by a signal
 * @retval false the block was woken up with ProcWakeUp()
 */
bool ProcYieldBlock(bool interruptable);

/**
 * Wakes up threads which have called ProcYieldBlock()
 *
 * This function allows the signals manager to wake threads up
 * but should not be used by other code.
 *
 * @param thread thread to wake up
 * @param isSignal true if the wake up was caused by a signal receipt
 * @private
 */
void ProcWakeUpSig(ProcThread * thread, bool isSignal);

/**
 * Wakes up threads which have called ProcYieldBlock()
 *
 * You should only wake up threads which you caused to block.
 *
 * @param thread thread to wake up
 * @private
 */
static inline void ProcWakeUp(ProcThread * thread)
{
	ProcWakeUpSig(thread, false);
}

/**
 * @}
 * @name Process and thread management
 * @{
 */

/**
 * Gets a process by looking up its id
 *
 * @param pid id to lookup
 * @return the process or NULL if if wasn't found
 */
ProcProcess * ProcGetProcessByID(unsigned int pid);

/**
 * Gets a thread by looking up its id
 *
 * @param tid id to lookup
 * @return the thread or NULL if if wasn't found
 */
ProcThread * ProcGetThreadByID(unsigned int tid);

/**
 * Creates a completely empty process from nothing
 *
 * The memory and IO contexts be filled in by the caller.
 * Any initial threads must also be created manually.
 *
 * @param name name of process
 * @param parent parent process (if there isn't one, use #ProcKernelProcess)
 * @return the created process
 */
ProcProcess * ProcCreateProcess(const char * name, ProcProcess * parent);

/**
 * Exits the current process with the given error code
 */
void NORETURN ProcExitProcess(unsigned int exitCode);

/**
 * Creates a new user mode thread within a process
 *
 * You must wake the thread up with ProcWakeUp()
 *
 * @param name name of thread
 * @param process parent process (which the threads runs in the context of)
 * @param startAddr start address to execute in user-mode
 * @param stackPtr initial stack pointer address (esp)
 * @return the created thread
 */
ProcThread * ProcCreateUserThread(const char * name, ProcProcess * process, void (* startAddr)(), void * stackPtr);

/**
 * Creates a new kernel mode thread
 *
 * Kernel threads are always created in the context of #ProcKernelProcess
 *
 * The thread is allocated a stack (size = #PROC_KSTACK_SIZE).
 * You must not use lots of stack space!
 *
 * The kernel thread may return and the value returned is used as its exit code.
 *
 * You must wake the thread up with ProcWakeUp()
 *
 * @param name name of thread
 * @param startAddr start address to execute at
 * @param arg argument to pass to kernel thread
 * @return the created thread
 */
ProcThread * ProcCreateKernelThread(const char * name, int (* startAddr)(void *), void * arg);

/**
 * Exits the current thread with the given exit code
 *
 * This function does not return
 *
 * @param exitCode exit code of the thread
 */
void NORETURN ProcExitThread(unsigned int exitCode);

/**
 * Forks the current process
 *
 * This is a "convinience" function which clones all the nessesary fields
 * of the process, creates a new thread for it and wakes it up.
 *
 * You cannot clone the kernel process. If drivers want to create a process,
 * they must do it manually (see ProcCreateProcess())
 *
 * @param startAddr start address of the new thread (this should come from the interrupt context)
 * @param userStackPtr initial stack pointer of the new thread (this should come from the interrupt context)
 * @return the new process
 */
ProcProcess * ProcFork(void (* startAddr)(), void * userStackPtr);

/**
 * Prevents ProcWaitProcess() and ProcWaitThread() from blocking
 */
#define WNOHANG 1

/**
 * Waits for a child process to exit
 *
 * Kernel threads cannot wait on processes.
 *
 * @param[in] id specifys the process(s) to wait for
 *  - >1 only the given process id
 *  - -1 any child process
 *  - 0  any process from the current process group
 *  - <1 any process from the given process group
 * @param[out] exitCode pointer to place to write exit code to (must be kernel mode)
 * @param[in] options any extra flags
 * @retval 0 #WNOHANG was given and there are no waitable processes
 * @retval >0 the process id (success)
 * @retval <0 error code
 * @bug Process groups not implemented yet
 */
int ProcWaitProcess(int id, unsigned int * exitCode, int options);

/**
 * Waits for another thread in the current process to exit
 *
 * Kernel threads cannot wait on any thread.
 *
 * @param[in] id specifys the process(s) to wait for
 *  - >1 only the given thread id
 *  - -1 any child thread
 * @param[out] exitCode pointer to place to write exit code to (must be kernel mode)
 * @param[in] options any extra flags
 * @retval 0 #WNOHANG was given and there are no waitable processes
 * @retval >0 the thread id (success)
 * @retval <0 error code
 * @bug Allow kernel threads to wait on other threads? - maybe have a special exit code for "autoreap"?
 */
int ProcWaitThread(int id, unsigned int * exitCode, int options);

/**
 * Kernel process data pointer
 *
 * @private
 */
extern ProcProcess ProcKernelProcessData;

/**
 * Pointer to idle thread
 */
extern ProcThread * ProcIdleThread;

/**
 * Pointer kernel process
 */
#define ProcKernelProcess (&ProcKernelProcessData)

/**
 * @}
 * @name Thread Local Storage
 *
 * TLS is implemented by changing the base pointer of the gs
 * register using different descriptors.
 *
 * @{
 */

/**
 * Null TLS descriptor
 *
 * Prevents all use of thread local storage.
 * All ProcThread::tlsDescriptor is initialized to this.
 */
#define PROC_NULL_TLS_DESCRIPTOR (0x0040F20000000000ULL)

/**
 * Base TLS descriptor
 *
 * Used as basis for creating other descriptors in ProcTlsCreateDescriptor()
 * @private
 */
#define PROC_BASE_TLS_DESCRIPTOR (0x00C0F60000000000ULL)

/**
 * Creates a new TLS descriptor using the given base pointer
 *
 * To use TLS on a thread, store the returned descriptor in
 * ProcThread::tlsDescriptor.
 *
 * @param basePtr pointer to base of TLS descriptors
 * @retval #PROC_NULL_TLS_DESCRIPTOR on error (basePtr out of range)
 * @retval other the generated descriptor
 */
unsigned long long ProcTlsCreateDescriptor(unsigned int basePtr) __attribute__((pure));

/**
 * @}
 * @name Signal Functions
 * @{
 */

/**
 * Sends an unblockable signal to the current thread
 *
 * If the signal would be blocked / ignored by the thread,
 * the thread is forcefully terminated (in this case, the function will not return).
 *
 * This is generally used by CPU exception handlers.
 *
 * @param sigNum signal number to raise
 */
void ProcSignalSendOrCrash(int sigNum);

/**
 * Sends a signal to the given thread
 *
 * The signal is sent to the thread ONLY
 * (signals are not redirected to the whole process).
 *
 * @param thread thread to send signal to
 * @param sigNum signal number to raise
 */
void ProcSignalSendThread(ProcThread * thread, int sigNum);

/**
 * Sends a signal to the given process
 *
 * The signal can be received by any thread of the process
 * which isn't blocking the signal.
 *
 * @param process process to send signal to
 * @param sigNum signal number to raise
 */
void ProcSignalSendProcess(ProcProcess * process, int sigNum);

/**
 * Updates the thread signal mask
 *
 * The signal can be received by any thread of the process
 * which isn't blocking the signal.
 *
 * @param thread thread to update mask of
 * @param how one of #SIG_BLOCK, #SIG_UNBLOCK, #SIG_SETMASK
 * @param signalSet set of signals to update
 *  signals are given a bit 1 less than their value
 *  (eg: #SIGHUP (number 1) goes to bit 0)
 */
void ProcSignalSetMask(ProcThread * thread, int how, ProcSigSet signalSet);

/**
 * Sets the signal handler for a signal in a process
 *
 * @param process process to change handler for
 * @param sigNum signal to change handler for
 * @param newAction new action for the signal
 */
void ProcSignalSetAction(ProcProcess * process, int sigNum, ProcSigaction newAction);

/**
 * Waits for a signal to be sent to this thread
 *
 * Any signal which isn't blocked and is sent to this thread
 * (possibly via being sent to the process) will cause this function
 * to return. The signal manager will first attempt to send signals to
 * running threads instead of waking this one up.
 *
 * #SIGKILL and #SIGSTOP signals will cause this function to return.
 * The actual killing will happen when ProcSignalHandler() is called
 * just before returning to user mode.
 */
static inline void ProcSignalWait()
{
    ProcYieldBlock(true);
}

/**
 * Returns true if a signal is pending for the specified thread and is not blocked.
 *
 * This also includes process wide signals.
 *
 * @param thread thread to check
 * @retval there is at least 1 signal which can be handled now on the given thread
 * @retval there are not signals which can be handled now
 */
static inline bool ProcSignalIsPending(ProcThread * thread)
{
    return (thread->sigPending | thread->parent->sigPending) & ~thread->sigBlocked;
}

/**
 * Delivers pending signals on the current thread
 *
 * @param iContext interrupt context to execute in
 * @private
 */
void ProcSignalHandler(IntrContext * iContext);

/**
 * Restores the state of the current thread after a user mode signal handler has executed
 *
 * @param iContext interrupt context to execute in
 * @private
 */
void ProcSignalReturn(IntrContext * iContext);

#endif /* PROCESS_H_ */
