/**
 * @file
 * Signal numbers and other constants
 *
 * @date January 2011
 * @author James Cowgill
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

#ifndef SIGNALNUMS_H_
#define SIGNALNUMS_H_

/**
 * @name Special signal handlers
 * @{
 */

#define SIG_DFL (void(*)(int)) 0	///< Perform default signal action
#define SIG_IGN (void(*)(int)) 1	///< Ignore signal
#define SIG_ERR (void(*)(int)) -1	///< Signal error (return code only)

/** @} */

/**
 * Set of signals
 *
 * Signals in sets use the bit 1 less than the value of the signal.
 *
 * Eg: #SIGHUP (1) uses bit 0
 */
typedef unsigned int ProcSigSet;

/**
 * Maximum allowed signal number
 */
#define SIG_MAX 32

/**
 * @name Signals
 * @{
 */
#define SIGHUP 1		///< Hangup
#define SIGINT 2		///< Interrupt (Ctrl-C)
#define SIGQUIT 3		///< Quit (Ctrl-\)
#define SIGILL 4		///< Illegal Instruction
#define SIGTRAP 5		///< Debug Trap
#define SIGABRT 6		///< Abort
#define SIGBUS 7		///< Alignment Error
#define SIGFPE 8		///< Math Error
#define SIGKILL 9		///< Process Killed
#define SIGUSR1 10		///< User Defined Signal 1
#define SIGSEGV 11		///< Segmentation Fault
#define SIGUSR2 12		///< User Defined Signal 2
#define SIGPIPE 13		///< Broken Pipe
#define SIGALRM 14		///< Alarm Clock
#define SIGTERM 15		///< Terminated
#define SIGCHLD 17		///< Child Process Exited
#define SIGCLD SIGCHLD	///< Child Process Exited
#define SIGCONT 18		///< Continue
#define SIGSTOP 19		///< Stop (suspend)
#define SIGTSTP 20		///< Terminal Stop
#define SIGTTIN 21		///< Terminal Stop from reading
#define SIGTTOU 22		///< Terminal Stop from writing
/** @} */

#define SIG_BLOCK   0	///< Block the specified signals
#define SIG_UNBLOCK 1	///< Unblock the specified signals
#define SIG_SETMASK 2	///< Set the blocked signals to the given signals

/**
 * @name Signal Flags
 * @todo none of these are implemented yet
 * @{
 */
#define SA_NOCLDSTOP 1				///< Don't send SIGCHLD when children stop
#define SA_NOCLDWAIT 2				///< Don't create zombie on child death
#define SA_SIGINFO   4				///< Don't signal handler accepts siginfo structure
#define SA_ONSTACK   0x08000000		///< Use special signal stack for handler
#define SA_STACK     SA_ONSTACK		///< Use special signal stack for handler
#define SA_RESTART   0x10000000		///< Restart system call when handler has finished
#define SA_NODEFER   0x40000000		///< Don't block signal when handler is being executed
#define SA_NOMASK    SA_NODEFER		///< Don't block signal when handler is being executed
#define SA_RESETHAND 0x80000000		///< Reset signal handler to SIG_DFL when signal is handled
#define SA_ONESHOT   SA_RESETHAND	///< Reset signal handler to SIG_DFL when signal is handled
/** @} */

#endif /* SIGNALNUMS_H_ */
