/*
 * processInt.h
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

#ifndef PROCESSINT_H_
#define PROCESSINT_H_

#include "chaff.h"
#include "process.h"

//Internal process functions

//Swaps the stack
void STDCALL ProcIntSchedulerSwap(void * newStackPtr, void ** oldStackPtr);

// Removes the current thread from scheduler existence
void NORETURN ProcIntSchedulerExitSelf();

// User thread entry point
void ProcIntUserThreadEntry();

// Idle thread code
int NORETURN ProcIntIdleThread(void *);

// Kernel thread return
void NORETURN ProcIntKernelThreadReturn(int);

//Reaper functions
void ProcIntReapProcess(ProcProcess * process);
void ProcIntReapThread(ProcThread * thread);

//Kernel reaper thread

//Initialize the reaper
void ProcIntReaperInit();

//Adds a thread to be automatically reaped
// The thread must be a zombie
// If the thread is owned by another process:
//   The process must be owned by the kernel
//   That process will also be reaped
void ProcIntReaperAdd(ProcThread * thread);

#endif /* PROCESSINT_H_ */
