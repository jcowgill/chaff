/*
 * iocontext.c
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
 *  Created on: 16 Jan 2012
 *      Author: james
 */

#include "chaff.h"
#include "process.h"
#include "io/iocontext.h"

//Gets an open file from the current process's context
IoFile * IoGetFile(int fd)
{
	//
}

//Finds the next avaliable file descriptor at least as large as fd
// Returns the file descriptor or -1 if there are none avaliable
int IoFindNextDescriptor(IoContext * context, int fd)
{
	//
}

//Opens a new file descriptor in the io context of process
// path = path of file to open
// flags = flags to open file
// mode = mode to create new file with
// fd = file descriptor to use (this will not replace a descriptor)
int IoOpen(ProcProcess * process, const char * path, int flags, IoMode mode, int fd)
{
	//
}

//Closes the file with the given descriptor
int IoClose(IoContext * context, int fd)
{
	//
}

//Reads some bytes from a file descriptor
int IoRead(IoContext * context, int fd, void * buffer, int count)
{
	//
}

//Writes some bytes to a file descriptor
int IoWrite(IoContext * context, int fd, void * buffer, int count)
{
	//
}

//Sends an ioctl request to the filesystem / device connected to the descriptor
int IoIoctl(IoContext * context, int fd, int request, void * data)
{
	//
}

//Duplicates a file descriptor
// flags can be the IO_DUP_ flags or IO_O_CLOEXEC
int IoDup(IoContext * context, int fd, int newFd, int flags)
{
	//
}
