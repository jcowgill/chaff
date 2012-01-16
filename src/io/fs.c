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

//Registers a filesystem type with the kernel
int IoFilesystemRegister(IoFilesystemType * type)
{
	//
}

//Unregisters a filesystem type with the kernel
// You cannot unregister a filesystem in use
int IoFilesystemUnRegister(IoFilesystemType * type)
{
	//
}

//Finds a registered filesystem
IoFilesystemType * IoFilesystemFind(const char * name)
{
	//
}

//Mounts a new filesystem
int IoFilesystemMount(IoFilesystemType * type, struct IoDevice * device,
		struct IoFileSystem * onto, unsigned int ontoINode, int flags)
{
	//
}

//UnMounts a filesystem
int IoFilesystemUnMount(struct IoFileSystem * onto)
{
	//
}
