/**
 * @file
 * Inline assembly instructions
 *
 * @date November 2010
 * @author James Cowgill
 * @ingroup Util
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

#ifndef INLINEASM_H_
#define INLINEASM_H_

/**
 * Sets the value of the CR0 register
 *
 * @param val value to set
 */
static inline void setCR0(unsigned int val)
{
	asm ("movl %0, %%cr0"::"r"(val));
}

/**
 * Gets the value of the CR0 register
 */
static inline unsigned int getCR0()
{
	unsigned int val;
	asm ("movl %%cr0, %0":"=r"(val));
	return val;
}

/**
 * Sets the value of the CR3 register
 *
 * CR3 stores the Page Directory Base Register
 *
 * @param val value to set
 */
static inline void setCR3(unsigned int val)
{
	asm ("movl %0, %%cr3"::"r"(val));
}

/**
 * Gets the value of the CR3 register
 *
 * CR3 stores the Page Directory Base Register
 */
static inline unsigned int getCR3()
{
	unsigned int val;
	asm ("movl %%cr3, %0":"=r"(val));
	return val;
}

/**
 * Gets the value of the CR2 register
 *
 * CR2 stores the Page Fault Linear Address
 */
static inline void * getCR2()
{
	void * val;
	asm ("movl %%cr2, %0":"=r"(val));
	return val;
}

/**
 * Invalidates the TLB entry at the given address
 *
 * This forces page tables at the given address to be recalculated by the CPU
 *
 * @param address address to invalidate
 */
static inline void invlpg(void * address)
{
	asm volatile("invlpg %0"::"m"(address));
}

/**
 * Load a pointer to the interrupt descriptor table
 *
 * @param len length of the descriptor table
 * @param ptr pointer to the table
 */
void INIT lidt(unsigned short len, void * ptr);

/**
 * Outputs one byte on the specified port
 *
 * @param port port to output on
 * @param data data to output
 */
static inline void outb(unsigned short port, unsigned char data)
{
	asm volatile("outb %1, %0"::"Nd"(port), "a"(data));
}

/**
 * Outputs two bytes on the specified port
 *
 * @param port port to output on
 * @param data data to output
 */
static inline void outw(unsigned short port, unsigned short data)
{
	asm volatile("outw %1, %0"::"Nd"(port), "a"(data));
}

/**
 * Outputs four bytes on the specified port
 *
 * @param port port to output on
 * @param data data to output
 */
static inline void outd(unsigned short port, unsigned int data)
{
	asm volatile("outd %1, %0"::"Nd"(port), "a"(data));
}

/**
 * Inputs one byte on the specified port
 *
 * @param port port to input from
 * @return the data currently on the port
 */
static inline unsigned char inb(unsigned short port)
{
	unsigned char data;
	asm volatile("inb %1, %0":"=a"(data):"Nd"(port));
	return data;
}

/**
 * Inputs two bytes on the specified port
 *
 * @param port port to input from
 * @return the data currently on the port
 */
static inline unsigned short inw(unsigned short port)
{
	unsigned short data;
	asm volatile("inb %1, %0":"=a"(data):"Nd"(port));
	return data;
}

/**
 * Inputs four bytes on the specified port
 *
 * @param port port to input from
 * @return the data currently on the port
 */
static inline unsigned int ind(unsigned short port)
{
	unsigned int data;
	asm volatile("inb %1, %0":"=a"(data):"Nd"(port));
	return data;
}

#endif
