/**
 * @file
 * Device functions
 *
 * There are two types of devices which behave in slighty different ways:
 * - Character Devices
 *   These devices do not use block caches, are not mountable and can be read 1 character at a time
 * - Block Devices
 *   These devices use block caches, are mountable and are read 1 block at a time (eg 512 bytes at once)
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

#ifndef IO_DEVICE_H_
#define IO_DEVICE_H_

#include "chaff.h"
#include "htable.h"
#include "io/mode.h"

struct IoDevice;
struct IoBlockCache;

/**
 * Device operations implemented by devices
 *
 * Usually, other devices should not call these directly.
 * Rather, they should use other Io / BlockCache functions.
 */
typedef struct
{
	/**
	 * Called when the device is opened as a file by IoOpen()
	 *
	 * This is not called for mounted devices.
	 *
	 * @param device device being opened
	 * @retval 0 successful open
	 * @retval <0 error code reported to user
	 */
	int (* open)(struct IoDevice * device);

	/**
	 * Called when the device is closed by IoClose()
	 *
	 * This is not called for mounted devices.
	 *
	 * @param device device being closed
	 */
	void (* close)(struct IoDevice * device);

	/**
	 * Attempts to read @a count bytes from the device into the given buffer
	 *
	 * @param device device to read from
	 * @param off offset within the device to read (may be ignored by character devices)
	 * @param buffer buffer to read into (may be user mode)
	 * @param count number of bytes to read
	 * @retval 0 on success
	 * @retval <0 error code - may be changed to -EIO in some cases
	 */
	int (* read)(struct IoDevice * device, unsigned long long off, void * buffer, unsigned int count);

	/**
	 * Attempts to write @a count bytes from the given buffer to the device
	 *
	 * @param device device to write to
	 * @param off offset within the device to write (may be ignored by character devices)
	 * @param buffer buffer to write into (may be user mode)
	 * @param count number of bytes to write
	 * @retval 0 on success
	 * @retval <0 error code - may be changed to -EIO in some cases
	 */
	int (* write)(struct IoDevice * device, unsigned long long off, void * buffer, unsigned int count);

	/**
	 * Performs a device-dependent request
	 *
	 * This is usually only implemented by character devices.
	 *
	 * @param device device to perform the function on
	 * @param request request to make (device defined)
	 * @param data optional data pointer (may be user mode)
	 * @return anything you want. by convention, 0 means success.
	 */
	int (* ioctl)(struct IoDevice * device, int request, void * data);

} IoDeviceOps;

/**
 * A device which can interface with devfs, the block cache and (possibly) can be mounted
 */
typedef struct IoDevice
{
	/**
	 * Filename of the device
	 */
	char * name;

	/**
	 * The mode of the device
	 *
	 * This handles permissions and some device options.
	 *
	 * Once the device has been registered with DevFs, you should not change the mode.
	 *
	 * You must set either #IO_DEV_CHAR or #IO_DEV_BLOCK to indicate what type of device this is.
	 */
	IoMode mode;

	/**
	 * User ID for permission checks
	 */
	unsigned int uid;

	/**
	 * Group ID for permission checks
	 */
	unsigned int gid;

	/**
	 * Hash item used by DevFs (initialize to 0)
	 */
	HashItem devFsHItem;

	/**
	 * INode number used by DevFs (initialize to 0)
	 */
	unsigned int devFsINode;

	/**
	 * Pointer to the block cache for this device
	 *
	 * This must be set for block devices
	 */
	struct IoBlockCache * blockCache;

	/**
	 * True if device is mounted
	 */
	bool mounted;

	/**
	 * Device operations (must be set)
	 */
	IoDeviceOps * devOps;

	/**
	 * Custom data about a device
	 */
	void * custom;

} IoDevice;

/**
 * Registers the devfs filesystem
 *
 * @private
 */
void IoDevFsInit();

/**
 * Registers a device with devfs
 *
 * The name specified in the device is used as the filename in devfs.
 * You cannot register a device with the same name as another device.
 *
 * @param device device to register
 * @retval 0 on success
 * @retval -ENOSPC no space left on devfs
 * @retval -EEXIST a device with this name already exists
 * @bug directories are not supported
 */
int IoDevFsRegister(IoDevice * device);

/**
 * Unregisters a device with devfs
 *
 * @param device device to unregister
 * @retval 0 on success
 * @retval -#ENOENT device is not registered
 */
int IoDevFsUnRegister(IoDevice * device);

#endif /* IO_DEVICE_H_ */
