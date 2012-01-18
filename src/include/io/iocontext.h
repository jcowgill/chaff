/*
 * iocontext.h
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
 *  Created on: 15 Jan 2012
 *      Author: james
 */

#ifndef IO_CONTEXT_H_
#define IO_CONTEXT_H_

//This file contains io context and file structures
// This is the "font-end" to the io system (exept mounts)

#include "chaff.h"
#include "io/mode.h"
#include "io/fs.h"

#define IO_MAX_OPEN_FILES 1024

struct ProcProcess;

//A file opened by an IoContext
typedef struct IoFile
{
	//Number of times this file has been opened per context
	unsigned int refCount;

	//File specific info
	unsigned int off;
	int flags;

	//File reference
	struct IoFilesystem * fs;
	unsigned int iNode;
	void * fsData;

	//File ops
	IoFileOps * ops;

} IoFile;

//The context which IO operations are run in
typedef struct IoContext
{
	//Open files
	IoFile * files[IO_MAX_OPEN_FILES];
	char descriptorFlags[IO_MAX_OPEN_FILES];	//Flags associated with a DESCRIPTOR
	int nextFreeFile;							//First free file id is at least this

	//Current directory
	IoFilesystem * dirFs;
	unsigned int dirINode;

} IoContext;

//IO "front-end" functions
//

#define IO_DUP_AT_LEAST		1		//use next free descriptor after new
#define IO_DUP_IGNORE_SAME	2		//ignore request (instead of error) if old == new

#define IO_O_RDONLY		0x01
#define IO_O_WRONLY		0x02
#define IO_O_RDWR 		(IO_O_RDONLY | IO_O_WRONLY)
#define IO_O_CREAT		0x04
#define IO_O_TRUNC		0x08
#define IO_O_APPEND		0x10
#define IO_O_EXCL		0x20
#define IO_O_CLOEXEC	0x40		//Descriptor flag
#define IO_O_DIRECTORY	0x80

#define IO_O_ALLFLAGS	0xFF

#define IO_O_FDERSERVED	0x01		//Descriptor flag (used internally)

//Gets an open file from the current process's context
IoFile * IoGetFile(int fd);

//Gets an open file from the given context
IoFile * IoGetFileWithContext(IoContext * context, int fd);

//Finds the next avaliable file descriptor at least as large as fd
// Returns the file descriptor or -1 if there are none avaliable
int IoFindNextDescriptor(IoContext * context, int fd);

//Opens a new file descriptor in the io context of process
// path = path of file to open
// flags = flags to open file
// mode = mode to create new file with
// fd = file descriptor to use (this will not replace a descriptor)
int IoOpen(struct ProcProcess * process, const char * path, int flags, IoMode mode, int fd);

//Closes the file with the given descriptor
int IoClose(IoContext * context, int fd);

//Closes all files marked as CLOEXEC
// Any errors are discarded
void IoCloseForExec(IoContext * context);

//Reads some bytes from a file descriptor
int IoRead(IoContext * context, int fd, void * buffer, int count);

//Writes some bytes to a file descriptor
int IoWrite(IoContext * context, int fd, void * buffer, int count);

//Sends an ioctl request to the filesystem / device connected to the descriptor
int IoIoctl(IoContext * context, int fd, int request, void * data);

//Duplicates a file descriptor
// flags can be the IO_DUP_ flags or IO_O_CLOEXEC
int IoDup(IoContext * context, int fd, int newFd, int flags);

//Structure used as the output to readdir requests
typedef struct
{
	//INode of this file
	unsigned int iNode;

	//Name of this file (null terminated)
	char name[256];

} IoReadDirEntry;

//Reads up to count directories from the directory opened by the given file descriptor
// Returns the number of directories read (if less than count, there are no more) or a negative
// number on error.
int IoReadDir(IoContext * context, int fd, IoReadDirEntry * buffer, int count);

#endif /* IO_CONTEXT_H_ */
