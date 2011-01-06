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

void NORETURN test(void * arg)
{
	bool t1;

	//Test arg
	if((unsigned int) arg == 1234)
	{
		t1 = true;
		PrintLog(Info, "Test thread 1 started");
	}
	else if((unsigned int) arg == 5678)
	{
		t1 = false;
		PrintLog(Info, "Test thread 2 started");
	}
	else
	{
		Panic("Invalid test argument");
	}

	for(;;)
	{
		//Do some crap
		if(t1)
		{
			PrintLog(Notice, "Some crap from test 1");
		}
		else
		{
			PrintLog(Notice, "Some crap from test 2");
		}

		//Wait an obseen amount of time
		for(int i = 100000; i > 0; --i)
			;

		//Yeild
		ProcYield();
	}
}

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

	//Malloc must be initialized after interrupts
	MAllocInit();

	// Initialize process manager and scheduler
	ProcInit();

#warning IDEAS
	//Idea - have drives load from a kernel thread so they don't have
	// to deal with the boot special case (ProcCurrThread == NULL)
#warning Why not have a kernel thread return address?

	//DEBUG
	// Setup some testing threads
	ProcWakeUp(ProcCreateKernelThread("test2", test, (void *) 5678));
	ProcWakeUp(ProcCreateKernelThread("test1", test, (void *) 1234));

	// Exit boot mode
	ProcExitBootMode();
}

void NORETURN Panic(const char * msg, ...)
{
	//Print message
	PrintLog(Fatal, msg);
#warning TODO varargs

	//Hang
	asm volatile("cli\n"
				 "hlt\n");
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
