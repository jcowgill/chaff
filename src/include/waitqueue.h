/*
 * waitqueue.h
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

#ifndef WAITQUEUE_H_
#define WAITQUEUE_H_

//Simple queue allowing multiple threads to wait for an event
//

#include "chaff.h"
#include "list.h"

//The wait queue which threads can wait for
typedef ListHead ProcWaitQueue;

//Causes this thread to wait for an event on the wait queue
bool ProcWaitQueueWait(ProcWaitQueue * queue, bool interruptable);

//Wakes up one thread on the wait queue (the oldest)
void ProcWaitQueueWakeOne(ProcWaitQueue * queue);

//Wakes up all the threads on the wait queue
void ProcWaitQueueWakeAll(ProcWaitQueue * queue);

#endif /* WAITQUEUE_H_ */
