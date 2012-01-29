/*
 * fs.c
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
#include "io/fs.h"
#include "io/device.h"
#include "list.h"
#include "htable.h"
#include "errno.h"

//Filesystem list
static ListHead fsTypeHead = LIST_INLINE_INIT(fsTypeHead);

//Root Fs
IoFilesystem * IoFilesystemRoot;

//Registers a filesystem type with the kernel
bool IoFilesystemRegister(IoFilesystemType * type)
{
	//Check that there isn't already a filesystem
	if(IoFilesystemFind(type->name) == NULL)
	{
		//Add filesystem
		ListHeadInit(&fsTypeHead);
		ListHeadAddLast(&fsTypeHead, &type->fsTypes);
		return true;
	}
	else
	{
		return false;
	}
}

//Unregisters a filesystem type with the kernel
// You cannot unregister a filesystem in use
bool IoFilesystemUnRegister(IoFilesystemType * type)
{
	//Check if in use
	if(type->refCount != 0)
	{
		return false;
	}

	//Remove from list
	ListDelete(&type->fsTypes);
	return true;
}

//Finds a registered filesystem
IoFilesystemType * IoFilesystemFind(const char * name)
{
	IoFilesystemType * type;

	//Find existing filesystem
	ListForEachEntry(type, &fsTypeHead, fsTypes)
	{
		//Same name?
		if(StrCmp(type->name, name) == 0)
		{
			//This one
			return type;
		}
	}

	//None found
	return NULL;
}

//Internal fs mounter
// *fs is only accessed when sucessful
static int IoFilesystemMountInternal(IoFilesystemType * type, IoDevice * device,
		int flags, IoFilesystem ** fs)
{
	if(device)
	{
		//Device mounted already?
		if(device->mounted)
		{
			return -EBUSY;
		}

		//Lock device
		device->mounted = true;
	}

	//Lock type
	type->refCount++;

	//Allocate fs
	IoFilesystem * newFs = MAlloc(sizeof(IoFilesystem));
	MemSet(newFs, 0, sizeof(IoFilesystem));

	//Setup filesystem information
	newFs->fsType = type;
	newFs->device = device;
	newFs->flags = flags;

	//Mount fs
	int res = type->mount(newFs);
	if(res != 0)
	{
		//Free fs and unlock stuff
		MFree(newFs);
		type->refCount--;
		device->mounted = false;
	}
	else
	{
		*fs = newFs;
	}

	return res;
}

//Mounts a new filesystem
int IoFilesystemMount(IoFilesystemType * type, struct IoDevice * device,
		IoINode * onto, int flags)
{
	//Test validity of iNode
	if(!IO_ISDIR(onto->mode))
	{
		return -ENOTDIR;
	}

	//Extract iNode properties
	IoFilesystem * parent = onto->fs;
	unsigned int parentINode = onto->number;

	//Check if mount point is already mounted
	if(HashTableFind(&parent->mountPoints, parentINode) != NULL)
	{
		return -EBUSY;
	}

	//Lock parent
	parent->refCount++;

	//Do mounting
	IoFilesystem * newFs;
	int res = IoFilesystemMountInternal(type, device, flags, &newFs);

	//Insert mount point
	if(res == 0 && HashTableInsert(&parent->mountPoints, &newFs->mountItem))
	{
		//Mounted
		return 0;
	}

	//Fs Unmount
	if(!IoFilesystemUnMount(newFs))
	{
		PrintLog(Critical, "IoFilesystemMount: could not unmount newly created filesystem");
	}

	return res;
}

//Mounts a new root filesystem
int IoFilesystemMountRoot(IoFilesystemType * type, struct IoDevice * device, int flags)
{
	//Cannot mount over existing filesystem
	if(IoFilesystemRoot != NULL)
	{
		return -EBUSY;
	}

	//Do mounting
	IoFilesystem * newFs;
	int res = IoFilesystemMountInternal(type, device, flags, &newFs);

	//Insert mount point
	if(res == 0 && IoFilesystemRoot == NULL)
	{
		//Mounted
		IoFilesystemRoot = newFs;
		return 0;
	}

	//Fs Unmount
	if(!IoFilesystemUnMount(newFs))
	{
		PrintLog(Critical, "IoFilesystemMountRoot: could not unmount newly created filesystem");
	}

	return res;
}

//UnMounts a filesystem
int IoFilesystemUnMount(IoFilesystem * fs)
{
	//Check if filesystem is in use
	if(fs->refCount > 0 || fs->mountPoints.count > 0)
	{
		return -EBUSY;
	}

	//Call unmount
	if(fs->ops->umount)
	{
		int res = fs->ops->umount(fs);
		if(res != 0)
		{
			return res;
		}
	}

	//Decrement parent refCount
	if(fs->parentFs == NULL)
	{
		//Remove rootFs
		if(fs == IoFilesystemRoot)
		{
			IoFilesystemRoot = NULL;
		}
		else
		{
			PrintLog(Error, "IoFilesystemUnMount: filesystem's parent is NULL but isn't the root fs");
		}
	}
	else
	{
		fs->parentFs->refCount--;
		HashTableRemoveItem(&fs->parentFs->mountPoints, &fs->mountItem);
	}

	//Unlock device and type
	fs->fsType->refCount--;
	fs->device->mounted = false;

	//Free filesystem
	MFree(fs);
	return 0;
}
