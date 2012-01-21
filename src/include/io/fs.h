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

//Callback function when a directory name if found
typedef int (* IoDirectoryFiller)(void * buf, unsigned int iNode, const char * name, int len);

//File Ops
// Things that can be done to file nodes (these are all implemented by filesystems)
typedef struct
{
	//Opens this file into a new file structure
	// This is not called for file duplications
	int (* open)(struct IoINode * iNode, struct IoFile * file);

	//Closes the file
	// This is called when the last reference to a file is closed
	// If the result is not 0, the file is not completely closed
	int (* close)(struct IoFile * file);

	//Attempts to read count bytes into the given buffer
	// Character devices are allowed to ignore off
	// Returns actual number of bytes or a negative number on error
	int (* read)(struct IoFile * file, void * buffer, unsigned int count);

	//Attempts to write count bytes from the given buffer
	// Character devices are allowed to ignore off
	// Returns actual number of bytes written or a negative number on error
	int (* write)(struct IoFile * file, void * buffer, unsigned int count);

	//Truncates a file to the given length
	// This can also make a file larger
	int (* truncate)(struct IoFile * file, unsigned long long size);

	//Performs device-dependent request
	// All parameters and return code
	int (* ioctl)(struct IoFile * file, int request, void * data);

	//Reads files from the given directory (opened as a file)
	// Should start reading at the position (off) stored in the file
	// buf should be passed to filler and not interpreted
	int (* readdir)(struct IoFile * file, void * buf, IoDirectoryFiller filler, int count);

} IoFileOps;

//A representation of a node in the filesystem
typedef struct IoINode
{
	//iNode number
	unsigned int number;

	//Filesystem this iNode belongs to
	struct IoFilesystem * fs;

	//File ops with this iNode
	IoFileOps * ops;

	//File mode (including type) and users
	IoMode mode;
	unsigned int uid;
	unsigned int gid;

	//File size in bytes
	unsigned long long size;

} IoINode;

//Filesystem Ops
// Things that can be done to an entire filesystem
typedef struct
{
	//Unmounts the filesystem
	int (* umount)(struct IoFilesystem * fs);

	//Gets the root inode number
	// REQUIRED
	unsigned int (* getRootINode)(struct IoFilesystem * fs);

	//Reads information about an inode into the iNode structure given
	// REQUIRED
	// You must set IoFileOps in the iNode
	// The iNode number and filesystem will be set in iNode and the rest is undefined
	int (* readINode)(struct IoFilesystem * fs, IoINode * iNode);

	//Finds the inode of a file in a directory
	// REQUIRED
	// parent = directory to read
	// name = string name of the file to find - NOT null terminated
	// nameLen = length of name
	// iNodeNum = fill with the inode number
	// The name paremeter is invalidated after blocking
	int (* findINode)(struct IoFilesystem * fs, unsigned int parent,
			const char * name, int nameLen, unsigned int * iNodeNum);

	//Creates a new empty file with the given name and mode (see findINode for params)
	int (* create)(struct IoFilesystem * fs, IoINode * parent,
			const char * name, int nameLen, IoMode mode, unsigned int * iNodeNum);

} IoFilesystemOps;

typedef struct IoFilesystem
{
	//Type of filesystem
	struct IoFilesystemType * fsType;
	struct IoDevice * device;	//Can be NULL

	//Filesystem functions
	IoFilesystemOps * ops;

	//Filesystem specific data
	int flags;
	void * fsData;

	//Parent mount point (used when accessing .. from root)
	struct IoFilesystem * parentFs;
	unsigned int parentINode;

	//Mount points mounted on this filesystem
	// These are iNode -> IoFilesystem * mappings
	HashTable mountPoints;
	HashItem mountItem;		//Mount item for the fs this is mounted onto

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
	// newFs contains everything already set exept ops and fsData
	int (* mount)(IoFilesystem * newFs);

} IoFilesystemType;

//Filesystem functions
// Most filesystem access stuff is implemented with the io-context

//Filesystem mount flags (global)
#define IO_MOUNT_RDONLY 1

//Registers a filesystem type with the kernel
bool IoFilesystemRegister(IoFilesystemType * type);

//Unregisters a filesystem type with the kernel
// You cannot unregister a filesystem in use
bool IoFilesystemUnRegister(IoFilesystemType * type);

//Finds a registered filesystem
IoFilesystemType * IoFilesystemFind(const char * name);

//Mounts a new filesystem
int IoFilesystemMount(IoFilesystemType * type, struct IoDevice * device,
		IoFilesystem * onto, unsigned int ontoINode, int flags);

//UnMounts a filesystem
int IoFilesystemUnMount(IoFilesystem * onto);

//The root filesystem
extern IoFilesystem * IoFilesystemRoot;

#endif /* IO_FS_H_ */
