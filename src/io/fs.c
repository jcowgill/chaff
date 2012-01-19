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
#include "list.h"

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

//Mounts a new filesystem
int IoFilesystemMount(IoFilesystemType * type, struct IoDevice * device,
		IoFilesystem * onto, unsigned int ontoINode, int flags)
{
	//
}

//UnMounts a filesystem
int IoFilesystemUnMount(IoFilesystem * onto)
{
	//
}
