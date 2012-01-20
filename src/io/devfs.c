/*
 * devfs.c
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
#include "io/device.h"
#include "io/fs.h"
#include "io/iocontext.h"
#include "io/bcache.h"
#include "htable.h"
#include "errno.h"

#define MAX_DEVICES 1024
#define GET_DEVICE(var, inode) IoDevice * var; \
	if((inode) >= MAX_DEVICES || devices[(inode)] == NULL) return -ENXIO; \
	var = devices[(inode)];

//Internal functions
static bool DevFsGetKey(HashItem * item, const char ** str, int * len);
static int DevFsMount(IoFilesystem * newFs);
static unsigned int DevFsGetRootINode(IoFilesystem * fs);
static int DevFsReadINode(IoFilesystem * fs, IoINode * iNode);
static int DevFsFindINode(IoFilesystem * fs, unsigned int parent,
		const char * name, int nameLen, unsigned int * iNodeNum);
static int DevFsOpen(IoINode * iNode, IoFile * file);
static int DevFsClose(IoFile * file);
static int DevFsRead(IoFile * file, void * buffer, unsigned int count);
static int DevFsWrite(IoFile * file, void * buffer, unsigned int count);
static int DevFsIoCtl(IoFile * file, int request, void * data);
static int DevFsReadDir(IoFile * file, void * buf, IoDirectoryFiller filler, int count);

//Filesystem Structures
static IoDevice * devices[MAX_DEVICES];
static unsigned int nextFreeINode = 0;

static HashTableString files =
	{
			.key = DevFsGetKey,
	};

static IoFilesystemType fsType =
	{
			.name = "devfs",
			.fsTypes = LIST_INLINE_INIT(fsType.fsTypes),
			.mount = DevFsMount
	};

static IoFilesystemOps fsOps =
	{
			.getRootINode = DevFsGetRootINode,
			.readINode = DevFsReadINode,
			.findINode = DevFsFindINode,
	};

static IoFileOps fileOps =
	{
			.open = DevFsOpen,
			.close = DevFsClose,
			.read = DevFsRead,
			.write = DevFsWrite,
			.ioctl = DevFsIoCtl,
	};

static IoFileOps rootDirOps =
	{
			.readdir = DevFsReadDir,
	};

//Registers the devfs filesystem
void IoDevFsInit()
{
	//Register our filesystem
	if(!IoFilesystemRegister(&fsType))
	{
		Panic("IoDevFsInit: failed to register devfs ?!");
	}
}

//Register a device with devfs using the name given in the device
// You cannot register a device with the same name as another device
int IoDevFsRegister(IoDevice * device)
{
	//INODES HERE ARE 1 PLUS THE INODES USED BY THE VFS

	//Find free iNode
	int freeINode = nextFreeINode;
	for(; freeINode < MAX_DEVICES; freeINode++)
	{
		if(devices[freeINode] == NULL)
		{
			//Use this one
			break;
		}
	}

	//Any left?
	if(freeINode == MAX_DEVICES)
	{
		return -ENOSPC;
	}

	//Attempt to add to hash table
	if(HashTableStringInsert(&files, &device->devFsHItem))
	{
		//Add to devices
		devices[freeINode] = device;
		device->devFsINode = freeINode;
		nextFreeINode = freeINode + 1;
		return 0;
	}
	else
	{
		return -EEXIST;
	}
}

//Unregisters an existing device with devfs
int IoDevFsUnRegister(IoDevice * device)
{
	//INODES HERE ARE 1 PLUS THE INODES USED BY THE VFS

	//Verify stored inode is correct
	if(device->devFsINode >= MAX_DEVICES || devices[device->devFsINode] != device)
	{
		return -ENOENT;
	}

	//Remove from hashtable and devices
	HashTableStringRemove(&files, device->name);
	devices[device->devFsINode] = NULL;

	//Make nextFreeINode lower if nessesary
	if(device->devFsINode < nextFreeINode)
	{
		nextFreeINode = device->devFsINode;
	}

	device->devFsINode = 0;
	return 0;
}

//Internal DevFs implementation
static bool DevFsGetKey(HashItem * item, const char ** str, int * len)
{
	IGNORE_PARAM len;

	*str = HashTableEntry(item, IoDevice, devFsHItem)->name;
	return false;
}

static int DevFsMount(IoFilesystem * newFs)
{
	//Set ops and return
	newFs->ops = &fsOps;
	return 0;
}

static unsigned int DevFsGetRootINode(IoFilesystem * fs)
{
	IGNORE_PARAM fs;
	return 0;
}

static int DevFsReadINode(IoFilesystem * fs, IoINode * iNode)
{
	IGNORE_PARAM fs;

	//Validate inode number
	if(iNode->number > MAX_DEVICES)
	{
		return -EIO;
	}

	//Handle root dir
	if(iNode->number == 0)
	{
		iNode->ops = &rootDirOps;
		iNode->mode = IO_OWNER_READ | IO_OWNER_EXEC | IO_GROUP_READ | IO_GROUP_EXEC |
					  IO_WORLD_READ | IO_WORLD_EXEC | IO_DIR;
		iNode->uid = 0;
		iNode->gid = 0;
	}
	else
	{
		//Get device
		GET_DEVICE(device, iNode->number);
		iNode->ops = &fileOps;
		iNode->mode = device->mode;
		iNode->uid = device->uid;
		iNode->gid = device->gid;
	}

	iNode->size = 0;
	return 0;
}

static int DevFsFindINode(IoFilesystem * fs, unsigned int parent,
		const char * name, int nameLen, unsigned int * iNodeNum)
{
	//Parent must be root inode
	if(parent == 0)
	{
		//Lookup device
		HashItem * item = HashTableStringFindLen(&files, name, nameLen);
		if(item == NULL)
		{
			return -ENOENT;
		}

		//Return device iNode
		return HashTableEntry(item, IoDevice, devFsHItem)->devFsINode;
	}
	else
	{
		//No children of non-parents
		return -ENOENT;
	}
}

static int DevFsOpen(IoINode * iNode, IoFile * file)
{
	//Forward to device
	GET_DEVICE(device, iNode->number);
	return device->devOps->open(device);
}

static int DevFsClose(IoFile * file)
{
	unsigned int iNode = file->iNode;

	//We don't return an error if the device is not found here
	if(iNode < MAX_DEVICES && devices[iNode] != NULL)
	{
		//Call close on device
		devices[iNode]->devOps->close(devices[iNode]);
	}

	return 0;
}

static int DevFsRead(IoFile * file, void * buffer, unsigned int count)
{
	//Get device
	GET_DEVICE(device, file->iNode);

	//If there is a block cache, use it
	int res;
	if(device->blockCache != NULL && IO_ISBLOCK(device->mode))
	{
		//Go via block cache
		res = IoBlockCacheReadBuffer(device, file->off, buffer, count);
	}
	else
	{
		//Go directly to device
		res = device->devOps->read(device, file->off, buffer, count);
	}

	//Adjust offset
#warning Move this to iocontext.c ?
	if(res > 0)
	{
		//Advance offset
		file->off += res;
		res = 0;
	}

	return res;
}

static int DevFsWrite(IoFile * file, void * buffer, unsigned int count)
{
	//Get device
	GET_DEVICE(device, file->iNode);

	//If there is a block cache, use it
	int res;
	if(device->blockCache != NULL && IO_ISBLOCK(device->mode))
	{
		//Go via block cache
		res = IoBlockCacheWrite(device, file->off, buffer, count);
	}
	else
	{
		//Go directly to device
		res = device->devOps->write(device, file->off, buffer, count);
	}

	//Adjust offset
#warning Move this to iocontext.c ?
	if(res > 0)
	{
		//Advance offset
		file->off += res;
		res = 0;
	}

	return res;
}

static int DevFsIoCtl(IoFile * file, int request, void * data)
{
	//Forward to device
	GET_DEVICE(device, file->iNode);
	return device->devOps->ioctl(device, request, data);
}

static int DevFsReadDir(IoFile * file, void * buf, IoDirectoryFiller filler, int count)
{
	//Only in top-level
	if(file->iNode == 0)
	{
		//Go through each iNode in the list
		int readSoFar = 0;
		int toSkip = file->off;

		for(int i = 0; i < MAX_DEVICES; i++)
		{
			//Avaliable and within offset
			if(devices[i] != NULL)
			{
				//Test if we need to skip
				if(toSkip == 0)
				{
					//Send back this file
					filler(buf, i - 1, devices[i]->name, StrLen(devices[i]->name));
					readSoFar++;

					//If we've read enough, exit
					if(readSoFar >= count)
					{
						break;
					}
				}
				else
				{
					toSkip--;
				}
			}
		}

		//Advance offset and return counter
		file->off += readSoFar;
		return readSoFar;
	}
	else
	{
		return -ENOTDIR;
	}
}
