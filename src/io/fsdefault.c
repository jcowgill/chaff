/*
 * fsdefault.c
 *
 *  Created on: 19 Jan 2012
 *      Author: james
 */

#include "chaff.h"
#include "io/fsdefault.h"
#include "errno.h"

//Returns 0
int IoDefaultUMount(IoFilesystem * fs)
{
	IGNORE_PARAM fs;

	return 0;
}

//Returns 1
unsigned int IoDefaultGetRootINode(IoFilesystem * fs)
{
	IGNORE_PARAM fs;

	return 1;
}

//Returns -EROFS
int IoDefaultCreate(IoFilesystem * fs, IoINode * parent,
		const char * name, int nameLen, IoMode mode, unsigned int * iNodeNum)
{
	IGNORE_PARAM fs;
	IGNORE_PARAM parent;
	IGNORE_PARAM name;
	IGNORE_PARAM nameLen;
	IGNORE_PARAM mode;
	IGNORE_PARAM iNodeNum;

	return -EROFS;
}

//Returns 0
int IoDefaultOpen(IoINode * iNode, struct IoFile * file)
{
	IGNORE_PARAM iNode;
	IGNORE_PARAM file;

	return 0;
}

//Returns 0
int IoDefaultClose(struct IoFile * file)
{
	IGNORE_PARAM file;

	return 0;
}

//Returns 0 (EOF)
int IoDefaultRead(struct IoFile * file, void * buffer, unsigned int count)
{
	IGNORE_PARAM file;
	IGNORE_PARAM buffer;
	IGNORE_PARAM count;

	return 0;
}

//Returns -EPERM
int IoDefaultWrite(struct IoFile * file, void * buffer, unsigned int count)
{
	IGNORE_PARAM file;
	IGNORE_PARAM buffer;
	IGNORE_PARAM count;

	return -EPERM;
}

//Returns -EPERM
int IoDefaultTruncate(struct IoFile * file, unsigned long long size)
{
	IGNORE_PARAM file;
	IGNORE_PARAM size;

	return 0;
}

//Returns -ENOTTY
int IoDefaultIoCtl(struct IoFile * file, int request, void * data)
{
	IGNORE_PARAM file;
	IGNORE_PARAM request;
	IGNORE_PARAM data;

	return -ENOTTY;
}
