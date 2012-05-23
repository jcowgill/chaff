/**
 * @file
 * Kernel Module Information
 *
 * @date March 2012
 * @author James Cowgill
 * @ingroup Loader
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

#ifndef MODULE_H
#define MODULE_H

#include "chaff.h"
#include "list.h"

/**
 * Maximum module size (16 MB)
 */
#define LDR_MAX_MODULE_SIZE (16 * 1024 * 1024)

/**
 * Maximum number of dependencies
 */
#define LDR_MAX_DEPENDENCIES 8

struct LdrModule;

/**
 * Function to call after the module has been loaded
 *
 * The arguments given is a NULL terminated string but has no particular format.
 *
 * @param module the module structure of your module
 * @param args arguments given to module
 * @retval 0 module loaded successfully
 * @retval errorcode module failed to load. the module must put
 * 	everything back the way it was before returning this.
 */
typedef int (* LdrModuleInitFunc)(struct LdrModule * module, const char * args);

/**
 * Called when a module is unloaded to cleanup what it has done
 *
 * @retval 0 module successfully cleaned up
 * @retval errorcode module cannot be unloaded
 */
typedef int (* LdrModuleCleanupFunc)();

/**
 * Persistent information stored about a kernel module
 */
typedef struct LdrModule
{
	/**
	 * Module name
	 */
	const char * name;

	/**
	 * Function to call when module is cleaned up
	 */
	LdrModuleCleanupFunc cleanup;

	/**
	 * List of dependencies (ends with NULL if less than 8 being used)
	 */
	struct LdrModule * deps[8];

	/**
	 * Number of modules dependent on this module (must be 0 to unload)
	 */
	unsigned int depRefCount;

	/**
	 * Head of the list of symbols owned by this module.
	 */
	ListHead symbols;

	/**
	 * Load address of the module.
	 */
	void * dataStart;

	/**
	 * Entry in the global list of modules
	 */
	ListHead modules;

} LdrModule;

/**
 * Loads a module into the kernel
 *
 * You may pass a user mode pointer to this function.
 *
 * @param data pointer to raw data to load
 * @param len length of data given
 * @param runInit true if LdrModule::init should be run
 * @param args arguments for LdrModule::init
 * @retval 0 loaded successfully
 * @retval -EINVAL invalid module data (actual error printed to log)
 */
int LdrLoadModule(const void * data, unsigned int len, const char * args);

/**
 * Adds a dependency from one module to another
 *
 * Creating a dependency prevents a module from being unloaded until all the modules that
 * use it are unloaded.
 *
 * The module loader will automatically add dependencies if a module uses a function
 * declared in another module. You only need to use this function for dependencies where
 * you don't use any functions in the module or for optional dependencies.
 *
 * @param from the module using the functionality
 * @param to the module being used
 * @retval 0 dependency added
 * @retval -ELOOP circular dependency detected
 * @retval -EEXIST the dependency already exists
 * @retval -ENOSPC too many dependencies (see #LDR_MAX_DEPENDENCIES)
 */
int LdrAddDependency(LdrModule * from, LdrModule * to);

/**
 * Looks up a module structure by name
 *
 * @param name name of module
 * @return the module structure or NULL if it doesn't exist
 */
LdrModule * LdrLookupModule(const char * name);

/**
 * Unloads the given module
 *
 * @param module module reference to unload
 * @retval 0 unloaded successfully
 * @retval -EBUSY module in use
 */
int LdrUnloadModule(LdrModule * module);

#endif
