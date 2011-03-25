/*
 * inlineasm.h
 *
 *  Created on: 21 Nov 2010
 *      Author: James
 */

#ifndef INLINEASM_H_
#define INLINEASM_H_

//Control register setters
static inline void setCR3(unsigned int val)
{
	asm ("movl %0, %%cr3"::"r"(val));
}

static inline unsigned int getCR3()
{
	unsigned int val;
	asm ("movl %%cr3, %0":"=r"(val));
	return val;
}

static inline void * getCR2()
{
	void * val;
	asm ("movl %%cr2, %0":"=r"(val));
	return val;
}

//Invalidate TLB entry at given address
static inline void invlpg(void * address)
{
	asm volatile("invlpg %0"::"m"(address));
}

//Load interrupt descriptor table
// This is not inline but implemented in interrupt.s
void lidt(unsigned short len, void * ptr);

//IO commands
static inline void outb(unsigned short port, unsigned char data)
{
	asm volatile("outb %1, %0"::"Nd"(port), "a"(data));
}

static inline void outw(unsigned short port, unsigned short data)
{
	asm volatile("outw %1, %0"::"Nd"(port), "a"(data));
}

static inline void outd(unsigned short port, unsigned int data)
{
	asm volatile("outd %1, %0"::"Nd"(port), "a"(data));
}

static inline unsigned char inb(unsigned short port)
{
	unsigned char data;
	asm volatile("inb %1, %0":"=a"(data):"Nd"(port));
	return data;
}

static inline unsigned short inw(unsigned short port)
{
	unsigned short data;
	asm volatile("inb %1, %0":"=a"(data):"Nd"(port));
	return data;
}

static inline unsigned int ind(unsigned short port)
{
	unsigned int data;
	asm volatile("inb %1, %0":"=a"(data):"Nd"(port));
	return data;
}

#endif
