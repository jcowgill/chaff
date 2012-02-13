/**
 * @file
 * Filesystem and Filesystem Type functions
 *
 * @date January 2012
 * @author James Cowgill
 */

/*
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
 */

#ifndef IO_FS_H_
#define IO_FS_H_

#include "chaff.h"
#include "list.h"
#include "htable.h"
#include "io/mode.h"

struct IoINode;
struct IoFile;
struct IoDevice;
struct IoFilesystem;
struct IoFilesystemType;

/**
 * Callback function which should be called when a directory is being filled
 *
 * @param buf internal buffer
 * @param iNode iNode of the node in the directory
 * @param name name of the node
 * @param len length of @a name in bytes
 * @retval 0 on success
 * @retval <0 an error code - this should be returned to the caller of IoFileOps::readDir
 */
typedef int (* IoDirectoryFiller)(void * buf, unsigned int iNode, const char * name, int len);

/**
 * File operations - things that can be done to open files
 *
 * Any of the functions here may be omitted to use the default action.
 */
typedef struct IoFileOps
{
	/**
	 * Opens a file
	 *
	 * This is not called for file duplications / when forking. As such, file
	 * may be passed to another security context and used there.
	 *
	 * The default action if unimplemented is to do nothing.
	 *
	 * @param iNode iNode being opened
	 * @param file file being opened into
	 * @retval 0 on success
	 * @retval <0 error code
	 */
	int (* open)(struct IoINode * iNode, struct IoFile * file);

	/**
	 * Closes a file
	 *
	 * This is called when the last reference to a file is closed. Closing
	 * files should not incur a permissions check since the final close can
	 * be made from any security context which has used the file.
	 *
	 * The default action if unimplemented is to do nothing.
	 *
	 * @param file file being closed
	 * @retval 0 on success
	 * @retval <0 error code
	 */
	int (* close)(struct IoFile * file);

	/**
	 * Attempts to read some bytes from a file
	 *
	 * @a file should be examined to fetch the filesystem, iNode number and file offset
	 *
	 * The file offset is automatically advanced by the number of bytes returned afterwards.
	 *
	 * The buffer supplied may be from user mode so ensure you perform checks on
	 * it with MemCanWrite() or MemCommitForWrite()
	 *
	 * The default action if unimplemented is to return -ENOSYS (Function not implemented).
	 *
	 * @param file file to read data from
	 * @param buffer buffer to write to (may be user mode)
	 * @param count number of bytes to read
	 * @retval >=0 the number of bytes read into the buffer
	 * @retval <0 error code
	 */
	int (* read)(struct IoFile * file, void * buffer, unsigned int count);

	/**
	 * Attempts to write some bytes to a file
	 *
	 * @a file should be examined to fetch the filesystem, iNode number and file offset
	 *
	 * The file offset is automatically advanced by the number of bytes returned afterwards.
	 *
	 * The buffer supplied may be from user mode so ensure you perform checks on
	 * it with MemCanRead() or MemCommitForRead()
	 *
	 * The default action if unimplemented is to return -ENOSYS (Function not implemented).
	 *
	 * @param file file to read write to
	 * @param buffer buffer to read data from (may be user mode)
	 * @param count number of bytes to write
	 * @retval >=0 the number of bytes written into the buffer
	 * @retval <0 error code
	 */
	int (* write)(struct IoFile * file, void * buffer, unsigned int count);

	/**
	 * Truncates a file to the given length
	 *
	 * If the file would be made larger, it is filled with 0s
	 *
	 * The default action if unimplemented is to return -ENOSYS (Function not implemented).
	 *
	 * @param file file to resize
	 * @param size size the file should be set to
	 * @retval 0 on success
	 * @retval <0 error code
	 */
	int (* truncate)(struct IoFile * file, unsigned long long size);

	/**
	 * Performs a device-dependent request
	 *
	 * Implementers should return -ENOTTY if the file does not refer to a device.
	 *
	 * The default action if unimplemented is to return -ENOTTY (No such device).
	 *
	 * @param file file to perform request on
	 * @param request request to perform
	 * @param data optional data from user mode
	 */
	int (* ioctl)(struct IoFile * file, int request, void * data);

	/**
	 * Reads the list of files in a directory
	 *
	 * The directory should start being read with the nth item given in @c file->off
	 *
	 * The default action if unimplemented is to return -ENOSYS (Function not implemented).
	 *
	 * @param file the directory to search
	 * @param buf internal buffer which should be passed to @a filler
	 * @param filler callback function which should be called for each file found
	 * @param count number of directories to return
	 * @retval 0 on success
	 * @retval <0 on error (pass back any errors generated by @a filler)
	 */
	int (* readdir)(struct IoFile * file, void * buf, IoDirectoryFiller filler, int count);

} IoFileOps;

/**
 * Represents a node in a filesystem
 *
 * Nodes can be files, directories, devices etc
 */
typedef struct IoINode
{
	/**
	 * Identification number (per filesystem) for this node
	 */
	unsigned int number;

	/**
	 * Filesystem this node belongs to
	 */
	struct IoFilesystem * fs;

	/**
	 * File operations used by this node
	 */
	IoFileOps * ops;

	/**
	 * Mode of the node (including what the node is)
	 */
	IoMode mode;

	/**
	 * User ID of node used for permissions
	 */
	unsigned int uid;

	/**
	 * Group ID of node used for permissions
	 */
	unsigned int gid;

	/**
	 * File size in bytes (0 for non files)
	 */
	unsigned long long size;

} IoINode;

/**
 * Filesystem operations
 *
 * Contains functions that cab be applied to the entire filesystem
 *
 * @a Some of the functions here can be omitted to use the default action
 */
typedef struct
{
	/**
	 * Unmounts the filesystem
	 *
	 * The default action if unimplemented is to do nothing.
	 *
	 * @param fs filesystem to unmount
	 * @retval 0 on success
	 * @retval <0 error code
	 */
	int (* umount)(struct IoFilesystem * fs);

	/**
	 * Gets the root iNode number
	 *
	 * This function MUST be implemented.
	 *
	 * @param fs filesystem to get number from
	 * @return the root iNode number
	 */
	unsigned int (* getRootINode)(struct IoFilesystem * fs);

	/**
	 * Reads information about an iNode into the iNode structure given
	 *
	 * @a iNode should be examined to fetch the iNode number and filesystem
	 *
	 * You must set IoINode::ops in this function.
	 *
	 * This function MUST be implemented.
	 *
	 * @param[in] fs filesystem to read node from
	 * @param[in,out] iNode iNode to read into
	 * @retval 0 on success
	 * @retval <0 error code
	 */
	int (* readINode)(struct IoFilesystem * fs, IoINode * iNode);

	/**
	 * Searches a directory to find a node in it
	 *
	 * The name parameter must be re-checked using MemCanRead()
	 * or MemCommitForRead() if used again after blocking.
	 *
	 * This function MUST be implemented.
	 *
	 * @param[in] fs filesystem to search
	 * @param[in] parent parent iNode number
	 * @param[in] name name of file to find (not null-terminated, may be user mode)
	 * @param[in] nameLen length of @a name
	 * @param[out] iNodeNum place the iNode number of the file found here
	 * @retval 0 on success
	 * @retval -ENOENT file not found
	 * @retval <0 any other error code
	 */
	int (* findINode)(struct IoFilesystem * fs, unsigned int parent,
			const char * name, int nameLen, unsigned int * iNodeNum);

	/**
	 * Creates a new empty file with the given name and mode
	 *
	 * The name parameter must be re-checked using MemCanRead()
	 * or MemCommitForRead() if used again after blocking.
	 *
	 * The default action if unimplemented is to return -ENOSYS (Function not implemented).
	 *
	 * @param[in] fs filesystem to create in
	 * @param[in] parent parent iNode number
	 * @param[in] name name of file to find (not null-terminated, may be user mode)
	 * @param[in] nameLen length of @a name
	 * @param[in] mode mode of new file
	 * @param[out] iNodeNum new iNode number
	 * @retval 0 on success
	 * @retval <0 error code
	 */
	int (* create)(struct IoFilesystem * fs, IoINode * parent,
			const char * name, int nameLen, IoMode mode, unsigned int * iNodeNum);

} IoFilesystemOps;

/**
 * Represents an individual filesystem mounted somewhere on the system
 */
typedef struct IoFilesystem
{
	/**
	 * Reference to the filesystem type
	 */
	struct IoFilesystemType * fsType;

	/**
	 * Device associated with this filesystem
	 *
	 * This can be NULL for special filesystems
	 */
	struct IoDevice * device;

	/**
	 * Operations associated with this filesystem
	 */
	IoFilesystemOps * ops;

	/**
	 * Number of reference to this filesystem (files, mount points)
	 */
	unsigned int refCount;

	/**
	 * Filesystem flags
	 */
	int flags;

	/**
	 * Filesystem custom data
	 */
	void * fsData;

	/**
	 * Parent filesystem (fs that this is mounted onto)
	 */
	struct IoFilesystem * parentFs;

	/**
	 * INode in parent filesystem this is mounted onto
	 */
	unsigned int parentINode;

	/**
	 * Table of filesystems mounted onto this filesystem
	 *
	 * These are | iNode -> IoFilesystem * | mappings
	 */
	HashTable mountPoints;

	/**
	 * Item in parent's mount table
	 */
	HashItem mountItem;

} IoFilesystem;

/**
 * Represents a type of supported filesystem
 */
typedef struct IoFilesystemType
{
	/**
	 * Name of this filesystem type
	 */
	char * name;

	/**
	 * Position in the list of filesystem types
	 */
	ListHead fsTypes;

	/**
	 * References to this filesystem type
	 */
	unsigned int refCount;

	/**
	 * Mounts a new filesystem
	 *
	 * @a newFs should be examined to fetch the type, device and flags set
	 *
	 * @param[in,out] newFs new filesystem options
	 * @retval 0 on success
	 * @retval <0 error code
	 */
	int (* mount)(IoFilesystem * newFs);

} IoFilesystemType;

/**
 * Mounts a filesystem in read-only mode
 *
 * This is handled by the Io functions and filesystem
 * implementors don't have to worry about it.
 */
#define IO_MOUNT_RDONLY 1

/**
 * Registers a type of filesystem with the kernel
 *
 * @param type type to register
 * @retval true successfully registered
 * @retval false a filesystem with that name already exists
 */
bool IoFilesystemRegister(IoFilesystemType * type);

/**
 * Unregisters a type of filesystem with the kernel
 *
 * @param type type to unregister
 * @retval true successfully unregistered
 * @retval false a filesystem with that has been mounted
 */
bool IoFilesystemUnRegister(IoFilesystemType * type);

/**
 * Finds a registered filesystem type
 *
 * @param name name of filesystem type to find (null terminated)
 * @return the filesystem or NULL if it wasn't found
 */
IoFilesystemType * IoFilesystemFind(const char * name);

/**
 * Mounts a new filesystem
 *
 * @param type type of the filesystem
 * @param device device to mount onto
 * @param onto iNode to mount the filesystem onto
 * @param flags flags to mount with
 * @retval 0 on success
 * @retval -ENOTDIR @a onto must be a directory
 * @retval -EBUSY something mounted on @a onto already / device already mounted
 * @retval <0 any other error returned from filesystem
 */
int IoFilesystemMount(IoFilesystemType * type, struct IoDevice * device,
		IoINode * onto, int flags);

/**
 * Mounts a new root filesystem
 *
 * @param type type of the filesystem
 * @param device device to mount onto
 * @param flags flags to mount with
 * @retval 0 on success
 * @retval -EBUSY if root has already been mounted / device already mounted
 * @retval <0 any other error returned from filesystem
 */
int IoFilesystemMountRoot(IoFilesystemType * type, struct IoDevice * device, int flags);

/**
 * Unmounts a filesystem
 *
 * @param fs filesystem to unmount
 * @retval 0 on success
 * @retval -EBUSY filesystem in use
 * @retval <0 any other error returned from filesystem
 */
int IoFilesystemUnMount(IoFilesystem * fs);

/**
 * The root filesystem
 *
 * Do not modify
 */
extern IoFilesystem * IoFilesystemRoot;

#endif /* IO_FS_H_ */
