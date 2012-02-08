/*
 * mm/misc.h
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
 *  Created on: 8 Feb 2010
 *      Author: James
 */

#ifndef MM_MISC_H_
#define MM_MISC_H_

//Miscelaneous kernel initialization functions
// All these are INTERNAL and should not be used by drivers
//

#include "chaff.h"
#include "multiboot.h"
#include "interrupt.h"
#include "mm/region.h"

//Memory manager initialisation
void MemManagerInit(multiboot_info_t * bootInfo);

//Page fault handler
// This MUST be called from an interrupt context
void MemPageFaultHandler(IntrContext * context);

//Malloc Init
void MAllocInit();

//Create region from static memory
// This is used by malloc so it can create its own region (MemRegionCreate depends on malloc)
// See MemRegionCreate
// Returns true on success
bool MemRegionCreateStatic(MemContext * context, void * startAddress,
		unsigned int length, MemRegionFlags flags, MemRegion * newRegion);

#endif
