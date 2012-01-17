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
		int res = file->ops->close(file);
		if(res != 0)
		{
			return res;
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
		if(context->files[i] == NULL)
		{
			//Return this id
			return i;
		}
	}

	//None found
	return -1;
}

//Opens a new file descriptor in the io context of process
// path = path of file to open
// flags = flags to open file
// mode = mode to create new file with
// fd = file descriptor to use (this will not replace a descriptor)
int IoOpen(ProcProcess * process, const char * path, int flags, IoMode mode, int fd)
{
#warning TODO IoOpen
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
	else if(file->flags & IO_O_DIRECT)
	{
		res = -EISDIR;
	}
	else
	{
#warning Are the buffers given user or kernel mode?
#warning Extra checks (like memory fault or very large count)

		//Forward to filesystem
		res = file->ops->read(file, buffer, count);
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
	else if(file->flags & IO_O_DIRECT)
	{
		res = -EISDIR;
	}
	else
	{
		//Forward to filesystem
		res = file->ops->write(file, buffer, count);
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
	int res = file->ops->ioctl(file, request, data);
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

#warning If this is usermode, add checks here

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
	if(!(file->flags & IO_O_DIRECT))
	{
		res = -ENOTDIR;
	}
	else if(count != 0)
	{
		//Forward to filesystem
		ReadDirFillerBuf buf = { buffer, count };
		res = file->ops->readdir(file, &buf, readDirFiller, count);

		//If sucessful, return the number found
		if(res == 0)
		{
			res = count - buf.count;
		}
	}

	//Release file and return
	IoReleaseFile(file, context, fd);
	return res;
}
