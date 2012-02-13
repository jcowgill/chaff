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
#include "secContext.h"

#define IO_MAX_OPEN_FILES 1024
#define IO_NAME_MAX 255

//A file opened by an IoContext
typedef struct IoFile
{
	//Number of times this file has been opened
	// (can be opened in more than one context after forking)
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
	IoFilesystem * cdirFs;
	unsigned int cdirINode;

	//References to this io context
	unsigned int refCount;

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

//Creates a new io context using the root directory as the current directory
IoContext * IoContextCreate();

//Creates a copy of an IO context including all open files
// The files in the new context refer to THE SAME file (sharing offset etc)
IoContext * IoContextClone(IoContext * context);

//Adds a reference to an io context
static inline void IoContextAddReference(IoContext * context)
{
	context->refCount++;
}

//Removes a reference to an IO context
// This must NOT be used when the io context could be in use by another thread
//  (io contexts are not locked)
// This will close all open files
void IoContextDeleteReference(IoContext * context);

//Gets an open file from the current process's context
IoFile * IoGetFile(int fd);

//Gets an open file from the given context
IoFile * IoGetFileWithContext(IoContext * context, int fd);

//Finds the next avaliable file descriptor at least as large as fd
// Returns the file descriptor or -1 if there are none avaliable
int IoFindNextDescriptor(IoContext * context, int fd);

//Looks up a path in the filesystem
// This performs tranverse directory permission checks (any dir in output also has these checks)
// Returns one of:
//  0 		= File found and placed in output
//  -ENOENT = File not found.
//				If the parent directory (of the file) was found:
//				 output is set to the parent and fileStart is set to the beginning of the unknown file
//				If not, fileStart is set to NULL
//  -EISDIR = Path represents a directory - the dir is placed in output
//  other   = Another error, the value in output is undefined
int IoLookupPath(SecContext * secContext, IoContext * ioContext, const char * path,
		IoINode * output, const char ** fileStart);

//Opens a new file descriptor in an io context
// path = path of file to open
// flags = flags to open file
// mode = mode to create new file with
// fd = file descriptor to use (this will not replace a descriptor)
int IoOpen(SecContext * secContext, IoContext * ioContext, const char * path,
		int flags, IoMode mode, int fd);

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

//Truncates a file to a precise length
// If the file gets larger, it is filled with nulls in the extra bits
// The file offset is unchanged
int IoTruncate(IoContext * context, int fd, unsigned long long size);

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
