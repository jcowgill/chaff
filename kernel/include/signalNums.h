/*
 * signalNums.h
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
 *  Created on: 12 Jan 2011
 *      Author: James
 */

#ifndef SIGNALNUMS_H_
#define SIGNALNUMS_H_

//Signal constants
#define SIG_DFL (void(*)(int)) 0
#define SIG_IGN (void(*)(int)) 1
#define SIG_ERR (void(*)(int)) -1

typedef unsigned int ProcSigSet;
#define SIG_MAX 32		//Maximum signal number allowed

// Signal 0 does nothing
#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGBUS 7
#define SIGFPE 8
#define SIGKILL 9
#define SIGUSR1 10
#define SIGSEGV 11
#define SIGUSR2 12
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
//#define SIGSTKFLT 16		- coprocessor stack fault - unused
#define SIGCHLD 17
#define SIGCLD SIGCHLD
#define SIGCONT 18
#define SIGSTOP 19
#define SIGTSTP 20
#define SIGTTIN 21
#define SIGTTOU 22

#define SIG_BLOCK   0 /* Block signals.  */
#define SIG_UNBLOCK 1 /* Unblock signals.  */
#define SIG_SETMASK 2 /* Set the set of blocked signals.  */

//Implemented flags:
// None of the flags implemented yet
#define SA_NOCLDSTOP 1 /* Don't send SIGCHLD when children stop.  */
#define SA_NOCLDWAIT 2 /* Don't create zombie on child death.  */
#define SA_SIGINFO   4 /* Invoke signal-catching function with three arguments instead of one.  */
#define SA_ONSTACK   0x08000000 /* Use signal stack by using `sa_restorer'. */
#define SA_STACK     SA_ONSTACK
#define SA_RESTART   0x10000000 /* Restart syscall on signal return.  */
#define SA_NODEFER   0x40000000 /* Don't automatically block the signal when its handler is being executed.  */
#define SA_NOMASK    SA_NODEFER
#define SA_RESETHAND 0x80000000 /* Reset to SIG_DFL on entry to handler.  */
#define SA_ONESHOT   SA_RESETHAND

#endif /* SIGNALNUMS_H_ */
