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
#include "mm/kmemory.h"
#include "io/bcache.h"
#include "loader/ksymbols.h"
#include "loader/bootmodule.h"
#include "processInt.h"

//INIT code residing in a threaded context
static int INIT ThreadedInit(void * mBootInfo);

void INIT NORETURN kMain(unsigned int mBootCode, multiboot_info_t * mBootInfo)
{
	//Kernel C Entry Point
	// Check boot code
	if(mBootCode != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		Panic("kMain: OS must be loaded by a multiboot bootloader");
	}

	// Wipe screen
	MemSet((void *) 0xC00B8000, 0, 0xFA0);

	// Core Initialization (most other stuff depends on this)
	IntrInit();
	CpuInit();
	MemManagerInit(mBootInfo);
	MemSlabInit();

	// Other Kernel Initializations
	CpuInitLate();
	TimerInit();
	ProcInit();
	IoBlockCacheInit();
	IoDevFsInit();
	LdrReadKernelSymbols(mBootInfo);

	// Modules are loaded in a kernel thread so that they can
	//  do things requiring a thread context
	ProcThread * initThread = ProcCreateKernelThread("init", ThreadedInit, mBootInfo);
	ProcWakeUp(initThread);

	// Exit boot mode
	ProcIntSelfExit();
}

//INIT code residing in a threaded context
static int INIT ThreadedInit(void * mBootInfo)
{
	// Initialize boot modules
	LdrLoadBootModules((multiboot_info_t *) mBootInfo);

	// Free init pages
	MemFreeInitPages();
	return 0;
}
