/*
 * fsdefault.h
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
 *  Created on: 19 Jan 2012
 *      Author: james
 */

#ifndef FSDEFAULT_H_
#define FSDEFAULT_H_

#include "chaff.h"
#include "io/fs.h"
#include "io/mode.h"

//Default fs implementations (filesystems have to set these though)
//

struct IoFile;

//Returns 0
int IoDefaultUMount(IoFilesystem * fs);

//Returns 1
unsigned int IoDefaultGetRootINode(IoFilesystem * fs);

//Returns -EPERM
int IoDefaultCreate(IoFilesystem * fs, IoINode * parent,
		const char * name, int nameLen, IoMode mode, unsigned int * iNodeNum);

//Returns 0
int IoDefaultOpen(IoINode * iNode, struct IoFile * file);

//Returns 0
int IoDefaultClose(struct IoFile * file);

//Returns 0 (EOF)
int IoDefaultRead(struct IoFile * file, void * buffer, unsigned int count);

//Returns -EPERM
int IoDefaultWrite(struct IoFile * file, void * buffer, unsigned int count);

//Returns 0
int IoDefaultTruncate(struct IoFile * file, unsigned long long size);

//Returns -ENOTTY
int IoDefaultIoCtl(struct IoFile * file, int request, void * data);

#endif /* FSDEFAULT_H_ */
