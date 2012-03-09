/**
 * @file
 * CPU Exceptions
 *
 * These functions are for internal use only.
 *
 * @date March 2011
 * @author James Cowgill
 * @privatesection
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

#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include "interrupt.h"

/**
 * Fatal math exception
 *
 * - Divide by zero
 * - FPU error
 *
 * Raises SIGFPU. Kills process if this is ignored.
 *
 * @param iContext context of this exception
 */
void PRIVATE IntrExceptMathFault(IntrContext * iContext);

/**
 * Normal math exception
 *
 * - Overflow (INTO instruction)
 * - Bounds error (BOUND instruction)
 *
 * Raises SIGFPU.
 *
 * @param iContext context of this exception
 */
void PRIVATE IntrExceptMathTrap(IntrContext * iContext);

/**
 * Debug exception
 *
 * - Step
 * - Breakpoint (INT 3 instruction
 *
 * Raises SIGTRAP.
 *
 * @param iContext context of this exception
 */
void PRIVATE IntrExceptDebugTrap(IntrContext * iContext);

/**
 * invalid opcode exception
 *
 * Raises SIGILL. Kills process if this is ignored.
 *
 * @param iContext context of this exception
 */
void PRIVATE IntrExceptIllOpcodeFault(IntrContext * iContext);

/**
 * General protection exception
 *
 * - Segment not present
 * - Stack fault
 * - General protection fault
 *
 * Most protection faults are handled by the page fault handler
 *
 * Raises SIGSEGV. Kills process if this is ignored.
 *
 * @param iContext context of this exception
 * @see MemPageFaultHandler()
 */
void PRIVATE IntrExceptProtectionFault(IntrContext * iContext);

/**
 * Alignment exception
 *
 * Raises SIGBUS. Kills process if this is ignored.
 *
 * @param iContext context of this exception
 */
void PRIVATE IntrExceptAlignmentFault(IntrContext * iContext);

/**
 * Undefined exception
 *
 * Panics with fatal CPU error
 *
 * @param iContext context of this exception
 */
void PRIVATE IntrExceptError(IntrContext * iContext);

#endif /* EXCEPTIONS_H_ */
