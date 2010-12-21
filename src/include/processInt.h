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

//Manually adds a thread to the scheduler (usually on thread creation)
void ProcIntSchedulerAdd(ProcThread * thread);

//Removes the current thread from scheduler existence
// This deallocates the ProcThread and kernel stack only
void NORETURN ProcIntSchedulerExitSelf();

#endif /* PROCESSINT_H_ */
