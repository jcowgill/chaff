/*
 * hwTime.c
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
#define RTC_DATA 0x70

#define TIMEMODE_24HR 4
#define TIMEMODE_BIN 2

//Sum of all previous days in a year
static const unsigned short monthdays[13] =
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

// leap year
static const unsigned short monthdays2[13] =
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };

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

	result = inb(RTC_ADDR);

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
	unsigned int days = 0;

	//Handle years and leap years
    for(unsigned int i = 1970; i < year; i++)
    {
        if(i % 4 == 0)
        {
        	//Leap year
            days += 366;
        }
        else
        {
            days += 365;
        }
    }

    //Handle months
    if(month > 12)
    {
    	month = 12;
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
	unixTime += DaysDiff(year + 2000, month, days);		//Assumes year is between 2000 and 2099

	//Return time shifted all the way to the left
	return ((TimerTime) unixTime) << 32;
}

