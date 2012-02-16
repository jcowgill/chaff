/**
 * @file
 * Unix file modes
 *
 * The modes here correspond to the standard unix modes (with different names)
 *
 * All numbers are in OCTAL
 *
 * @date January 2012
 * @author James Cowgill
 * @ingroup Io
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

#ifndef IO_MODE_H_
#define IO_MODE_H_

#include "chaff.h"
#include "secContext.h"

#define IO_OWNER_READ		0000400		///< Readable by file owner
#define IO_OWNER_WRITE		0000200		///< Writable by file owner
#define IO_OWNER_EXEC		0000100		///< Executable by file owner
#define IO_OWNER_ALL		0000700		///< Readable, writable and executable by file owner

#define IO_GROUP_READ		0000040		///< Readable by file's group
#define IO_GROUP_WRITE		0000020		///< Writable by file's group
#define IO_GROUP_EXEC		0000010		///< Executable by file's group
#define IO_GROUP_ALL		0000070		///< Readable, writable and executable by file's group

#define IO_WORLD_READ		0000004		///< Readable by everyone
#define IO_WORLD_WRITE		0000002		///< Writable by everyone
#define IO_WORLD_EXEC		0000001		///< Executable by everyone
#define IO_WORLD_ALL		0000007		///< Readable, writable and executable by everyone

/**
 * Sticky bit
 *
 * - On files, has no effect
 * - On directories, prevents deletion of files except by the owner or by root
 *
 * @todo unimplemented
 */
#define IO_STICKY			0001000

/**
 * Set group id
 *
 * - On files, sets group id on execution
 * - On directories, new files / directories are owned by the owner of the directory
 *   instead of being owned by the group of the user that created them.
 *
 * @todo unimplemented
 */
#define IO_SET_GID			0002000

/**
 * Set user id
 *
 * - On files, sets user id on execution
 * - On directories, has no effect.
 *
 * @todo unimplemented
 */
#define IO_SET_UID			0004000

#define IO_FIFO				0010000		///< First in, first out pipe @todo unimplemented
#define IO_DEV_CHAR			0020000		///< Character device
#define IO_DIR				0040000		///< Directory
#define IO_DEV_BLOCK		0060000		///< Block device
#define IO_REGULAR			0100000		///< Regular file
#define IO_SYMLINK			0120000		///< Symbolic link @todo unimplemented
#define IO_SOCKET			0140000		///< Unix socket @todo unimplemented
#define IO_ALL_TYPES		0170000		///< All types
										///<  This is not a valid type, but used to mask off types

///Tests whether the mode given is from a character device
#define IO_ISCHAR(mode) 	(((mode) & IO_ALL_TYPES) == IO_DEV_CHAR)

///Tests whether the mode given is from a block device
#define IO_ISBLOCK(mode) 	(((mode) & IO_ALL_TYPES) == IO_DEV_BLOCK)

///Tests whether the mode given is from a directory
#define IO_ISDIR(mode) 		(((mode) & IO_ALL_TYPES) == IO_DIR)

///Tests whether the mode given is from a regular file
#define IO_ISREGULAR(mode) 	(((mode) & IO_ALL_TYPES) == IO_REGULAR)

///Tests whether the mode given is from a fifo pipe
#define IO_ISFIFO(mode) 	(((mode) & IO_ALL_TYPES) == IO_FIFO)

///Tests whether the mode given is from a symbolic link
#define IO_ISSYMLINK(mode) 	(((mode) & IO_ALL_TYPES) == IO_SYMLINK)

///Tests whether the mode given is from a socket
#define IO_ISSOCKET(mode) 	(((mode) & IO_ALL_TYPES) == IO_SOCKET)

/**
 * Type used for storing modes
 */
typedef unsigned short IoMode;

struct IoINode;

/**
 * Determines if the given security context can access a file
 *
 * The access mode should use the "WORLD" permissions and
 * can have multiple permissions ORed together
 *
 * If you have an iNode, use IoModeCanAccessINode() instead.
 *
 * @param accessMode mode of access required
 * @param mode mode of the file to check
 * @param uid user id of the file to check
 * @param gid group id of the file to check
 * @param secContext security context accessing the file
 * @retval true the security context can access the file
 * @retval false the security context cannot access the file
 */
bool IoModeCanAccess(IoMode accessMode, IoMode mode, unsigned int uid, unsigned int gid,
		SecContext * secContext);

/**
 * Determines if the given security context can access a file
 *
 * The access mode should use the "WORLD" permissions and
 * can have multiple permissions ORed together.
 *
 * This is similar to IoModeCanAccess() but obtains the permissions from an iNode.
 *
 * @param accessMode mode of access required
 * @param iNode iNode being accessed
 * @param secContext security context accessing the file
 * @retval true the security context can access the file
 * @retval false the security context cannot access the file
 */
bool IoModeCanAccessINode(IoMode accessMode, struct IoINode * iNode, SecContext * secContext);

#endif /* IO_MODE_H_ */
