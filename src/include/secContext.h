/*
 * seccontext.h
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
 *  Created on: 21 Jan 2012
 *      Author: james
 */

#ifndef SECCONTEXT_H_
#define SECCONTEXT_H_

//This file contains the simple security context for each process
// This allows things to use a particular security context without the process
//

#include "chaff.h"

typedef struct
{
	//Real, effective and saved user and group identifiers
	// Real = launcher of the process
	// Effective = permissions currently using (eg for files)
	// Saved = allows setuid files to change effective id and go back again
	unsigned int ruid, euid, suid;
	unsigned int rgid, egid, sgid;

} SecContext;

//Tests if a security context has root permissions
static inline bool SecContextIsRoot(SecContext * context)
{
	return context->euid == 0;
}

#endif /* SECCONTEXT_H_ */
