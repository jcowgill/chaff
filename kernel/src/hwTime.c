/*
 * hwTime.c
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
 *  Created on: 8 Feb 2011
 *      Author: James
 */

#include "chaff.h"
#include "inlineasm.h"
#include "timer.h"

//Hardware time reader
//

#define RTC_ADDR 0x70
#define RTC_DATA 0x71

#define TIMEMODE_24HR 2
#define TIMEMODE_BIN 4

//Sum of all previous days in a year
static const unsigned short monthdays[12] =
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

// leap year
static const unsigned short monthdays2[12] =
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };

//Converts a BCD number to binary
static inline unsigned int BCD2Binary(unsigned char value)
{
	return ((value >> 4) * 10) + (value & 0xF);
}

//Converts a 12-hour time to 24-hour time
static inline unsigned int Hour12To24(unsigned char value, bool useBCD)
{
	//Get PM part
	unsigned char isPM = value & 0x80;

	//Get binary value
	if(useBCD)
	{
		value = BCD2Binary(value & 0x7F);
	}
	else
	{
		value = value & 0x7F;
	}

	//Handle strange midnight
	if(value == 12)
	{
		value = 0;
	}

	//Add 12 for PM
	if(isPM)
	{
		value += 12;
	}

	return value;
}

//Reads a register from the CMOS chip
static inline unsigned char CMOSRead(unsigned char reg, bool bcd)
{
	unsigned char result;
	outb(RTC_ADDR, reg);

	result = inb(RTC_DATA);

	if(bcd)
	{
		result = BCD2Binary(result);
	}

	return result;
}

//Calculates number of days since the epoch with given variables
// DOES NOT WORK with years 2100 and beyond
static inline unsigned int DaysDiff(unsigned int year, unsigned char month,
		unsigned char day)
{
	unsigned int days;

	//Handle years and leap year
	days = (year - 1970) * 365 + ((year - 1970 + 1) / 4);

    //Handle months
    month--;
    if(month > 11)
    {
    	month = 11;
    }

    if(year % 4 == 0)
    {
        days += monthdays2[month];
    }
    else
    {
        days += monthdays[month];
    }

    //Handle days
    days += day - 1;

    return days;
}

TimerTime TimerGetCMOSTime()
{
	//Wait until not updating
	while(CMOSRead(0xA, false) & 0x80)
	{
	}

	//Read format from register B
	unsigned char timeFormat = CMOSRead(0xB, false);

	bool isBCD = !(timeFormat & TIMEMODE_BIN);
	bool is24Hr = timeFormat & TIMEMODE_24HR;

	/*
	Register   Contents
	   0       Seconds
	   2       Minutes
	   4       Hours
	   6       Weekday
	   7       Day of Month

	   8       Month
	   9       Year
	  0x32     Century (usually)
	 */

	unsigned int unixTime;

	//Read time parts
	unixTime  = CMOSRead(0, isBCD);
	unixTime += CMOSRead(2, isBCD) * 60;

	if(is24Hr)
	{
		unixTime += Hour12To24(CMOSRead(4, false), isBCD) * 60 * 60;
	}
	else
	{
		unixTime += CMOSRead(4, isBCD) * 60 * 60;
	}

	//Read days, months, years and century
	unsigned char days = CMOSRead(7, isBCD);
	unsigned char month = CMOSRead(8, isBCD);
	unsigned char year = CMOSRead(9, isBCD);

	//Add days part to unixTime
	unixTime += DaysDiff(year + 2000, month, days) * 24 * 60 * 60;	//Assumes year is between 2000 and 2099

	//Return time shifted all the way to the left
	return ((TimerTime) unixTime) << 32;
}

