/*
 * mode.h
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
 *      Author: james
 */

#ifndef IO_MODE_H_
#define IO_MODE_H_

#include "chaff.h"
#include "secContext.h"

//Unix file modes
// The modes here correspond (with different names) to the standard unix modes
// All numbers are in OCTAL
//

#define IO_OWNER_READ		0000400
#define IO_OWNER_WRITE		0000200
#define IO_OWNER_EXEC		0000100
#define IO_OWNER_ALL		0000700

#define IO_GROUP_READ		0000040
#define IO_GROUP_WRITE		0000020
#define IO_GROUP_EXEC		0000010
#define IO_GROUP_ALL		0000070

#define IO_WORLD_READ		0000004
#define IO_WORLD_WRITE		0000002
#define IO_WORLD_EXEC		0000001
#define IO_WORLD_ALL		0000007

#define IO_STICKY			0001000
#define IO_SET_GID			0002000
#define IO_SET_UID			0004000

#define IO_FIFO				0010000		//Unimplemented
#define IO_DEV_CHAR			0020000
#define IO_DIR				0040000
#define IO_DEV_BLOCK		0060000
#define IO_REGULAR			0100000
#define IO_SYMLINK			0120000		//Unimplemented
#define IO_SOCKET			0140000		//Unimplemented
#define IO_ALL_TYPES		0170000

//Helper type checkers
#define IO_ISCHAR(mode) 	(((mode) & IO_ALL_TYPES) == IO_DEV_CHAR)
#define IO_ISBLOCK(mode) 	(((mode) & IO_ALL_TYPES) == IO_DEV_BLOCK)
#define IO_ISDIR(mode) 		(((mode) & IO_ALL_TYPES) == IO_DIR)
#define IO_ISREGULAR(mode) 	(((mode) & IO_ALL_TYPES) == IO_REGULAR)
#define IO_ISFIFO(mode) 	(((mode) & IO_ALL_TYPES) == IO_FIFO)
#define IO_ISSYMLINK(mode) 	(((mode) & IO_ALL_TYPES) == IO_SYMLINK)
#define IO_ISSOCKET(mode) 	(((mode) & IO_ALL_TYPES) == IO_SOCKET)

//IO Mode type
typedef unsigned short IoMode;

struct IoINode;

//Determines if the given security context can read / write / execute
// the file with the iNode / given permissions
// Use the "WORLD" permissions in access mode
bool IoModeCanAccess(IoMode accessMode, IoMode mode, unsigned int uid, unsigned int gid,
		SecContext * secContext);
bool IoModeCanAccessINode(IoMode accessMode, struct IoINode * iNode, SecContext * secContext);

#endif /* IO_MODE_H_ */
