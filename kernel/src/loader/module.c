/*
 * module.c
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
 *  Created on: 11 Mar 2012
 *      Author: James
 */

#include "chaff.h"
#include "loader/ksymbols.h"
#include "loader/module.h"
#include "loader/elf.h"
#include "errno.h"
#include "mm/kmemory.h"

//Determines if a value is a power of 2
#define IS_POWER_OF_2(val) (((val) & ((val) - 1)) == 0)

//Head of the list of modules
static ListHead moduleList;

#warning Add return codes to doxygen listings

//Stores addresses of sections
typedef union LdrSectionAddress
{
	//Offset to load section at (relative to base of module)
	unsigned int loadOff;

	//Virtual address
	char * vAddr;

} LdrSectionAddress;

//Loads a module into the kernel
int LdrLoadModule(const void * data, unsigned int len, bool runInit, const char * args)
{
	LdrElfHeader * elfHeader = (LdrElfHeader *) data;

	//Validate ELF header
	if(len < sizeof(LdrElfHeader) || !LdrElfValidateHeader(elfHeader) ||
			elfHeader->type != LDR_ELF_ET_REL || elfHeader->machine != LDR_ELF_EM_386)
	{
		PrintLog(Error, "LdrLoadModule: Module has invalid ELF header");
		return -EINVAL;
	}

	//Get section header pointer
	LdrElfSection * section = (LdrElfSection *) (elfHeader->shOff + data);

	if(elfHeader->shOff + elfHeader->shEntSize * elfHeader->shNumber > len)
	{
		//Not long enough
		PrintLog(Error, "LdrLoadModule: Module has invalid section table");
		return -EINVAL;
	}

	//Maximum of 1024 sections
	if(elfHeader->shNumber > 1024)
	{
		PrintLog(Error, "LdrLoadModule: Module has too many sections (maximum of 1024)");
		return -EINVAL;
	}

	//Find size to allocate for module
	LdrSectionAddress * sectionAddrs = MemKZAlloc(sizeof(LdrSectionAddress) * elfHeader->shNumber);
	unsigned int allocBytes = 0;

	LdrElfSection * symTabSection = NULL;
	LdrElfSection * strTabSection = NULL;

	for(unsigned int i = 0; i < elfHeader->shNumber; i++)
	{
		//Validate section length
		if(section->offset + section->size > len)
		{
			//Not long enough
			PrintLog(Error, "LdrLoadModule: Module has invalid section table");
			goto error1;
		}

		//Test if to be allocated?
		if(section->flags & LDR_ELF_SHF_ALLOC)
		{
			//Align bytes allocation
			unsigned int alignment = section->addrAlign;

			// Must be power of 2 and less than 4096
			if(alignment > 4096)
			{
				PrintLog(Warning, "LdrLoadModule: alignments greater than 4096 bytes are no supported");
				alignment = 4096;
			}
			else if(!IS_POWER_OF_2(alignment))
			{
				PrintLog(Error, "LdrLoadModule: section alignments must be powers of 2");
				goto error1;
			}
			else if(alignment == 0)
			{
				alignment = 1;
			}

			// Perform alignment
			allocBytes = (allocBytes + alignment - 1) & ~(alignment - 1);

			// Store section load address
			sectionAddrs[i].loadOff = allocBytes;

			// Add number of bytes to allocate
			allocBytes += section->size;
		}
		else if(section->type == LDR_ELF_SHT_SYMTAB && symTabSection == 0)
		{
			//Get string table entry
			if(section->link >= elfHeader->shNumber)
			{
				PrintLog(Error, "LdrLoadModule: Module has invalid section table");
				goto error1;
			}

			strTabSection = (LdrElfSection *) (data + section->link * section->entSize);

			//Validate string table
			if(strTabSection->type != LDR_ELF_SHT_STRTAB)
			{
				PrintLog(Error, "LdrLoadModule: Module has invalid section table");
				goto error1;
			}

			//Store section number
			symTabSection = section;
		}

		//Advance section
		section = (LdrElfSection *) ((char *) section + elfHeader->shEntSize);
	}

	//Validate sizes and core tables
	if(symTabSection == NULL || strTabSection == NULL ||
			allocBytes + strTabSection->size > LDR_MAX_MODULE_SIZE)
	{
		PrintLog(Error, "LdrLoadModule: Module has invalid section table");
		goto error1;
	}

	//Allocate data for module
	void * moduleData = MemVirtualAlloc(allocBytes + strTabSection->size);

	//Copy string table to end
	const char * strTabPtr = (const char *) moduleData + allocBytes;
	MemCmp(strTabPtr, data + strTabSection->offset, strTabSection->size);

	//Calculate load addresses for each section
	for(unsigned int i = 0; i < elfHeader->shNumber; i++)
	{
		sectionAddrs[i].vAddr = (char *) (moduleData + sectionAddrs[i].loadOff);
	}

#error TODO
	//Load all sections into memory
	//Load my symbol table
	//Relocate all sections
	//Find ModuleInfo structure and set it up

error2:
	MemVirtualFree(moduleData);

error1:
	MemKFree(sectionAddrs);
	return -EINVAL;
}

//Looks up a module structure by name
LdrModule * LdrLookupModule(const char * name)
{
	LdrModule * module;

	ListForEachEntry(module, &moduleList, modules)
	{
		//This module?
		if(StrCmp(module->name, name) == 0)
		{
			return module;
		}
	}

	return NULL;
}

//Unloads the given module
int LdrUnloadModule(LdrModule * module)
{
	//Any modules dependent on this?
	if(module->depRefCount != 0)
	{
		return -EBUSY;
	}

	//Cleanup module
	int res = module->cleanup();
	if(res != 0)
	{
		return res;
	}

	//Decrement ref counts in this module's dependancies
	if(module->deps)
	{
		for(int i = 0; module->deps[i]; i++)
		{
			LdrModule * depMod = LdrLookupModule(module->deps[i]);

			if(depMod)
			{
				depMod->depRefCount--;
			}
		}
	}

	//Remove symbols
	LdrKSymbolRemoveModule(module);

	//Remove from list
	ListDelete(&module->modules);

	//Free data area
	MemVirtualFree(module->dataStart);

	return 0;
}
