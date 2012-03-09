/**
 * @file
 * Internal process and scheduler functions
 *
 * @date December 2010
 * @author James Cowgill
 * @privatesection
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

#ifndef PROCESSINT_H_
#define PROCESSINT_H_

#include "chaff.h"
#include "process.h"

/**
 * Swaps the stack pointer
 *
 * Since this swaps the stack, this function will not return to the same location.
 *
 * @param newStackPtr new kernel stack pointer
 * @param oldStackPtr pointer to place to store the old stack pointer
 */
void PRIVATE STDCALL ProcIntSchedulerSwap(void * newStackPtr, void ** oldStackPtr);

/**
 * Completely removes the current thread from the scheduler
 */
void PRIVATE NORETURN ProcIntSchedulerExitSelf();

/**
 * User mode entry point
 *
 * This function prepares the stack for user mode and then transfers control into user mode
 */
void PRIVATE ProcIntUserThreadEntry();

/**
 * Idle thread
 */
int PRIVATE NORETURN ProcIntIdleThread(void *);

/**
 * Function which kernel threads return to when they exit
 */
void PRIVATE NORETURN ProcIntKernelThreadReturn(int);

/**
 * Reaps a process (frees all kernel structures)
 *
 * @param process process to reap
 */
void PRIVATE ProcIntReapProcess(ProcProcess * process);

/**
 * Reaps a thread (frees all kernel structures)
 *
 * @param thread thread to reap
 */
void PRIVATE ProcIntReapThread(ProcThread * thread);

/**
 * Initializes the reaper thread
 */
void PRIVATE ProcIntReaperInit();

/**
 * Adds a thread and process to be automatically reaped at a later time
 *
 * @a thread must be in a zombie state.
 *
 * The process owning the thread will also be reaped.
 *
 * @param thread thread to reap (parent process will also be reaped)
 */
void PRIVATE ProcIntReaperAdd(ProcThread * thread);

#endif /* PROCESSINT_H_ */
