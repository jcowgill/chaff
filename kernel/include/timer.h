/*
 * timer.h
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

#ifndef TIMER_H_
#define TIMER_H_

#include "chaff.h"
#include "interrupt.h"

//Initialises the PIT and PC Speaker
void TimerInit();

//Time representation
// The high 32 bits are the number of seconds since 1st Jan 1970.
// The low 32 bits are fractions of seconds (1 = 1/(2^32) seconds)
typedef signed long long TimerTime;

//Gets or sets the current time
TimerTime TimerGetTime();
TimerTime TimerGetStartupTime();
void TimerSetTime(TimerTime newTime);		//Currently this does not store the time in the RTC and is lost on reboot

//Gets the time stored in the CMOS
// This is much slower than TimeGetTime and shouldn't usually be used since it can be different
TimerTime TimerGetCMOSTime();

//Waits time until returning (current thread only)
// Returns time actually left to sleep (not 0 when interrupted)
TimerTime TimerSleep(TimerTime time);

//Sets the PROCESS alarm
// Returns number of seconds left on existing alarm or 0 if no previous alarm existed
//  Call with 0 time to clear the alarm
// Important: POSIX alarms are PROCESS WIDE.
// Kernel threads should not call this
TimerTime TimerSetAlarm(TimerTime time);

//Stops the timer from beeping manually (before time is up)
void TimerBeepStop();

//Plays a beep of the given frequency and time from the PC Speaker
// Note the time is supported by the timer interrupt so the beep won't stop
//  while interrupts are disabled
// This will "replace" any beep currently happening
void TimerBeepAdv(unsigned int freq, TimerTime time);

//Plays a 1kHz beep for 1s
static inline void TimerBeep()
{
	TimerBeepAdv(1000, 1ULL << 32);
}

//Quantum functions
#define TIMER_INITIAL_QUANTUM 20		//Number of timer ticks per task
extern unsigned int TimerQuantum;

//Returns true if the scheduler should preempt the current thread
static inline bool TimerShouldPreempt()
{
	return TimerQuantum == 0;
}

//Resets the current quantum
static inline void TimerResetQuantum()
{
	TimerQuantum = TIMER_INITIAL_QUANTUM;
}

#endif /* TIMER_H_ */
