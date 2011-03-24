/*
 * exceptions.h
 *
 *  Created on: 24 Mar 2011
 *      Author: james
 */

#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include "interrupt.h"

//CPU Exception Implementation
// Note: names ending with Fault will force the program to exit if the signal is ignored
void IntrExceptMathFault(IntrContext * iContext);		//Divide by zero, No math coprocessor, FPU error
void IntrExceptMathTrap(IntrContext * iContext);		//Overflow (INTO), Bound error (BOUND)
void IntrExceptDebugTrap(IntrContext * iContext);		//Step, Breakpoint
void IntrExceptIllOpcodeFault(IntrContext * iContext);	//Invalid opcode
void IntrExceptProtectionFault(IntrContext * iContext);	//Segment not present, Stack fault, General Protection fault
void IntrExceptAlignmentFault(IntrContext * iContext);	//Alignment check

void IntrExceptError(IntrContext * iContext);			//Error exceptions

#endif /* EXCEPTIONS_H_ */
