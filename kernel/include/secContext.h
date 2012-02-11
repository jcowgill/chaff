/**
 * @file
 * Process security context
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

#ifndef SECCONTEXT_H_
#define SECCONTEXT_H_

#include "chaff.h"

/**
 * A set of security options assigned to a process
 *
 * The type of ids are:
 * - Real = Launcher of the process
 * - Effective = Permissions currently being used
 * - Saved = Allows setuid files to change effective id and then go back
 */
typedef struct SecContext
{
			/**
	 * Real user id
	 */
	unsigned int ruid;

	/**
	 * Effective user id
	 */
	unsigned int euid;

	/**
	 * Saved user id
	 */
	unsigned int suid;

	/**
	 * Real group id
	 */
	unsigned int rgid;

	/**
	 * Effective group id
	 */
	unsigned int egid;

	/**
	 * Saved group id
	 */
	unsigned int sgid;

} SecContext;

/**
 * Tests if a security context has root permissions
 *
 * @param context security context to test
 * @return whether the security context has root permissions
 */
static inline bool SecContextIsRoot(SecContext * context)
{
	return context->euid == 0;
}

#endif /* SECCONTEXT_H_ */
