/*
 * bootmodule.c
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
 *  Created on: 23 May 2012
 *      Author: James
 */

#include "chaff.h"
#include "loader/module.h"
#include "multiboot.h"

//Load boot modules
void INIT LdrLoadBootModules(struct multiboot_info * mHeader)
{
	//Process each module in the list
	if(mHeader->flags & MULTIBOOT_INFO_MODS)
	{
		MODULES_FOREACH(multibootMod, mHeader->mods_addr, mHeader->mods_count)
		{
			//Attempt to load module
			LdrModule * module = LdrLoadModule(
					multibootMod->mod_start + KERNEL_VIRTUAL_BASE,
					multibootMod->mod_end - multibootMod->mod_start,
					multibootMod->cmdline + KERNEL_VIRTUAL_BASE
				);

			//Success?
			if(!module)
			{
				Panic("LdrLoadBootModules: Some boot modules failed to load");
			}
		}
	}
}
