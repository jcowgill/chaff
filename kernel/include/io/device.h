/*
 * device.h
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
 *      Author: James
 */

#ifndef IO_DEVICE_H_
#define IO_DEVICE_H_

#include "chaff.h"
#include "htable.h"
#include "io/mode.h"

//Devices
// There are 2 types of devices: character and block devices
// Block devices are always cached and are mountable (eg hard disk)
// Character devices are not cached, not mountable and are read 1 character at a time (eg /dev/zero)
//

struct IoDevice;
struct IoBlockCache;

//Device ops
// These must be implemented by devices
// For callers, you should not call read and write directly.
//  instead call them via the block cache
typedef struct
{
	//Called when the device is opened
	int (* open)(struct IoDevice * device);

	//Called when a device is closed
	void (* close)(struct IoDevice * device);

	//Attempts to read count bytes into the given buffer at position off
	// Character devices are allowed to ignore off
	// For block devices, cache reads will always use the same count as the block size
	// Returns actual number of bytes or a negative number on error
	int (* read)(struct IoDevice * device, unsigned long long off, void * buffer, unsigned int count);

	//Attempts to write count bytes from the given buffer at position off
	// Character devices are allowed to ignore off
	// Returns actual number of bytes written or a negative number on error
	int (* write)(struct IoDevice * device, unsigned long long off, void * buffer, unsigned int count);

	//Performs device-dependent request
	// All parameters and return code
	int (* ioctl)(struct IoDevice * device, int request, void * data);

} IoDeviceOps;

//A device which can live in devfs
// DevFs directories also use the IoDevice structure
typedef struct IoDevice
{
	//Filename of the device
	char * name;

	//The mode of the device
	// This handles permissions etc and soem device options
	// You should not change the type parts of the mode after it is created
	// For block devices:
	//  All read and writes are cached by the block cache
	//  The device is mountable (if there is an avaliable filesystem)
	// For character devices:
	//  Read and writes are forwarded immediately
	//  The device is not mountable
	IoMode mode;

	//IDs used with the mode
	unsigned int uid;
	unsigned int gid;

	//DevFs relationships
	HashItem devFsHItem;
	unsigned int devFsINode;

	//(block device only) Pointer to the block cache for this device
	struct IoBlockCache * blockCache;

	//Mounted?
	bool mounted;

	//Device functions
	IoDeviceOps * devOps;

	//Custom data about a device
	void * custom;

} IoDevice;

//DevFs functions
//

//Registers the devfs filesystem
void IoDevFsInit();

//Register a device with devfs using the name given in the device
// You cannot register a device with the same name as another device
int IoDevFsRegister(IoDevice * device);

//Unregisters an existing device with devfs
int IoDevFsUnRegister(IoDevice * device);

#endif /* IO_DEVICE_H_ */
