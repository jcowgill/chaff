/*
 * waitqueue.c
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
 *  Created on: 30 Jan 2012
 *      Author: james
 */

#include "chaff.h"
#include "process.h"
#include "waitqueue.h"

//Causes this thread to wait for an event on the wait queue
bool ProcWaitQueueWait(ProcWaitQueue * queue, bool interruptable)
{
	//Add to queue
	ListHeadAddLast(&ProcCurrThread->waitQueue, (ListHead *) queue);

	//Wait
	bool res = ProcYieldBlock(interruptable);

	//Remove from queue if still on it
	if(!ListEmpty(&ProcCurrThread->waitQueue))
	{
		ListDeleteInit(&ProcCurrThread->waitQueue);
	}

	return res;
}

//Wakes up one thread on the wait queue (the oldest)
void ProcWaitQueueWakeOne(ProcWaitQueue * queue)
{
	//Wake up first thread
	ProcThread * thread = ListEntry(queue->next, ProcThread, waitQueue);

	ListDeleteInit(&thread->waitQueue);
	ProcWakeUp(thread);
}

//Wakes up all the threads on the wait queue
void ProcWaitQueueWakeAll(ProcWaitQueue * queue)
{
	//Wake all threads and remove them
	ProcThread * thread;
	ProcThread * tmpThread;

	ListForEachEntrySafe(thread, tmpThread, queue, waitQueue)
	{
		ListDeleteInit(&thread->waitQueue);
		ProcWakeUp(thread);
	}
}
