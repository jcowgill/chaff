/**
 * @file
 * Miscellaneous internal memory functions
 *
 * @date February 2012
 * @author James Cowgill
 * @privatesection
 * @ingroup Mem
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

#ifndef MM_MISC_H_
#define MM_MISC_H_

#include "chaff.h"
#include "multiboot.h"
#include "interrupt.h"
#include "mm/region.h"

/**
 * Memory manager initialization
 *
 * @param bootInfo memory information from boot loader
 */
void INIT MemManagerInit(multiboot_info_t * bootInfo);

/**
 * Frees pages marked as INIT
 */
void INIT MemFreeInitPages();

/**
 * Page fault handler
 *
 * @param context interrupt context page faults occur in
 */
void PRIVATE MemPageFaultHandler(IntrContext * context);

#endif
