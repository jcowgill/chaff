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

//Error constants

/**
 * Operation not permitted
 */
#define EPERM            1

/**
 * No such file or directory
 */
#define ENOENT           2

/**
 * No such process
 */
#define ESRCH            3

/**
 * Interrupted system call
 */
#define EINTR            4

/**
 * I/O error
 */
#define EIO              5

/**
 * No such device or address
 */
#define ENXIO            6

/**
 * Arg list too long
 */
#define E2BIG            7

/**
 * Exec format error
 */
#define ENOEXEC          8

/**
 * Bad file number
 */
#define EBADF            9

/**
 * No child processes
 */
#define ECHILD          10

/**
 * Try again
 */
#define EAGAIN          11

/**
 * Out of memory
 */
#define ENOMEM          12

/**
 * Permission denied
 */
#define EACCES          13

/**
 * Bad address
 */
#define EFAULT          14

/**
 * Block device required
 */
#define ENOTBLK         15

/**
 * Device or resource busy
 */
#define EBUSY           16

/**
 * File exists
 */
#define EEXIST          17

/**
 * Cross-device link
 */
#define EXDEV           18

/**
 * No such device
 */
#define ENODEV          19

/**
 * Not a directory
 */
#define ENOTDIR         20

/**
 * Is a directory
 */
#define EISDIR          21

/**
 * Invalid argument
 */
#define EINVAL          22

/**
 * File table overflow
 */
#define ENFILE          23

/**
 * Too many open files
 */
#define EMFILE          24

/**
 * Not a typewriter
 */
#define ENOTTY          25

/**
 * Text file busy
 */
#define ETXTBSY         26

/**
 * File too large
 */
#define EFBIG           27

/**
 * No space left on device
 */
#define ENOSPC          28

/**
 * Illegal seek
 */
#define ESPIPE          29

/**
 * Read-only file system
 */
#define EROFS           30

/**
 * Too many links
 */
#define EMLINK          31

/**
 * Broken pipe
 */
#define EPIPE           32

/**
 * Math argument out of domain of func
 */
#define EDOM            33

/**
 * Math result not representable
 */
#define ERANGE          34

/**
 * Resource deadlock would occur
 */
#define EDEADLK         35

/**
 * Resource deadlock would occur
 */
#define EDEADLOCK       EDEADLK

/**
 * File name too long
 */
#define ENAMETOOLONG    36

/**
 * No record locks available
 */
#define ENOLCK          37

/**
 * Function not implemented
 */
#define ENOSYS          38

/**
 * Directory not empty
 */
#define ENOTEMPTY       39

/**
 * Too many symbolic links encountered
 */
#define ELOOP           40

/**
 * Operation would block
 */
#define EWOULDBLOCK     EAGAIN

/**
 * No message of desired type
 */
#define ENOMSG          42

/**
 * Identifier removed
 */
#define EIDRM           43

#endif /* ERRNO_H_ */
