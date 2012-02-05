/*
 * exceptions.h
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
