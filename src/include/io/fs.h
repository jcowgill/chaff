/*
 * fs.h
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
 *  Created on: 13 Jan 2012
 *      Author: James
 */

#ifndef IO_FS_H_
#define IO_FS_H_

#include "chaff.h"
#include "list.h"
#include "htable.h"
#include "io/mode.h"

//This header contains all the filesystem related structures and functions
//

struct IoINode;
struct IoFile;
struct IoDevice;
struct IoFilesystem;

//File Ops
// Things that can be done to file nodes (these are all implemented by filesystems)
typedef struct
{
	//Opens this file into a new file structure
	// This is not called for file duplications
	int (* open)(struct IoINode * iNode, struct IoFile * file);

	//Closes the file
	// This is called when the last reference to a file is closed
	int (* close)(struct IoFile * file);

	//Attempts to read count bytes into the given buffer at position off
	// Character devices are allowed to ignore off
	// Returns actual number of bytes or a negative number on error
	int (* read)(struct IoFile * file, unsigned int off, void * buffer, unsigned int count);

	//Attempts to write count bytes from the given buffer at position off
	// Character devices are allowed to ignore off
	// Returns actual number of bytes written or a negative number on error
	int (* write)(struct IoFile * file, unsigned int off, void * buffer, unsigned int count);

	//Performs device-dependent request
	// All parameters and return code
	int (* ioctl)(struct IoFile * file, int request, void * data);

} IoFileOps;

//A representation of a node in the filesystem
typedef struct IoINode
{
	//Filesystem this iNode belongs to
	struct IoFilesystem * fs;

	//File ops with this iNode
	IoFileOps ops;

	//File mode (including type) and users
	IoMode mode;
	unsigned int uid;
	unsigned int gid;

	//Time of: access, create, modification
	int atime, ctime, mtime;

	//File size in bytes
	unsigned long long size;

} IoINode;

//Filesystem Ops
// Things that can be done to an entire filesystem
typedef struct
{
	//Unmounts the filesystem
	int (* umount)(struct IoFilesystem * fs);

	//Reads information about an inode into the iNode structure given
	// You must set IoFileOps in the iNode
	int (* readINode)(struct IoFilesystem * fs, unsigned int id, IoINode * iNode);

	//Reads the nth inode of a directory into iNodeNum
	// parent = directory to read
	// child = the nth child
	// iNodeNum = fill with the inode number
	int (* getNthINode)(struct IoFilesystem * fs, IoINode * parent, unsigned int child, unsigned int * iNodeNum);

	//Finds the inode of a file in a directory
	// parent = directory to read
	// name = string name of the file to find - NOT null terminated
	// nameLen = length of name
	// iNodeNum = fill with the inode number
	// The name paremeter is invalidated after blocking
	int (* findINode)(struct IoFilesystem * fs, IoINode * parent,
			const char * name, int nameLen, unsigned int * iNodeNum);

} IoFilesystemOps;

typedef struct IoFilesystem
{
	//Type of filesystem
	struct IoFilesystemType * fsType;
	struct IoDevice * device;	//Can be NULL

	//Filesystem functions
	IoFilesystemOps ops;

	//Filesystem specific data
	int flags;
	void * fsData;

	//Parent mount point (used when accessing .. from root)
	struct IoFilesystem * parentFs;
	unsigned int parentINode;

	//Mount points mounted on this filesystem
	HashTable mountPoints;

} IoFilesystem;

//Represents a type of supported filssyetem
typedef struct
{
	//Name of this filesystem
	char * name;

	//Filesystem type list
	ListHead fsTypes;

	//Number of filesystems of this type
	unsigned int refCount;

	//Mounts a device using this filesystem
	// newFs contains everything already set
	int (* mount)(IoFilesystem ** newFs);

} IoFilesystemType;

//Filesystem functions
// Most filesystem access stuff is implemented with the io-context

//Registers a filesystem type with the kernel
int IoFilesystemRegister(IoFilesystemType * type);

//Unregisters a filesystem type with the kernel
// You cannot unregister a filesystem in use
int IoFilesystemUnRegister(IoFilesystemType * type);

//Finds a registered filesystem
IoFilesystemType * IoFilesystemFind(const char * name);

//Mounts a new filesystem
int IoFilesystemMount(IoFilesystemType * type, struct IoDevice * device,
		struct IoFileSystem * onto, unsigned int ontoINode, int flags);

//UnMounts a filesystem
int IoFilesystemUnMount(struct IoFileSystem * onto);

#endif /* IO_FS_H_ */
