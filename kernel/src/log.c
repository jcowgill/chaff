/*
 * log.c
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
 *  Created on: 27 May 2012
 *      Author: James
 */

#include "chaff.h"

#define TO_STRING_LEN 12

#define HEX_ABASE_UPPER ('A' - 10)
#define HEX_ABASE_LOWER ('a' - 10)

//Flags
#define LEFT_ALIGN	1
#define ZERO_PAD	2
#define ALTERNATE	4

static char printLogBuffer[1024];
static char toStringResult[TO_STRING_LEN];

//Converts decimal number to string and stores in toStringResult
// Returns the index where the string starts
static char * toDecimalString(unsigned int num)
{
	char * resultAddr = &toStringResult[TO_STRING_LEN - 1];

	//Do iteration
	do
	{
		//Store char
		*--resultAddr = '0' + num % 10;

		//Next number
		num /= 10;
	}
	while(num > 0);

	//Return initial location
	return resultAddr;
}

//Converts decimal number to string and stores in toStringResult
// Returns the index where the string starts
static char * toOctalString(unsigned int num)
{
	char * resultAddr = &toStringResult[TO_STRING_LEN - 1];

	//Do iteration
	do
	{
		//Store char
		*--resultAddr = '0' + num % 8;

		//Next number
		num /= 8;
	}
	while(num > 0);

	//Return initial location
	return resultAddr;
}

//Converts hex number to string and stores in toStringResult
// aBase should be HEX_ABASE_UPPER or HEX_ABASE_LOWER for upper or lower case
// Returns the index where the string starts (NOT null-terminated)
static char * toHexString(unsigned int num, char aBase)
{
	char * resultAddr = &toStringResult[TO_STRING_LEN - 1];

	//Do iteration
	do
	{
		//Hex?
		if(num % 16 >= 10)
		{
			*--resultAddr = aBase + num % 16;
		}
		else
		{
			*--resultAddr = '0' + num % 16;
		}

		//Next number
		num /= 16;
	}
	while(num > 0);

	//Return initial location
	return resultAddr;
}

//Converts the string at *format to an unsigned int
// Stops at the first non-numeric character and sets *format to this char
static unsigned int stringToNumber(const char ** format)
{
	unsigned int num = 0;

	//Do iteration
	while(**format >= '0' && **format <= '9')
	{
		//The shifts multiply num by 10
		num = (num << 3 + num << 1) + **format - '0';
		(*format)++;
	}

	return num;
}

//Prints a message to the kernel log
void PrintLogVarArgs(LogLevel level, const char * format, va_list args)
{
}

//Generates a formatted string
void SPrintFVarArgs(char * buffer, int bufSize, const char * format, va_list args)
{
	//Ignore for blank buffer size
	if(bufSize <= 0)
		return;

	//Process the format string
	while(*format)
	{
		//Percent?
		if(*format == '%')
		{
			//Process special format info
			format++;

			//Data
			char sign = '\0';		//+, - or <space>
			char * rawStr = NULL;	//Final raw string
			int flags = 0;			//On off flags
			unsigned int width;		//Minimum width of element
			unsigned int precision;	//Precision of data

			//Start control character loop
			while(!rawStr)
			{
				switch(*format++)
				{
					case '+':
						//Set plus flag
						sign = '+';
						break;

					case ' ':
						//Set space flag if not plus already
						if(!sign)
							sign = ' ';
						break;

					case '#':
						//Alternate
						flags |= ALTERNATE;
						break;

					case '-':
						//Left align
						flags |= LEFT_ALIGN;
						break;

					case '0':
						//Zero pad
						flags |= ZERO_PAD;
						break;

					case '.':
						//Precision
						precision = stringToNumber(&format);
						break;

					case '1': case '2': case '3': case '4': case '5':
					case '6': case '7': case '8': case '9':
						//Width
						format--;
						precision = stringToNumber(&format);
						break;

					case 'd':
					case 'i':
						{
							//Print signed integer
							int signedValue = va_arg(args, int);
							unsigned int absValue;

							//Calculate absolute value (keeping INT_MIN working)
							if(signedValue == INT_MIN)
							{
								absValue = (unsigned int) INT_MAX + 1;
							}
							else if(signedValue < 0)
							{
								absValue = -signedValue;
								sign = '-';
							}
							else
							{
								absValue = signedValue;
							}

							//Get unsigned value
							rawStr = toDecimalString(absValue);
							break;
						}

					case 'u':
						//Get unsigned int
						rawStr = toDecimalString(va_arg(args, unsigned int));
						break;

					case 'x':
						//Get unsigned int as hex
						rawStr = toHexString(va_arg(args, unsigned int), HEX_ABASE_LOWER);
						break;

					case 'X':
						//Get unsigned int as hex
						rawStr = toHexString(va_arg(args, unsigned int), HEX_ABASE_UPPER);
						break;

					case 'o':
						//Get unsigned int as octal
						rawStr = toOctalString(va_arg(args, unsigned int));
						break;

					case 's':
						//String
						rawStr = va_arg(args, char *);
						break;

					case 'c':
						//Character
						toStringResult[TO_STRING_LEN - 1] = (char) va_arg(args, int);
						rawStr = &toStringResult[TO_STRING_LEN - 1];
						break;

					case 'p':
						//Pointer
#error Set some flags here
						rawStr = toHexString(va_arg(args, unsigned int), HEX_ABASE_UPPER);
						break;

					case '%':
						//Literal %
						toStringResult[TO_STRING_LEN - 1] = '%';
						rawStr = &toStringResult[TO_STRING_LEN - 1];
						break;

					default:
						//Invalid char
						PrintLog(Error, "SPrintF: Invalid character in format string / premature end of format");
						*buffer = '\0';
						return;
				}
			}

			//
#error TODO Do the printing
		}
		else
		{
			//Print character
#warning Print char
			format++;
		}
	}
}

void NORETURN Panic(const char * format, ...)
{
	//Print message
	va_list args;
	va_start(args, format);
	PrintLogVarArgs(Fatal, format, args);
	va_end(args);

	//Hang
	for(;;)
	{
		asm volatile("cli\n"
					 "hlt\n");
	}
}

static char * nextPos = (char *) 0xC00B8000;

void PrintStr(const char * msg)
{
	//Prints a string to the video output
	while(*msg)
	{
		*nextPos++ = *msg++;
		*nextPos++ = 7;

		//Wrap around if reached the end
		if((unsigned int) nextPos >= 0xC00B8FA0)
		{
			nextPos = (char *)0xC00B8000;
		}
	}
}

/*
void PrintLog(LogLevel level, const char * msg, ...)
{
	//Print level
	switch(level)
	{
	case Fatal:
		PrintStr("Panic: ");
		break;

	case Critical:
		PrintStr("Critical: ");
		break;

	case Error:
		PrintStr("Error: ");
		break;

	case Warning:
		PrintStr("Warning: ");
		break;

	case Notice:
		PrintStr("Notice: ");
		break;

	case Info:
		PrintStr("Info: ");
		break;

	case Debug:
		PrintStr("Debug: ");
		break;
	}

	//Print message
	PrintStr(msg);

	//New line + wrap around
	nextPos += 160 - ((((unsigned int) nextPos) - 0xC00B8000) % 160);
	if((unsigned int) nextPos >= 0xC00B8FA0)
	{
		nextPos = (char *)0xC00B8000;
	}
}
*/
