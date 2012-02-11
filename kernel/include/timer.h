/**
 * @file
 * Timer, clock and sleeping functions
 *
 * @date February 2011
 * @author James Cowgill
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

#ifndef TIMER_H_
#define TIMER_H_

#include "chaff.h"
#include "interrupt.h"

/**
 * Initialises the PIT and PC Speaker
 *
 * @private
 */
void TimerInit();

/**
 * Time representation
 *
 * The high 32 bits are the number of seconds since 1st Jan 1970.
 * The low 32 bits are fractions of seconds (1 = 1/(2^32) seconds)
 */
typedef signed long long TimerTime;

/**
 * Returns the current time
 *
 * @return the time
 */
TimerTime TimerGetTime();

/**
 * Returns the time the system was started
 *
 * @return the time the system started
 */
TimerTime TimerGetStartupTime();

/**
 * Sets the system time
 *
 * @param newTime new time for the system
 * @bug currently does not store in real time clock
 */
void TimerSetTime(TimerTime newTime);

/**
 * Returns the time stored in the CMOS
 *
 * This is much slower than TimeGetTime and shouldn't usually be used since it can be different
 * @return the time in the CMOS
 */
TimerTime TimerGetCMOSTime();

/**
 * Sleeps for at least the specified length of time
 *
 * @param time time to sleep form
 * @return time left to sleep (>0 when interrupted)
 */
TimerTime TimerSleep(TimerTime time);

/**
 * Sets the @a process wide alarm
 *
 * Kernel threads should not call this
 *
 * @param time new time on alarm or 0 to cancel the alarm
 * @return time left on existing alarm or 0 if no previous alarm existed
 */
TimerTime TimerSetAlarm(TimerTime time);

/**
 * Stops the PS Speaker sound
 */
void TimerBeepStop();

/**
 * Plays a beep of the given frequency from the PC Speaker
 *
 * This "replaces" any beep request currently happening
 *
 * @param freq frequency of the sound in Hz
 * @param time time the beep should last for
 */
void TimerBeepAdv(unsigned int freq, TimerTime time);

/**
 * Plays a 1kHz beep for 1s
 */
static inline void TimerBeep()
{
	TimerBeepAdv(1000, 1000);
}

/**
 * Number of timer ticks given to each task
 */
#define TIMER_INITIAL_QUANTUM 20

/**
 * Number of ticks left for the current thread
 */
extern unsigned int TimerQuantum;

/**
 * Returns true if the scheduler should preempt the current thread
 */
static inline bool TimerShouldPreempt()
{
	return TimerQuantum == 0;
}

/**
 * Resets the timer quantum
 */
static inline void TimerResetQuantum()
{
	TimerQuantum = TIMER_INITIAL_QUANTUM;
}

#endif /* TIMER_H_ */
