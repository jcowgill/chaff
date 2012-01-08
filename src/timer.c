/*
 * timer.c
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
 *  Created on: 5 Feb 2011
 *      Author: James
 */

#include "chaff.h"
#include "timer.h"
#include "process.h"
#include "inlineasm.h"

//PIT Frequencies and Magic Numbers
// These are for 100Hz (frequency of out PIT IRQ)
#define PIT_RELOADVAL 11932
#define PIT_TICKS_PER_INTERRUPT ((TimerTime) 42950333)		//42950332.885217534630798048355308

#define PIT_OSCILLATOR_RATE 1193182

//Current time
TimerTime currentTime;

//System boot time
TimerTime startupTime;

//Time beep is schedule to end / 0 = unused
TimerTime beepEndTime = 0;

//Timer sleep queues
typedef struct
{
	union
	{
		ProcThread * thread;		//Used by sleep queue
		ProcProcess * process;		//Used by alarm queue
	};

	TimerTime endTime;
	ListHead list;

} TimerQueue;

static ListHead sleepQueueHead = LIST_INLINE_INIT(sleepQueueHead);
static ListHead alarmQueueHead = LIST_INLINE_INIT(alarmQueueHead);

static void TimerInterrupt(IntrContext * iContext);

//Initialises the PIT and PC Speaker
void TimerInit()
{
	//Register interrupts
	if(!IntrRegister(0, 0, TimerInterrupt))
	{
		Panic("TimerInit: Cannot initialize timer - IntrRegister returned false");
	}

	//Read time from CMOS
	currentTime = TimerGetCMOSTime();

	//Store startup time
	startupTime = currentTime;

	//Setup PIT
	outb(0x43, 0x34);		//Control pin, channel 0, mode 2 rate generator

	outb(0x40, PIT_RELOADVAL & 0xFF);			//Set reload value (low byte, then high byte)
	outb(0x40, PIT_RELOADVAL >> 8);

	// Setup PIT for speaker
	outb(0x43, 0xB6);		//Control pin, channel 2, mode 3 square wave generator

	//Stop beep
	TimerBeepStop();
}

//Gets or sets the current time
// The time is represented as the number of MILIseconds since 1st Jan 1970
TimerTime TimerGetTime()
{
	return currentTime;
}

TimerTime TimerGetStartupTime()
{
	return startupTime;
}

void TimerSetTime(TimerTime newTime)
{
	//Update current time
	currentTime = newTime;
}

//Adds a timer queue item to a given timer queue
static void AddTimerToQueue(TimerQueue * newItem, ListHead * headPtr)
{
	TimerTime time = newItem->endTime;

	ListHeadInit(&newItem->list);

	//Find place to insert into queue
	TimerQueue * currPos;

	ListForEachEntry(currPos, headPtr, list)
	{
		//Is this time more than ours?
		if(time > currPos->endTime)
		{
			//Add before current pos
			ListAddBefore(&newItem->list, &currPos->list);
			return;
		}
	}

	//Add list to end of queue
	ListHeadAddLast(&newItem->list, headPtr);
}

//Waits time milliseconds until returning (current thread only)
// Returns number of milliseconds actually left to sleep (not 0 when interrupted)
TimerTime TimerSleep(TimerTime time)
{
	//Calculate wake up time
	time += currentTime;

	//Create entry
	TimerQueue * newQueueEntry = MAlloc(sizeof(TimerQueue));
	newQueueEntry->thread = ProcCurrThread;
	newQueueEntry->endTime = time;

	//Add to queue
	AddTimerToQueue(newQueueEntry, &sleepQueueHead);

	//Block thread
	if(ProcYieldBlock(true))
	{
		//Interrupted, we must manually remove the queue entry
		ListDelete(&newQueueEntry->list);
		MFree(newQueueEntry);

		//Return difference between current time and given time
		return currentTime - time;
	}

	//If uninterrupted, the queue has already been freed
	return 0;
}

//Sets the PROCESS alarm
// Returns number of seconds left on existing alarm or 0 if no previous alarm existed
// Important: POSIX alarms are PROCESS WIDE.
// Kernel threads should not call this
TimerTime TimerSetAlarm(TimerTime time)
{
	TimerTime timeLeft;
	TimerQueue * queueHead;

	//First, remove previous alarm
	if(ProcCurrProcess->alarmPtr != NULL)
	{
		//Get time left
		queueHead = ListEntry(ProcCurrProcess->alarmPtr, TimerQueue, list);
		timeLeft = queueHead->endTime - currentTime;

		//Remove from list + free
		ListDelete(&queueHead->list);
		MFree(queueHead);

		//Wipe from process
		ProcCurrProcess->alarmPtr = NULL;
	}
	else
	{
		timeLeft = 0;
	}

	//If time is not 0, create new alarm
	if(time != 0)
	{
		//Create alarm
		queueHead = MAlloc(sizeof(TimerQueue));
		queueHead->process = ProcCurrProcess;
		queueHead->endTime = time;

		//Add to list
		AddTimerToQueue(queueHead, &alarmQueueHead);

		//Add to process
		ProcCurrProcess->alarmPtr = &queueHead->list;
	}

	return timeLeft;
}

//Stops the timer from beeping manually (before time is up)
void TimerBeepStop()
{
	//Wipe end time
	beepEndTime = 0;

	//Clear bits 0 and 1 in keyboard controller (yet another use for it)
	outb(0x61, inb(0x61) & 0xFC);
}

//Plays a beep of the given frequency and time from the PC Speaker
// Note the time is supported by the timer interrupt so the beep won't stop
//  while interrupts are disabled
// This will "replace" any beep currently happening
void TimerBeepAdv(unsigned int freq, TimerTime time)
{
	unsigned short reloadValue;

	//Set ending time
	beepEndTime = currentTime + time;

	//Check max and min oscillator frequency
	if(freq < 18)
	{
		freq = 18;
	}
	else if(freq > 596591)
	{
		freq = 596591;
	}

	//Get reload value
	reloadValue = PIT_OSCILLATOR_RATE / freq;

	//Set oscillator frequency
	outb(0x42, reloadValue & 0xFF);		//Channel 2, set reload value
	outb(0x42, reloadValue >> 8);

	//Set keyboard controller bits 0 and 1
	outb(0x61, inb(0x61) | 3);
}

//Timer interrupt
static void TimerInterrupt(IntrContext * iContext)
{
	IGNORE_PARAM iContext;

	//Update system time
	currentTime += PIT_TICKS_PER_INTERRUPT;

	//Stop beep if run out of time
	if(beepEndTime != 0 && currentTime >= beepEndTime)
	{
		TimerBeepStop();
	}

	//Process sleep queue
	if(!ListHeadIsEmpty(&sleepQueueHead))
	{
		TimerQueue * head = ListEntry(sleepQueueHead.next, TimerQueue, list);
		TimerQueue * newHead;

		while(currentTime >= head->endTime)
		{
			//Wake up head
			ProcWakeUp(head->thread);

			//Get next head
			newHead = ListEntry(head->list.next, TimerQueue, list);

			//Remove current from queue
			ListDelete(&head->list);
			MFree(head);

			//Next head
			head = newHead;

			//Check wrapped
			if(ListHeadIsEmpty(&sleepQueueHead))
			{
				break;
			}
		}
	}

	//Process alarm queue
	if(!ListHeadIsEmpty(&alarmQueueHead))
	{
		TimerQueue * head = ListEntry(alarmQueueHead.next, TimerQueue, list);
		TimerQueue * newHead;

		while(currentTime >= head->endTime)
		{
			//Send signal and remove process pointer
			ProcSignalSendProcess(head->process, SIGALRM);
			head->process->alarmPtr = NULL;

			//Get next head
			newHead = ListEntry(head->list.next, TimerQueue, list);

			//Remove current from queue
			ListDelete(&head->list);
			MFree(head);

			//Next head
			head = newHead;

			//Check wrapped
			if(ListHeadIsEmpty(&alarmQueueHead))
			{
				break;
			}
		}
	}
}
