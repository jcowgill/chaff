/*
 * open.c
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
 *  Created on: 18 Jan 2012
 *      Author: james
 */

#include "chaff.h"
#include "io/iocontext.h"
#include "io/fs.h"
#include "errno.h"
#include "process.h"
#include "htable.h"

//IoOpen and IoLookupPath calls
//

//Looks up a path in the filesystem using the given process context
// This performs tranverse directory permission checks (any dir in output also has these checks)
// Returns one of:
//  0 		= File found and placed in output
//  -ENOENT = File not found.
//				If the parent directory (of the file) was found:
//				 output is set to the parent and fileStart is set to the beginning of the unknown file
//				If not, fileStart is set to NULL
//  -EISDIR = Path represents a directory - the dir is placed in output
//  other   = Another error, the value in output is undefined
int IoLookupPath(ProcProcess * process, const char * path, IoINode * output, const char ** fileStart)
{
	IoContext * context = process->ioContext;
	IoFilesystem * currFs;
	unsigned int currINode;
	int res;

	*fileStart = NULL;

	//Find top level directory
	if(*path == '\0')
	{
		//Invalid path
		return -ENOENT;
	}
	else if(*path == '/')
	{
		//Use root directory
		if(IoFilesystemRoot == NULL)
		{
			//No files avaliable
			return -ENOENT;
		}

		currFs = IoFilesystemRoot;
		currINode = currFs->ops->getRootINode(currFs);

		path++;
	}
	else
	{
		//Use current directory
		currFs = context->cdirFs;
		currINode = context->cdirINode;
	}

	//Read top level iNode
	output->fs = currFs;
	output->number = currINode;
	res = currFs->ops->readINode(currFs, output);

	if(res != 0)
	{
		return res;
	}

	//Do checks for top level
	if(!IoModeCanAccessINode(IO_WORLD_EXEC, output, process))
	{
		return -EACCES;
	}

	//Start looking up inodes
	for(;;)
	{
		//Find next path element
		int len = 0;
		while(len < IO_NAME_MAX && path[len] != '\0' && path[len] != '/')
		{
			len++;
		}

		//Test for errors and special cases
		if(len == IO_NAME_MAX)
		{
			return -ENAMETOOLONG;
		}
		else if(len == 0)
		{
			//Empty path element
			if(*path == '\0')
			{
				//End of path with trailing /
				// INode must refer to a directory
				if(IO_ISDIR(output->mode))
				{
					//OK
					return -EISDIR;
				}
				else
				{
					return -ENOTDIR;
				}
			}
			else
			{
				//Ignore multiple slashes in a row
				path++;
				continue;
			}
		}

		//Can only read into a directory
		if(!IO_ISDIR(output->mode))
		{
			return -ENOTDIR;
		}

		//Test (some) special paths
		if(len == 1 && *path == '.')
		{
			//Ignore dot path
			path += 2;
			continue;
		}
		else if(len == 2 && path[0] == '.' && path[1] == '.' && currINode == currFs->ops->getRootINode(currFs))
		{
			//Go to parent mount point
			if(currFs->parentFs == NULL)
			{
				//This is the root fs, ignore this
				path += 3;
				continue;
			}
			else
			{
				//Go up
				currINode = currFs->parentINode;
				currFs = currFs->parentFs;

				//Get the iNode
				output->fs = currFs;
				output->number = currINode;
				res = currFs->ops->readINode(currFs, output);

				if(res != 0)
				{
					return res;
				}

				//Do checks
				if(!IoModeCanAccessINode(IO_WORLD_EXEC, output, process))
				{
					return -EACCES;
				}
			}
		}

		//Lookup this name
		res = currFs->ops->findINode(currFs, currINode, path, len, &currINode);
		if(res == -ENOENT)
		{
			//File / path not found
			// Was this the last part of the file?
			if(path[len] == '\0')
			{
				//Yes - so the parent dir was found (and is already in the iNode structure)
				*fileStart = path;
			}

			return -ENOENT;
		}
		else
		{
			return res;
		}

		//Handle mount points
		HashItem * mountPoint = HashTableFind(&currFs->mountPoints, currINode);
		if(mountPoint != NULL)
		{
			//Translate mount point
			currFs = HashTableEntry(mountPoint, IoFilesystem, mountItem);
			currINode = currFs->ops->getRootINode(currFs);
		}

		//Get the iNode
		output->fs = currFs;
		output->number = currINode;
		res = currFs->ops->readINode(currFs, output);

		if(res != 0)
		{
			return res;
		}

		//Do checks
		if(IO_ISDIR(output->mode) && !IoModeCanAccessINode(IO_WORLD_EXEC, output, process))
		{
			return -EACCES;
		}

		//Exit if this is the last part
		if(path[len] == '\0')
		{
			return 0;
		}

		//Move path on and repeat
		path += len + 1;
	}
}

//Opens a new file descriptor in the io context of process
// path = path of file to open
// flags = flags to open file
// mode = mode to create new file with
// fd = file descriptor to use (this will not replace a descriptor)
int IoOpen(ProcProcess * process, const char * path, int flags, IoMode mode, int fd)
{
	IoContext * context = process->ioContext;

	//Check if fd exists and is not reserved
	if(fd < 0 || fd >= IO_MAX_OPEN_FILES || context->files[fd] != NULL ||
			(context->descriptorFlags[fd] & IO_O_FDERSERVED))
	{
		//Already Exists
		return -EINVAL;
	}

	//Validate flags before doing any big operations
	flags &= IO_O_ALLFLAGS;
	if((flags & IO_O_RDWR) == 0)
	{
		return -EINVAL;
	}

	//Trim truncate flag
	if((flags & IO_O_WRONLY) == 0)
	{
		flags &= ~IO_O_TRUNC;
	}

	//Mark descriptor as reserved
	context->descriptorFlags[fd] = IO_O_FDERSERVED;

	//Search for path
	IoINode iNode;
	const char * fileStart;
	bool fileCreated = false;

	int res = IoLookupPath(process, path, &iNode, &fileStart);

	switch(res)
	{
		case 0:
			//Check for directory only
			if(flags & IO_O_DIRECTORY)
			{
				return -ENOTDIR;
			}

			//Check if an exclusive create was requested
			if((flags & IO_O_CREAT) && (flags & IO_O_EXCL))
			{
				//File already exists
				return -EEXIST;
			}

			break;

		case -EISDIR:
			//Check if an exclusive create was requested
			if((flags & IO_O_CREAT) && (flags & IO_O_EXCL))
			{
				//File already exists
				return -EEXIST;
			}

			//Cannot open a directory in write mode
			if(flags & IO_O_WRONLY)
			{
				return -EISDIR;
			}

			//Force directory flag
			flags |= IO_O_DIRECTORY;

			break;

		case -ENOENT:
			//Check if the path exists and we want to create a file
			if(fileStart != NULL && (flags & IO_O_CREAT))
			{
				//Check for directory only
				if(flags & IO_O_DIRECTORY)
				{
					return -ENOTDIR;
				}

				//Check permissions
				if(iNode.fs->flags & IO_MOUNT_RDONLY)
				{
					return -EROFS;
				}

				if(!IoModeCanAccessINode(IO_WORLD_WRITE, &iNode, process))
				{
					return -EACCES;
				}

				//Set mode to use regular files
				mode &= IO_OWNER_ALL | IO_GROUP_ALL | IO_WORLD_ALL;
				mode |= IO_REGULAR;

				//Create file
				unsigned int outINode;
				res = iNode.fs->ops->create(
						iNode.fs,
						&iNode,
						fileStart,
						StrLen(fileStart),
						mode,
						&outINode);

				if(res != 0)
				{
					return res;
				}

				fileCreated = true;

				//Read created iNode
				iNode.number = outINode;
				res = iNode.fs->ops->readINode(iNode.fs, &iNode);
				if(res != 0)
				{
					goto returnError;
				}

				//Force permissions to be granted
				iNode.mode |= IO_OWNER_ALL | IO_GROUP_ALL | IO_WORLD_ALL;
				break;
			}
			else
			{
				return -ENOENT;
			}

		default:
			//Other path lookup error
			return res;
	}

	//Do checks
	//Only open implemented types
	if(IO_ISFIFO(iNode.mode) || IO_ISSOCKET(iNode.mode) || IO_ISSYMLINK(iNode.mode))
	{
		res = -ENOSYS;
		goto returnError;
	}

	//Check readonly fs
	if((iNode.fs->flags & IO_MOUNT_RDONLY) && (flags & IO_O_WRONLY) != 0)
	{
		res = -EROFS;
		goto returnError;
	}

	//Check permissions
	int permsRequired = 0;
	if(flags & IO_O_RDONLY)
	{
		permsRequired |= IO_WORLD_READ;
	}

	if(flags & IO_O_WRONLY)
	{
		permsRequired |= IO_WORLD_WRITE;
	}

	if(!IoModeCanAccessINode(permsRequired, &iNode, process))
	{
		res = -EACCES;
		goto returnError;
	}

	//Create descriptor
	IoFile * file = MAlloc(sizeof(IoFile));
	file->refCount = 1;
	file->off = 0;
	file->flags = flags & (IO_O_RDWR | IO_O_APPEND | IO_O_DIRECTORY);
	file->fs = iNode.fs;
	file->iNode = iNode.number;
	file->ops = iNode.ops;

	res = file->ops->open(&iNode, file);
	if(res != 0)
	{
		MFree(file);
		goto returnError;
	}

	//Truncate file
	if(flags & IO_O_TRUNC)
	{
		res = file->ops->truncate(file, 0);
		if(res != 0)
		{
			MFree(file);
			goto returnError;
		}
	}

	//Place in context
	context->files[fd] = file;
	context->descriptorFlags[fd] = flags & IO_O_CLOEXEC;
	return 0;

	//Returns if an error occured
	// This will remove the file if it was created
returnError:
	if(fileCreated)
	{
#warning Remove created file here?
	}

	//Unreserve fd
	context->descriptorFlags[fd] = 0;
	return res;
}
