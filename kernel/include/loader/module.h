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
	 * The list contains commas to separate modules or a blank string if there are no modules.
	 * Do not use spaces.
	 *
	 * @code
	 * "" - No dependencies
	 * "abc" - Dependent on module "abc"
	 * "abc,def" - Dependent on modules "abc" and "def"
	 * @endcode
	 */
	const char * deps;

	/**
	 * Function to call after the module has been loaded
	 *
	 * The arguments given is a NULL terminated string but has no particular format.
	 *
	 * @param args arguments given to module
	 * @retval true module loaded successfully
	 * @retval false module failed to load. the module must put
	 * 	everything back the way it was before returning this.
	 */
	bool (* init)(const char * args);

	/**
	 * Called when a module is unloaded to cleanup what it has done
	 *
	 * @retval true module successfully cleaned up
	 * @retval false module cannot be unloaded
	 */
	bool (* cleanup)();

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

} LdrModule;

#endif
