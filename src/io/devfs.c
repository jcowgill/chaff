/*
 * devfs.c
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
 *  Created on: 16 Jan 2012
 *      Author: james
 */

#include "chaff.h"
#include "io/device.h"
#include "io/fs.h"

//Registers the devfs filesystem
void IoDevFsInit()
{
	//
}

//Register a device with devfs using the name given in the device
// You cannot register a device with the same name as another device
int IoDevFsRegister(IoDevice * device)
{
	//
}

//Unregisters an existing device with devfs
int IoDevFsUnRegister(IoDevice * device)
{
	//
}
