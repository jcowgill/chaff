/*
 * timer.c
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

static void UpdateTimeFromCMOS();

//Initialises the PIT and PC Speaker
void TimerInit()
{
	//Read time from CMOS
	UpdateTimeFromCMOS();

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

	//Store in CMOS
#warning Write time to CMOS
}

//Waits time miliseconds until returning (current thread only)
// Returns number of milliseconds actually left to sleep (not 0 when interrupted)
TimerTime TimerSleep(TimerTime time)
{
	//
}

//Sets the PROCESS alarm
// Returns number of seconds left on existing alarm or 0 if no previous alarm existed
// Important: POSIX alarms are PROCESS WIDE.
// Kernel threads should not call this
TimerTime TimerSetAlarm(TimerTime time)
{
	//
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
void TimerInterrupt(IntrContext * iContext)
{
	//Update system time
	currentTime += PIT_TICKS_PER_INTERRUPT;

	//Stop beep if run out of time
	if(beepEndTime != 0 && currentTime >= beepEndTime)
	{
		TimerBeepStop();
	}
}
