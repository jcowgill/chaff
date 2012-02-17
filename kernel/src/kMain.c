/*
 * kMain.c
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
 *  Created on: 30 Sep 2010
 *      Author: James
 */

#include "chaff.h"
#include "multiboot.h"
#include "mm/misc.h"
#include "process.h"
#include "interrupt.h"
#include "timer.h"
#include "io/device.h"
#include "cpu.h"

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

	// Initialize CPU specific instructions
	CpuInit();

	// Initialize interrupts
	IntrInit();

	//Malloc must be initialized after interrupts (to allow page faults)
	MAllocInit();

	// Initialize timer system
	TimerInit();

	// Initialize process manager and scheduler
	ProcInit();

	// Initialize devfs
	IoDevFsInit();

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
