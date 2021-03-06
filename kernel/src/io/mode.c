/*
 * mode.c
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

#include "chaff.h"
#include "io/mode.h"
#include "io/fs.h"

//Determines if the given security context can read / write / execute the file with the given mode and ids
bool IoModeCanAccess(IoMode accessMode, IoMode mode, unsigned int uid, unsigned int gid,
		SecContext * secContext)
{
	//Test root
	if(secContext->euid == 0)
	{
		return true;
	}

	//Test user permissions
	if(secContext->euid == uid)
	{
		return mode & accessMode;
	}
	else if(secContext->egid == gid)
	{
		return mode & (accessMode << 3);
	}
	else
	{
		return mode & (accessMode << 6);
	}
}

//Determines if the given security context can read / write / execute the file with the given iNode
bool IoModeCanAccessINode(IoMode accessMode, IoINode * iNode, SecContext * secContext)
{
	return IoModeCanAccess(accessMode, iNode->mode, iNode->uid, iNode->gid, secContext);
}
