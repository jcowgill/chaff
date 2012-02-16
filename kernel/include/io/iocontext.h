/**
 * @file
 * Io context and file structures
 *
 * This is the "front-end" of the IO system.
 *
 * @date January 2012
 * @author James Cowgill
 * @ingroup Io
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

#ifndef IO_CONTEXT_H_
#define IO_CONTEXT_H_

#include "chaff.h"
#include "io/mode.h"
#include "io/fs.h"
#include "secContext.h"

/**
 * Maximum number of open files per IO context
 */
#define IO_MAX_OPEN_FILES 1024

/**
 * Maximum length of an element in a path
 */
#define IO_NAME_MAX 255

/**
 * A file description opened by an IoContext
 *
 * There is a subtle difference between @a descriptor and @a description
 * - Descriptions are the actual opened files (IoFile)
 * - Descriptors are references to descriptions which can reside in many
 *   different contexts but all point to the same file
 *
 * Eg:
 *  dup duplicates the descriptor but not the description
 */
typedef struct IoFile
{
	/**
	 * Reference count for this file
	 *
	 * This can be higher if more than one context owns this file
	 */
	unsigned int refCount;

	/**
	 * Offset of file pointer within file
	 */
	unsigned int off;

	/**
	 * Flags the file was opened with
	 *
	 * See IO_O_ defines
	 */
	int flags;

	/**
	 * Filesystem this file is on
	 */
	struct IoFilesystem * fs;

	/**
	 * iNode of this file
	 */
	unsigned int iNode;

	/**
	 * Filesystem data associated with the file
	 */
	void * fsData;

	/**
	 * Operations applied to the file
	 */
	IoFileOps * ops;

} IoFile;

/**
 * The context which IO operations are run in
 */
typedef struct IoContext
{
	/**
	 * Array of open files
	 */
	IoFile * files[IO_MAX_OPEN_FILES];

	/**
	 * Extra flags associated with a descriptor (not part of the file description itself)
	 */
	char descriptorFlags[IO_MAX_OPEN_FILES];

	/**
	 * ID of the next free space in the files array
	 */
	int nextFreeFile;

	/**
	 * Filesystem of the current directory
	 */
	IoFilesystem * cdirFs;

	/**
	 * iNode of the current directory
	 */
	unsigned int cdirINode;

	/**
	 * Number of references to this IO context
	 */
	unsigned int refCount;

} IoContext;

/**
 * @name IoDup() flags
 * @{
 */

/**
 * Use the next file descriptor available
 *
 * (rather than forcing the use of the descriptor provided)
 */
#define IO_DUP_AT_LEAST		1

/**
 * Ignore request (instead of error) if old == new
 */
#define IO_DUP_IGNORE_SAME	2

/**
 * @}
 * @name IoOpen() flags
 * @{
 */
#define IO_O_RDONLY		0x01		///< Open for reading
#define IO_O_WRONLY		0x02		///< Open for writing
#define IO_O_RDWR 		(IO_O_RDONLY | IO_O_WRONLY)		///< Open for reading and writing
#define IO_O_CREAT		0x04		///< Create file if it doesn't exist
#define IO_O_TRUNC		0x08		///< Truncate file to 0 length if it exists
#define IO_O_APPEND		0x10		///< Open for appending
#define IO_O_EXCL		0x20		///< File must not already exist
#define IO_O_CLOEXEC	0x40		///< Close file descriptor on exec
#define IO_O_DIRECTORY	0x80		///< File must be a directory

/**
 * @}
 */

#define IO_O_ALLFLAGS	0xFF		///< All flags (used for masking)

#define IO_O_FDRESERVED	0x01		///< File descriptor slot is reserved @private

/**
 * Creates a new empty IO context using the root directory as the current directory
 *
 * @return the created context
 */
IoContext * IoContextCreate();

/**
 * Creates a copy of an IO context including all open files
 *
 * The files are duplicates (like in IoDup()) so they refer to the same file description.
 *
 * @param context the context to clone
 * @return the cloned context
 */
IoContext * IoContextClone(IoContext * context);

/**
 * Adds a reference to an IO context
 *
 * @param context context to add reference to
 */
static inline void IoContextAddReference(IoContext * context)
{
	context->refCount++;
}

/**
 * Deletes a reference to an IO context
 *
 * This must NOT be used when the io context could be in use by another thread
 * (io contexts are not locked).
 *
 * This will close all open files
 *
 * @param context context to delete reference from
 */
void IoContextDeleteReference(IoContext * context);

/**
 * Gets a file object from the current IO context and a descriptor id
 *
 * @param fd file descriptor to search for
 * @return the file object or NULL if no file exists
 */
IoFile * IoGetFile(int fd);

/**
 * Gets a file object from the given IO context and a descriptor id
 *
 * @param context context to get file from
 * @param fd file descriptor to search for
 * @return the file object or NULL if no file exists
 */
IoFile * IoGetFileWithContext(IoContext * context, int fd);

/**
 * Finds the next available file descriptor at least as large as @a fd
 *
 * The descriptor may no longer be free after blocking.
 *
 * @param context the context to search in
 * @param fd file descriptor to start the search at
 * @retval -1 no available descriptors
 * @retval other the free file descriptor
 */
int IoFindNextDescriptor(IoContext * context, int fd);

/**
 * Looks up a path in the filesystem
 *
 * This function may block
 *
 * @param[in] secContext security context to use for the lookup (for traverse permission checks)
 * @param[in] ioContext io context to use for the lookup
 * @param[in] path path to search for (null terminated)
 * @param[out] output the found iNode is placed here
 * @param[out] fileStart if the file is not found but the path was, the start of the filename is placed here
 * @retval 0 successfully found file - iNode is placed in @a output
 * @retval -ENOENT file not found
 *  - If the parent directory (of the requested file) was found:
 *    - @a output is set to the parent directory
 *    - @a fileStart is set to the beginning of the unknown filename
 *  - If not:
 *    - @a fileStart is set to NULL
 * @retval -EISDIR path is a directory which was placed in @a output
 * @retval other another error from the filesystem
 */
int IoLookupPath(SecContext * secContext, IoContext * ioContext, const char * path,
		IoINode * output, const char ** fileStart);

/**
 * Opens a file in an IO context
 *
 * @param secContext security context to use for the lookup (for traverse permission checks)
 * @param ioContext io context to use for the lookup
 * @param path path to open (null terminated)
 * @param flags flags to open file with (see IO_O_ flags)
 * @param mode mode of new file if #IO_O_CREAT is specified (ignored otherwise)
 *  only the permission bits of the mode are used
 * @param fd file descriptor to open as
 * @retval 0 on success
 * @retval <0 error code
 */
int IoOpen(SecContext * secContext, IoContext * ioContext, const char * path,
		int flags, IoMode mode, int fd);

/**
 * Closes the file with the given descriptor
 *
 * @param context context to use
 * @param fd descriptor to close
 * @retval 0 on success
 * @retval <0 error code
 */
int IoClose(IoContext * context, int fd);

/**
 * Closes all the files in a context marked as #IO_O_CLOEXEC
 *
 * Any errors are discarded
 *
 * @param context context to use
 */
void IoCloseForExec(IoContext * context);

/**
 * Reads some bytes from a file descriptor
 *
 * @param context context to get file from
 * @param fd file descriptor
 * @param buffer buffer to read into (may be user mode)
 * @param count number of bytes to read
 * @retval >=0 the number of bytes actually read
 * @retval <0 error code
 */
int IoRead(IoContext * context, int fd, void * buffer, int count);

/**
 * Writes some bytes to a file descriptor
 *
 * @param context context to get file from
 * @param fd file descriptor
 * @param buffer buffer to write into (may be user mode)
 * @param count number of bytes to write
 * @retval >=0 the number of bytes actually written
 * @retval <0 error code
 */
int IoWrite(IoContext * context, int fd, void * buffer, int count);

/**
 * Sends an ioctl request to the device associated with a descriptor
 *
 * @param context context to use
 * @param fd file descriptor
 * @param request request to send to device
 * @param data optional data
 * @return anything the device wants
 */
int IoIoctl(IoContext * context, int fd, int request, void * data);

//Truncates a file to a precise length
// If the file gets larger, it is filled with nulls in the extra bits
// The file offset is unchanged
int IoTruncate(IoContext * context, int fd, unsigned long long size);

/**
 * Duplicates a file descriptor
 *
 * @param context context to use
 * @param fd descriptor to duplicate
 * @param newFd new descriptor (affected by flags)
 * @param flags flags to use for the duplication (see IO_DUP_ flags, and #IO_O_CLOEXEC)
 * @retval 0 on success
 * @retval -EMFILE #IO_DUP_AT_LEAST specified and there are no free slots
 * @retval -EINVAL attempted to duplicate into self and #IO_DUP_IGNORE_SAME not specified
 * @retval -EBADF invalid file descriptor (@a fd or @a newFd)
 * @retval -EBUSY file descriptor in use (and cannot be replaced)
 * @retval <0 other error as a result of closing the file
 */
int IoDup(IoContext * context, int fd, int newFd, int flags);

/**
 * Structure used as output in IoReadDir() requests
 */
typedef struct
{
	///INode of this file
	unsigned int iNode;

	///Name of this file (null terminated)
	char name[IO_NAME_MAX];

} IoReadDirEntry;

/**
 * Read the files from a directory
 *
 * @param context context to use
 * @param fd descriptor of the directory to read from
 * @param buffer buffer to read information into (may be user mode)
 * @param count number of entries to read
 * @retval >=0 the number of directories actually read
 * @retval <0 error code
 */
int IoReadDir(IoContext * context, int fd, IoReadDirEntry * buffer, int count);

#endif /* IO_CONTEXT_H_ */
