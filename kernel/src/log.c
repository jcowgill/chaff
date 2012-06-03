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
#include "process.h"

#define MAX_STR_LEN 1024

#define TO_STRING_LEN 12

//Flags
#define LEFT_ALIGN	1
#define ZERO_PAD	2
#define ALTERNATE	4

#define PRECISION	8
#define HEX_UPPER	16

//To string result
static char toStringResult[TO_STRING_LEN];

//String buffer variables
static char * strBuffer;
static int strBufferSize;

//Top of the startup stack
// Use &ProcStartupStackTop to get the pointer
extern unsigned int ProcStartupStackTop;

//Prints a stack trace
static void PrintStackTrace() __attribute__ ((noinline));

//Converts the string at *format to an unsigned int
// Stops at the first non-numeric character and sets *format to this char
static unsigned int stringToNumber(const char ** format)
{
	unsigned int num = 0;

	//Do iteration
	while(**format >= '0' && **format <= '9')
	{
		//The shifts multiply num by 10
		num = (num << 3) + (num << 1) + **format - '0';
		(*format)++;
	}

	return num;
}

//Generates a formatted string
// if emitChar returns true, the function exits immediately
static void DoStringFormat(bool (* emitChar)(char), const char * format, va_list args)
{
	//Process the format string
	while(*format)
	{
		//Percent?
		if(*format == '%')
		{
			//Process special format info
			format++;

			//Data
			char prefix[2] = {0, 0};	//Number prefix (+, -, <space> or 0x)
			char * rawStr = NULL;		//Final raw string
			unsigned int rawStrLen;		//Length of raw string
			int flags = 0;				//On off flags
			unsigned int width = 0;		//Minimum width of element
			unsigned int precision = 1;	//Precision of data
			unsigned int absValue;		//Absolute numeric value
			unsigned short base;		//Numeric base (8, 10 or 16)
			int extraZeros = 0;			//Number of extra zeros to add
			int spaces;					//Spaces to add at the end

			//Start control character loop
			// Exit of this loop are gotos to stringHandler and numberHandler
			for(;;)
			{
				switch(*format++)
				{
					case '+':
						//Set plus flag
						prefix[0] = '+';
						break;

					case ' ':
						//Set space flag if not plus already
						if(!prefix[0])
							prefix[0] = ' ';
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
						flags &= ~ZERO_PAD;
						flags |= PRECISION;
						precision = stringToNumber(&format);
						break;

					case '1': case '2': case '3': case '4': case '5':
					case '6': case '7': case '8': case '9':
						//Width
						format--;
						width = stringToNumber(&format);
						break;

					//##########################################

					case 'd':
					case 'i':
						{
							//Print signed integer
							int signedValue = va_arg(args, int);

							//Calculate absolute value (keeping INT_MIN working)
							if(signedValue < 0)
							{
								if(signedValue == INT_MIN)
									absValue = (unsigned int) INT_MAX + 1;
								else
									absValue = (unsigned int) -signedValue;

								prefix[0] = '-';
							}
							else
							{
								absValue = (unsigned int) signedValue;
							}

							//Setup numer parameters
							base = 10;
							goto numberHandler;
						}

					case 'u':
						//Get unsigned int
						absValue = va_arg(args, unsigned int);
						prefix[0] = '\0';
						base = 10;
						goto numberHandler;

					case 'o':
						//Get unsigned int as octal
						absValue = va_arg(args, unsigned int);
						base = 8;

						if(absValue != 0 && flags & ALTERNATE)
							prefix[0] = '0';
						else
							prefix[0] = '\0';

						goto numberHandler;

					case 'p':
						//Pointer
						absValue = va_arg(args, unsigned int);
						flags |= HEX_UPPER;
						base = 16;
						precision = 8;
						width = 0;
						prefix[0] = '0';
						prefix[1] = 'x';

						goto numberHandler;

					case 'x':
						//Get unsigned int as hex
						absValue = va_arg(args, unsigned int);
						base = 16;

						if(absValue != 0 && flags & ALTERNATE)
						{
							prefix[0] = '0';
							prefix[1] = 'x';
						}
						else
						{
							prefix[0] = '\0';
						}

						goto numberHandler;

					case 'X':
						//Get unsigned int as hex
						absValue = va_arg(args, unsigned int);
						base = 16;
						flags |= HEX_UPPER;

						if(absValue != 0 && flags & ALTERNATE)
						{
							prefix[0] = '0';
							prefix[1] = 'X';
						}
						else
						{
							prefix[0] = '\0';
						}

						goto numberHandler;

					case 's':
						//String
						rawStr = va_arg(args, char *);

						//Null?
						if(rawStr == NULL)
						{
							rawStr = "(null)";
							rawStrLen = precision = 6;
						}
						else
						{
							//Find length of string and store as the precision
							unsigned int maxLen = (flags & PRECISION) ? precision : UINT_MAX;
							rawStrLen = precision = StrLen(rawStr, maxLen);
						}

						goto stringHandler;

					case 'c':
						//Character
						toStringResult[0] = (char) va_arg(args, int);
						rawStr = &toStringResult[0];
						rawStrLen = precision = 1;
						goto stringHandler;

					case '%':
						//Literal %
						toStringResult[0] = '%';
						rawStr = &toStringResult[0];
						rawStrLen = precision = 1;
						goto stringHandler;

					default:
						//Invalid char
						return;
				}
			}

		numberHandler:
			//Convert number to string
			rawStr = &toStringResult[TO_STRING_LEN - 1];
			rawStrLen = 0;

			//Iterate over each character to generate
			while(absValue > 0)
			{
				//Decrement pointer
				rawStr--;

				//Hex?
				if(absValue % base >= 10)
					*rawStr = ((flags & HEX_UPPER) ? 'A' : 'a') - 10 + absValue % base;
				else
					*rawStr = '0' + absValue % base;

				//Next number
				absValue /= base;

				//Increment length
				rawStrLen++;
			}

			//Calculate number of extra 0s to add
			int prefixLen = prefix[0] ? (prefix[1] ? 2 : 1) : 0;

			if(flags & ZERO_PAD && !(flags & LEFT_ALIGN))
			{
				extraZeros = width - rawStrLen - prefixLen;
			}
			else
			{
				extraZeros = precision - rawStrLen;
			}

			//Ensure extraZeros is positive
			if(extraZeros < 0)
				extraZeros = 0;

			//Set precision to correct "inner" length
			precision = rawStrLen + extraZeros + prefixLen;

		stringHandler:
			//Calculate number of padding spaces to add
			spaces = width - precision;

			//Add left padding
			if(!(flags & LEFT_ALIGN))
			{
				for(; spaces > 0; spaces--)
				{
					emitChar(' ');
				}
			}

			//Print prefix
			if(prefix[0])
			{
				emitChar(prefix[0]);

				if(prefix[1])
					emitChar(prefix[1]);
			}

			//Print extra zeros
			for(; extraZeros > 0; extraZeros--)
			{
				emitChar('0');
			}

			//Print string
			for(; rawStrLen > 0; rawStrLen--)
			{
				emitChar(*rawStr);
				rawStr++;
			}

			//Add right padding
			if(flags & LEFT_ALIGN)
			{
				for(; spaces > 0; spaces--)
				{
					emitChar(' ');
				}
			}
		}
		else
		{
			//Print character
			emitChar(*format);
			format++;
		}
	}
}

//Emit character to string
static bool SPrintEmitChar(char c)
{
	//Emit char
	*strBuffer++ = c;
	strBufferSize--;

	//Exit if only 1 char left
	return (strBufferSize <= 1);
}

//Generates a formatted string
void SPrintFVarArgs(char * buffer, int bufSize, const char * format, va_list args)
{
	//Only do the formatting if the buffer is large enough
	if(bufSize > 0)
	{
		if(bufSize > 1)
		{
			//Format string
			strBuffer = buffer;
			strBufferSize = bufSize;
			DoStringFormat(SPrintEmitChar, format, args);
		}

		//Append null character
		*strBuffer = '\0';
	}
}

//Emit character to string
static bool PrintLogEmitChar(char c)
{
	//Print character
	static char * nextPos = (char *) 0xC00B8000;

	//Newline?
	if(c == '\n')
	{
		//Next line
		nextPos += 160 - ((((unsigned int) nextPos) - 0xC00B8000) % 160);
	}
	else
	{
		//Just print character
		*nextPos++ = c;
		*nextPos++ = 7;
	}

	//Wrap around if reached the end
	if((unsigned int) nextPos >= 0xC00B8FA0)
	{
		nextPos = (char *) 0xC00B8000;
	}

	return false;
}

//Prints a message to the kernel log
void PrintLogVarArgs(LogLevel level, const char * format, va_list args)
{
	//Log strings
	static const char * logLevelStrings[] =
	{
		[Fatal] 	= "Panic",
		[Critical] 	= "Critical",
		[Error] 	= "Error",
		[Warning] 	= "Warning",
		[Notice] 	= "Notice",
		[Info] 		= "Info",
		[Debug] 	= "Debug",
	};

	//Validate level
	if(level < Fatal || level > Debug)
	{
		//Change to notice
		level = Notice;
	}

	//Print level
	const char * levelStr = logLevelStrings[level];

	do
	{
		PrintLogEmitChar(*levelStr++);
	}
	while(*levelStr);

	//Print space
	PrintLogEmitChar(':');
	PrintLogEmitChar(' ');

	//Print formatted message
	DoStringFormat(PrintLogEmitChar, format, args);
	PrintLogEmitChar('\n');
}

//Print a string without newline at the end
static void RawPrint(const char * format, ...)
{
	va_list args;
	va_start(args, format);
	DoStringFormat(PrintLogEmitChar, format, args);
	va_end(args);
}

//Prints a stack trace
static void PrintStackTrace()
{
	//Print header
	RawPrint("Call Stack:\n");

	//Get base pointer
	unsigned int * firstEbp;
	unsigned int * ebp;
	asm ("movl %%ebp, %0\n":"=g"(firstEbp));
	ebp = firstEbp;

	//Get base of the stack
	unsigned int * stackBase;
	if(ProcCurrThread->kStackBase == NULL)
	{
		//This is the init dummy thread, so use the top of the startup stack
		stackBase = &ProcStartupStackTop;
	}
	else
	{
		stackBase = ProcCurrThread->kStackBase + PROC_KSTACK_SIZE;
	}

	//Process each call in the stack
	while(ebp < stackBase && ebp >= firstEbp)
	{
		//Print the call
		RawPrint(" %p\n", ebp[1]);

#warning TODO print symbol for this function

		//Move to next stack frame
		ebp = (unsigned int *) ebp[0];
	}
}

//Fatal error
void NORETURN Panic(const char * format, ...)
{
	//Print message
	va_list args;
	va_start(args, format);
	PrintLogVarArgs(Fatal, format, args);
	va_end(args);

	//Print stack trace
	PrintStackTrace();

	//Hang
	for(;;)
	{
		asm volatile("cli\n"
					 "hlt\n");
	}
}
