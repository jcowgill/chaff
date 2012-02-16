/**
 * @file
 * Unix error codes
 *
 * @date January 2011
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

#ifndef ERRNO_H_
#define ERRNO_H_

#define EPERM            1      ///< Operation not permitted
#define ENOENT           2      ///< No such file or directory
#define ESRCH            3      ///< No such process
#define EINTR            4      ///< Interrupted system call
#define EIO              5      ///< I/O error
#define ENXIO            6      ///< No such device or address
#define E2BIG            7      ///< Arg list too long
#define ENOEXEC          8      ///< Executable format error
#define EBADF            9      ///< Bad file number
#define ECHILD          10      ///< No child processes
#define EAGAIN          11      ///< Try again
#define ENOMEM          12      ///< Out of memory
#define EACCES          13      ///< Permission denied
#define EFAULT          14      ///< Bad address
#define ENOTBLK         15      ///< Block device required
#define EBUSY           16      ///< Device or resource busy
#define EEXIST          17      ///< File exists
#define EXDEV           18      ///< Cross-device link
#define ENODEV          19      ///< No such device
#define ENOTDIR         20      ///< Not a directory
#define EISDIR          21      ///< Is a directory
#define EINVAL          22      ///< Invalid argument
#define ENFILE          23      ///< File table overflow
#define EMFILE          24      ///< Too many open files
#define ENOTTY          25      ///< Not a typewriter
#define ETXTBSY         26      ///< Text file busy
#define EFBIG           27      ///< File too large
#define ENOSPC          28      ///< No space left on device
#define ESPIPE          29      ///< Illegal seek
#define EROFS           30      ///< Read-only file system
#define EMLINK          31      ///< Too many links
#define EPIPE           32      ///< Broken pipe
#define EDOM            33      ///< Math argument out of domain of function
#define ERANGE          34      ///< Math result not representable
#define EDEADLK         35      ///< Resource deadlock would occur
#define EDEADLOCK       EDEADLK	///< Resource deadlock would occur
#define ENAMETOOLONG    36      ///< File name too long
#define ENOLCK          37      ///< No record locks available
#define ENOSYS          38      ///< Function not implemented
#define ENOTEMPTY       39      ///< Directory not empty
#define ELOOP           40      ///< Too many symbolic links encountered
#define EWOULDBLOCK     EAGAIN  ///< Operation would block
#define ENOMSG          42      ///< No message of desired type
#define EIDRM           43      ///< Identifier removed

#endif /* ERRNO_H_ */
