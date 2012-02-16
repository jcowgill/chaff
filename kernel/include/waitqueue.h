/**
 * @file
 * Queue of waiting threads
 *
 * This allows multiple threads to wait on a single event
 *
 * @date January 2012
 * @author James Cowgill
 * @ingroup Proc Util
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

#ifndef WAITQUEUE_H_
#define WAITQUEUE_H_

#include "chaff.h"
#include "list.h"

/**
 * The wait queue threads can wait on
 */
typedef ListHead ProcWaitQueue;

/**
 * Initializes a wait queue
 *
 * @param head queue to initialize
 */
#define ProcWaitQueueInit ListHeadInit

/**
 * Causes the current thread to wait on the specified queue
 *
 * @param queue queue to wait on
 * @param interruptable true if the wait can be interrupted by a signal
 * @retval true if the thread was interrupted
 * @retval false if the thread was woken by ProcWaitQueueWakeOne() or ProcWaitQueueWakeAll()
 */
bool ProcWaitQueueWait(ProcWaitQueue * queue, bool interruptable);

/**
 * Wakes up the oldest thread on a wait queue
 *
 * @param queue queue to wake a thread from
 * @retval true if a thread was woken up
 * @retval false if there are no threads on the queue
 */
bool ProcWaitQueueWakeOne(ProcWaitQueue * queue);

/**
 * Wakes all the threads on a wait queue
 *
 * @param queue queue to wake all threads from
 */
void ProcWaitQueueWakeAll(ProcWaitQueue * queue);

#endif /* WAITQUEUE_H_ */
