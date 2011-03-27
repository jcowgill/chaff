/*
 * kMain.c
 *
 *  Created on: 30 Sep 2010
 *      Author: James
 */

#include "chaff.h"
#include "multiboot.h"
#include "memmgr.h"
#include "process.h"
#include "interrupt.h"

void NORETURN kMain(unsigned int mBootCode, multiboot_info_t * mBootInfo)
{
	//Kernel C Entry Point
	// Check boot code
	if(mBootCode != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		Panic("kMain: OS must be loaded by a multiboot bootloader");
	}

	// Initialize memory manager
	MemManagerInit(mBootInfo);

	// Initialize interrupts
	IntrInit();

	//Malloc must be initialized after interrupts (to allow page faults)
	MAllocInit();

	// Initialize process manager and scheduler
	ProcInit();

	// Exit boot mode
	ProcExitBootMode();
}

void NORETURN Panic(const char * msg, ...)
{
	//Print message
	PrintLog(Fatal, msg);
#warning TODO varargs

	//Hang
	for(;;)
	{
		asm volatile("cli\n"
					 "hlt\n");
	}
}

static char * nextPos = (char *) 0xC00B8000;

void PrintStr(const char * msg)
{
	//Prints a string to the video output
	while(*msg)
	{
		*nextPos++ = *msg++;
		*nextPos++ = 7;

		//Wrap around if reached the end
		if((unsigned int) nextPos >= 0xC00B8FA0)
		{
			nextPos = (char *)0xC00B8000;
		}
	}
}

void PrintLog(LogLevel level, const char * msg, ...)
{
	//Print level
	switch(level)
	{
	case Fatal:
		PrintStr("Panic: ");
		break;

	case Critical:
		PrintStr("Critical: ");
		break;

	case Error:
		PrintStr("Error: ");
		break;

	case Warning:
		PrintStr("Warning: ");
		break;

	case Notice:
		PrintStr("Notice: ");
		break;

	case Info:
		PrintStr("Info: ");
		break;

	case Debug:
		PrintStr("Debug: ");
		break;
	}

	//Print message
	PrintStr(msg);

	//New line + wrap around
	nextPos += 160 - ((((unsigned int) nextPos) - 0xC00B8000) % 160);
	if((unsigned int) nextPos >= 0xC00B8FA0)
	{
		nextPos = (char *)0xC00B8000;
	}
}
