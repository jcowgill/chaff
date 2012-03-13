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
 * Information stored about a kernel module
 *
 * Modules must declare a variable called "ModuleInfo" with this type once in their module.
 * It contains information about loading the module required by the module loader.
 */
typedef struct LdrModule
{
	/**
	 * Module name
	 *
	 * Names must not contain commas
	 */
	const char * name;

	/**
	 * Module dependencies
	 *
	 * This is a list of other modules which must be loaded before this module can be loaded
	 * (and cannot be unloaded while this module is loaded).
	 *
	 * This is an array of strings containing the dependencies.
	 * The array itself can be NULL to indicate no dependencies.
	 * If an array is given, NULL inidcates the end of the list.
	 */
	const char ** deps;

	/**
	 * Function to call after the module has been loaded
	 *
	 * The arguments given is a NULL terminated string but has no particular format.
	 *
	 * @param args arguments given to module
	 * @retval 0 module loaded successfully
	 * @retval errorcode module failed to load. the module must put
	 * 	everything back the way it was before returning this.
	 */
	int (* init)(const char * args);

	/**
	 * Called when a module is unloaded to cleanup what it has done
	 *
	 * @retval 0 module successfully cleaned up
	 * @retval errorcode module cannot be unloaded
	 */
	int (* cleanup)();

	/**
	 * Number of modules dependent on this module (must be 0 to unload)
	 *
	 * Modules should ignore this field.
	 *
	 * @private
	 */
	unsigned int depRefCount;

	/**
	 * Head of the list of symbols owned by this module.
	 *
	 * Modules should ignore this field.
	 *
	 * @private
	 */
	ListHead symbols;

	/**
	 * Load address of the module.
	 *
	 * Modules should ignore this field.
	 *
	 * @private
	 */
	void * dataStart;

	/**
	 * Entry in the global list of modules
	 *
	 * Modules should ignore this field.
	 *
	 * @private
	 */
	ListHead modules;

} LdrModule;

/**
 * Loads a module into the kernel
 *
 * This does not block (unless init is run), so you can pass a user mode pointer to it.
 *
 * @param data pointer to raw data to load
 * @param len length of data given
 * @param runInit true if LdrModule::init should be run
 * @param args arguments for LdrModule::init
 * @retval 0 loaded successfully
 */
int LdrLoadModule(const void * data, unsigned int len, bool runInit, const char * args);

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
