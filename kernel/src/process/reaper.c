/*
 * reaper.c
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
 *  Created on: 11 Jan 2012
 *      Author: james
 */

#include "chaff.h"
#include "process.h"
#include "processInt.h"
#include "list.h"

//Kernel reaper thread

static ListHead reaperHead = LIST_INLINE_INIT(reaperHead);
static ProcThread * reaperThread;

static int NORETURN ProcIntReaperThread(void * unused);

//Initialize the reaper
void ProcIntReaperInit()
{
	//Create kernel thread
	reaperThread = ProcCreateKernelThread("reaper", ProcIntReaperThread, NULL);
}

//Reaper thread entry point
static int NORETURN ProcIntReaperThread(void * unused)
{
	IGNORE_PARAM unused;

	//Start reaping loop
	for(;;)
	{
		//Process all reaper entries
		while(!ListEmpty(&reaperHead))
		{
			//Extract thread and remove from list
			ProcThread * thread = ListEntry(reaperHead.next, ProcThread, schedQueueEntry);
			ListDelete(reaperHead.next);

			//Check if thread is the child of a zombie process
			if(thread->parent != ProcKernelProcess)
			{
				//Reap entire process
				ProcIntReapProcess(thread->parent);
			}
			else
			{
				//Reap thread
				ProcIntReapThread(thread);
			}
		}

		//Wait for more requests
		ProcYieldBlock(false);
	}
}

//Adds a thread to be automatically reaped
// The thread must be a zombie
// If the thread is owned by another process:
//   The process must be owned by the kernel
//   That process will also be reaped
void ProcIntReaperAdd(ProcThread * thread)
{
	//Validate request
	if(thread->state != PTS_ZOMBIE)
	{
		PrintLog(Error, "ProcIntReaperAdd: non-zombie thread passed");
	}
	else if(thread->parent != ProcKernelProcess && !thread->parent->zombie)
	{
		PrintLog(Error, "ProcIntReaperAdd: thread of a non-zombie process passed");
	}
	else
	{
		//Add to queue
		ListHeadAddLast(&thread->schedQueueEntry, &reaperHead);
		ProcWakeUp(reaperThread);
	}
}

