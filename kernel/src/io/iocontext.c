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
#include "errno.h"
#include "mm/check.h"

//This file contains most io front-end functions except open
// which is located in open.c

//Structure used for readDirFiller
typedef struct
{
	IoReadDirEntry * nextEntry;
	unsigned int count;

} ReadDirFillerBuf;

//Filler for readdir
static int readDirFiller(void * buf, unsigned int iNode, const char * name, int len);

//Creates a new file object "var" with the file from a context and fd
// Returns -EBADF with an invalid file
// After using this, you must call IoReleaseFile before returning
#define IO_GET_FILE(var, context, fd) \
	IoFile * var = IoGetFileWithContext((context), (fd)); \
	if(var == NULL) \
		return -EBADF; \
	else \
		var->refCount++;

//Releases a file obtained with IO_GET_FILE
// This may close the file if it was closed DURING the operation
static int IoReleaseFile(IoFile * file, IoContext * context, int fd)
{
	//Destroy file if ref count will become 0
	if(file->refCount <= 1)
	{
		//Destroy file
		if(file->ops->close)
		{
			int res = file->ops->close(file);
			if(res != 0)
			{
				return res;
			}
		}

		//Ensure file still exists (close may block)
		if(context->files[fd] == file)
		{
			//Free file
			MFree(file);

			//Delete descriptor reference
			context->files[fd] = NULL;

			//Update free pointer
			if(context->nextFreeFile < fd)
			{
				context->nextFreeFile = fd;
			}
		}
	}
	else
	{
		//Decrement ref count
		file->refCount--;
	}

	return 0;
}

//Creates a new io context using the root directory as the current directory
IoContext * IoContextCreate()
{
	//Get filesystem root
	IoFilesystem * fs = IoFilesystemRoot;

	if(fs == NULL)
	{
		return NULL;
	}

	//Allocate context
	IoContext * context = MAlloc(sizeof(IoContext));

	//Wipe stuff
	MemSet(context->files, 0, sizeof(context->files));
	MemSet(context->descriptorFlags, 0, sizeof(context->descriptorFlags));
	context->nextFreeFile = 0;

	//Use root filesystem as current directory
	context->cdirFs = fs;
	context->cdirINode = fs->rootINode;

	//Init ref count
	context->refCount = 1;

	return context;
}

//Creates a copy of an IO context including all file positions as flags
IoContext * IoContextClone(IoContext * context)
{
	//Allocate context
	IoContext * newContext = MAlloc(sizeof(IoContext));

	//Copy the entire context
	MemCpy(newContext, context, sizeof(IoContext));

	//Ref count of new context is 1
	newContext->refCount = 1;

	//Increment ref counts on all files
	for(int i = 0; i < IO_MAX_OPEN_FILES; i++)
	{
		if(newContext->files[i])
		{
			newContext->files[i]->refCount++;
		}
	}

	return newContext;
}

//Removes a reference to an IO context
// This must NOT be used when the io context could be in use by another thread
//  (io contexts are not locked)
// This will close all open files
void IoContextDeleteReference(IoContext * context)
{
	//Decrement ref count
	if(context->refCount <= 1)
	{
		//Check each file in the context
		for(int i = 0; i < IO_MAX_OPEN_FILES; i++)
		{
			//Is file open?
			if(context->files[i])
			{
				//Close our reference
				IoReleaseFile(context->files[i], context, i);
			}
		}

		//Free context
		MFree(context);
	}
	else
	{
		context->refCount--;
	}
}

//Gets an open file from the current process's context
IoFile * IoGetFile(int fd)
{
	//Get file with given descriptor
	if(fd >= 0 && fd < IO_MAX_OPEN_FILES && ProcCurrProcess->ioContext != NULL)
	{
		return ProcCurrProcess->ioContext->files[fd];
	}
	else
	{
		return NULL;
	}
}

//Gets an open file from the given context
IoFile * IoGetFileWithContext(IoContext * context, int fd)
{
	//Get file with given descriptor
	if(fd >= 0 && fd < IO_MAX_OPEN_FILES)
	{
		return context->files[fd];
	}
	else
	{
		return NULL;
	}
}

//Finds the next avaliable file descriptor at least as large as fd
// Returns the file descriptor or -1 if there are none avaliable
int IoFindNextDescriptor(IoContext * context, int fd)
{
	//Get starting descriptor
	int start = context->nextFreeFile;
	if(fd > start)
	{
		start = fd;
	}

	//Check each descriptor in turn
	for(int i = start; i < IO_MAX_OPEN_FILES; i++)
	{
		//Free?
		if(context->files[i] == NULL && (context->descriptorFlags[i] & IO_O_FDRESERVED) == 0)
		{
			//Return this id
			return i;
		}
	}

	//None found
	return -1;
}

//Closes the file with the given descriptor
int IoClose(IoContext * context, int fd)
{
	//Get file
	IO_GET_FILE(file, context, fd);

	//Relase twice (once for this reference and once for the actual close)
	file->refCount--;
	return IoReleaseFile(file, context, fd);
}

//Closes all files marked as CLOEXEC
// Any errors are discarded
void IoCloseForExec(IoContext * context)
{
	//Process each file
	for(int i = 0; i < IO_MAX_OPEN_FILES; i++)
	{
		//Close if nessesary
		if(context->files[i] != NULL && (context->descriptorFlags[i] & IO_O_CLOEXEC))
		{
			IoClose(context, i);
		}
	}
}

//Reads some bytes from a file descriptor
int IoRead(IoContext * context, int fd, void * buffer, int count)
{
	//Get file
	int res;
	IO_GET_FILE(file, context, fd);

	//Check if open for reading
	if(!(file->flags & IO_O_RDONLY))
	{
		res = -EBADF;
	}
	else if(file->flags & IO_O_DIRECTORY)
	{
		res = -EISDIR;
	}
	else if(!MemCanRead(buffer, count))
	{
		//Basic memory checks
		res = -EFAULT;
	}
	else
	{
		//Forward to filesystem
		if(file->ops->read)
		{
			res = file->ops->read(file, buffer, count);

			//Advance offset
			if(res > 0)
			{
				file->off += res;
			}
		}
		else
		{
			res = -ENOSYS;
		}
	}

	//Release file and return
	IoReleaseFile(file, context, fd);
	return res;
}

//Writes some bytes to a file descriptor
int IoWrite(IoContext * context, int fd, void * buffer, int count)
{
	//Get file
	int res;
	IO_GET_FILE(file, context, fd);

	//Check if open for writing
	if(!(file->flags & IO_O_WRONLY))
	{
		res = -EBADF;
	}
	else if(file->flags & IO_O_DIRECTORY)
	{
		res = -EISDIR;
	}
	else if(!MemCanWrite(buffer, count))
	{
		//Basic memory checks
		res = -EFAULT;
	}
	else
	{
		//Forward to filesystem
		if(file->ops->write)
		{
			res = file->ops->write(file, buffer, count);

			//Advance offset
			if(res > 0)
			{
				file->off += res;
			}
		}
		else
		{
			res = -ENOSYS;
		}
	}

	//Release file and return
	IoReleaseFile(file, context, fd);
	return res;
}

//Sends an ioctl request to the filesystem / device connected to the descriptor
int IoIoctl(IoContext * context, int fd, int request, void * data)
{
	//Get file
	IO_GET_FILE(file, context, fd);

	//Forward to filesystem
	int res;

	if(file->ops->ioctl)
	{
		res = file->ops->ioctl(file, request, data);
	}
	else
	{
		res = -ENOTTY;		//Not a device
	}

	IoReleaseFile(file, context, fd);
	return res;
}

//Truncates a file to a precise length
// If the file gets larger, it is filled with nulls in the extra bits
// The file offset is unchanged
int IoTruncate(IoContext * context, int fd, unsigned long long size)
{
	//Get file
	IO_GET_FILE(file, context, fd);

	//Forward to filesystem
	int res;

	if(file->ops->truncate)
	{
		res = file->ops->truncate(file, size);
	}
	else
	{
		res = -ENOSYS;		//Function not implemented
	}

	IoReleaseFile(file, context, fd);
	return res;
}

//Duplicates a file descriptor
// flags can be the IO_DUP_ flags or IO_O_CLOEXEC
int IoDup(IoContext * context, int fd, int newFd, int flags)
{
	//Get file
	int res = 1;			//1 = OK
	IO_GET_FILE(file, context, fd);

	//Determine new descriptor
	if(flags & IO_DUP_AT_LEAST)
	{
		newFd = IoFindNextDescriptor(context, newFd);

		//Check for invalid descriptor
		if(newFd == -1)
		{
			res = -EMFILE;
		}
	}
	else if(fd == newFd)
	{
		//Cannot duplicate into self
		if(flags & IO_DUP_IGNORE_SAME)
		{
			res = 0;
		}
		else
		{
			res = -EINVAL;
		}
	}
	else if(newFd < 0 || newFd >= IO_MAX_OPEN_FILES)
	{
		//Invalid descriptor
		res = -EBADF;
	}
	else if(context->descriptorFlags[newFd] & IO_O_FDRESERVED)
	{
		//Cannot duplicate into reserved descriptor
		res = -EBUSY;
	}
	else if(context->files[newFd] != NULL)
	{
		//Close descriptor first
		int closeRes = IoClose(context, newFd);
		if(closeRes != 0)
		{
			res = closeRes;
		}
	}

	//If ok, Duplicate reference
	if(res == 1)
	{
		context->files[newFd] = file;
		context->descriptorFlags[newFd] = flags & IO_O_CLOEXEC;
		file->refCount++;
		res = 0;
	}

	//Release file and return
	IoReleaseFile(file, context, fd);
	return res;
}

//Function called by filesystems to fill readdir entry
static int readDirFiller(void * buf, unsigned int iNode, const char * name, int len)
{
	//Called to fill buffer
	ReadDirFillerBuf * rdBuffer = (ReadDirFillerBuf *) buf;
	IoReadDirEntry * entry = rdBuffer->nextEntry;

	//If count is 0, overflow
	if(rdBuffer->count == 0)
	{
		return -EINVAL;
	}

	//Basic memory checks
	if(!MemCommitForWrite(entry, sizeof(IoReadDirEntry)))
	{
		return -EFAULT;
	}

	//Fill info
	entry->iNode = iNode;
	if(len > 255)
	{
		len = 255;
	}

	MemCpy(entry->name, name, len);
	entry->name[len] = '\0';

	//Move to next entry
	rdBuffer->nextEntry++;
	rdBuffer->count--;
	return 0;
}

//Reads up to count directories from the directory opened by the given file descriptor
// Returns the number of directories read (if less than count, there are no more) or a negative
// number on error.
int IoReadDir(IoContext * context, int fd, IoReadDirEntry * buffer, int count)
{
	//Get file
	int res = 0;
	IO_GET_FILE(file, context, fd);

	//Must be a directory
	if(!(file->flags & IO_O_DIRECTORY))
	{
		res = -ENOTDIR;
	}
	else if(!MemCanWrite(buffer, count * sizeof(IoReadDirEntry)))
	{
		//Basic memory checks
		res = -EFAULT;
	}
	else if(count != 0)
	{
		//Forward to filesystem
		ReadDirFillerBuf buf = { buffer, count };
		if(file->ops->readdir)
		{
			res = file->ops->readdir(file, &buf, readDirFiller, count);

			//If sucessful, return the number found
			if(res == 0)
			{
				res = count - buf.count;
			}
		}
		else
		{
			res = -ENOSYS;
		}
	}

	//Release file and return
	IoReleaseFile(file, context, fd);
	return res;
}
