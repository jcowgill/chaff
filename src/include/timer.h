/*
 * timer.h
 *
 *  Created on: 5 Feb 2011
 *      Author: James
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "interrupt.h"

//Initialises the PIT and PC Speaker
void TimerInit();

//Time representation
// The high 32 bits are the number of seconds since 1st Jan 1970.
// The low 32 bits are fractions of seconds (1 = 1/(2^32) seconds)
typedef unsigned long long TimerTime;

//Gets or sets the current time
TimerTime TimerGetTime();
TimerTime TimerGetStartupTime();
void TimerSetTime(TimerTime newTime);

//Waits time until returning (current thread only)
// Returns time actually left to sleep (not 0 when interrupted)
TimerTime TimerSleep(TimerTime time);

//Sets the PROCESS alarm
// Returns number of seconds left on existing alarm or 0 if no previous alarm existed
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
	TimerBeepAdv(1000, 1000);
}

//Timer interrupt
void TimerInterrupt(IntrContext * iContext);

#endif /* TIMER_H_ */
