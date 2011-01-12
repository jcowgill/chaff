/*
 * processInt.h
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

// Kernel thread return address
void ProcIntKernelThreadReturn();

// Idle thread code
int NORETURN ProcIntIdleThread(void *);

#endif /* PROCESSINT_H_ */
