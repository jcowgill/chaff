/**
 * @file
 * CPU interrupt handling
 *
 * @date December 2010
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

#ifndef INTERRUPT_H_
#define INTERRUPT_H_

/**
 * The context an interrupt occurs in
 *
 * This structure contains all the registers saved on the stack before
 * an interrupt is handled. You may modify some values here to return
 * from an interrupt with different register values but with some restrictions.
 */
typedef struct IntrContext
{
	/**
	 * @name General purpose registers
	 *
	 * Generally, you should only need to change these (and usually only eax)
	 * @{
	 */
	unsigned int edi;
	unsigned int esi;
	unsigned int ebp;
	unsigned int : 32;
	unsigned int ebx;
	unsigned int edx;
	unsigned int ecx;
	unsigned int eax;

	/**
	 * @}
	 * @name Segment selectors
	 *
	 * Changing these can be a security risk
	 * @{
	 */
	unsigned int ds;
	unsigned int es;
	unsigned int fs;
	unsigned int gs;

	/**
	 * @}
	 * @name Interrupt information
	 *
	 * The interrupt number contains the interrupt fired.
	 * The interrupt error is used for some interrupts so the CPU can supply additional information.
	 * @{
	 */
	unsigned int intrNum;
	unsigned int intrError;

	/**
	 * @}
	 * @name Caller return information
	 *
	 * Changing cs and eflags can be a security risk
	 * @{
	 */
	unsigned int eip;
	unsigned int cs;
	unsigned int eflags;

	/**
	 * @}
	 * @name Caller stack information
	 *
	 * These exist only if the caller was in user mode (cs == 0x1B)
	 * @{
	 */
	unsigned int esp;
	unsigned int ss;

} IntrContext;

/**
 * Initializes the interrupt system and installs CPU exception interrupts
 *
 * @private
 */
void IntrInit();

/**
 * Entry point for the interrupt handler
 *
 * @param iContext context of the interrupt
 * @private
 */
void IntrHandler(IntrContext iContext);

/**
 * This flag allows interrupts to have multiple handlers.
 *
 * Each handler will be called one after the other
 */
#define INTR_SHARED 1

/**
 * Registers the given IRQ with an interrupt handler
 *
 * @param irq IRQ number to register (0 - 15)
 * @param flags extra flags for the handler
 * @param handler pointer to the function to use as the interrupt handler
 *
 * @retval true the handler was successfully registered
 * @retval false a handler already exists or the IRQ was invalid
 */
bool IntrRegister(int irq, int flags, void (* handler)(IntrContext *));

/**
 * Unregisters the given IRQ handler
 *
 * @param irq IRQ number to unregister
 * @param handler pointer to the function passed to IntrRegister() before
 * @retval true the handler was successfully unregistered
 * @retval false the handler is not registered
 */
bool IntrUnRegister(int irq, void (* handler)(IntrContext *));

#endif /* INTERRUPT_H_ */
