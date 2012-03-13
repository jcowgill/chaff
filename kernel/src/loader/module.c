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
	LdrElfSection * firstSection = (LdrElfSection *) (elfHeader->shOff + data);

	if(elfHeader->shEntSize != sizeof(LdrElfSection))
	{
		PrintLog(Error, "LdrLoadModule: Module has invalid section table entry size");
		return -EINVAL;
	}

	if(elfHeader->shOff + elfHeader->shNumber * sizeof(LdrElfSection) > len)
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

	LdrElfSymbol * symTab = NULL;
	char * strTab;
	unsigned int symTabCount;
	unsigned int strTabLen;

	LdrElfSection * section = firstSection;

	for(unsigned int i = 0; i < elfHeader->shNumber; i++, section++)
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
		else if(section->type == LDR_ELF_SHT_SYMTAB && symTab == NULL)
		{
			//Must use standard sized entries
			if(section->entSize != sizeof(LdrElfSymbol))
			{
				PrintLog(Error, "LdrLoadModule: Module has invalid symbol table entry size");
				goto error1;
			}

			//Get string table entry
			if(section->link >= elfHeader->shNumber)
			{
				PrintLog(Error, "LdrLoadModule: Module has invalid section table");
				goto error1;
			}

			LdrElfSection * strTabSection = &firstSection[section->link];

			//Validate string table
			if(strTabSection->type != LDR_ELF_SHT_STRTAB)
			{
				PrintLog(Error, "LdrLoadModule: Module has invalid section table");
				goto error1;
			}

			//Store gathered information
			symTab = (LdrElfSymbol *) (data + section->offset);
			strTab = (char *) (data + strTabSection->offset);
			symTabCount = section->size / sizeof(LdrElfSymbol);
			strTabLen = strTabSection->size;
		}
		else if(section->type == LDR_ELF_SHT_REL)
		{
			//Check for standard sized entries
			if(section->entSize != sizeof(LdrElfRelocation))
			{
				PrintLog(Error, "LdrLoadModule: Module has invalid relocation table entry size");
				goto error1;
			}

			//Ensure remote section is valid
			if(section->link >= elfHeader->shNumber)
			{
				PrintLog(Error, "LdrLoadModule: Module has invalid section table");
				goto error1;
			}
		}
	}

	//Validate sizes and core tables
	if(symTab == NULL || allocBytes + strTabLen > LDR_MAX_MODULE_SIZE)
	{
		PrintLog(Error, "LdrLoadModule: Module has invalid section table");
		goto error1;
	}

	//Allocate data for module
	void * moduleData = MemVirtualAlloc(allocBytes + strTabLen);

	//Copy string table to end
	const char * strTabPtr = (const char *) moduleData + allocBytes;
	MemCmp(strTabPtr, strTab, strTabLen);

	//Calculate load addresses for each section
	for(unsigned int i = 0; i < elfHeader->shNumber; i++)
	{
		sectionAddrs[i].vAddr = (char *) (moduleData + sectionAddrs[i].loadOff);
	}

	//Load all sections into memory
	for(unsigned int i = 0; i < elfHeader->shNumber; i++, section++)
	{
		//Test if to be allocated?
		if(section->flags & LDR_ELF_SHF_ALLOC)
		{
			//Copying data?
			if(section->type == LDR_ELF_SHT_NOBITS)
			{
				//Set data
				MemSet(sectionAddrs[i].vAddr, 0, section->size);
			}
			else
			{
				//Copy data
				MemCpy(sectionAddrs[i].vAddr, data + section->offset, section->size);
			}
		}
	}
	
	//Relocate all sections
	section = firstSection;
	for(unsigned int i = 0; i < elfHeader->shNumber; i++)
	{
		//Relocation?
		if(section->type == LDR_ELF_SHT_REL)
		{
			//Process section
			LdrElfSection * remoteSection = &firstSection[section->link];
			LdrElfRelocation * rel = (LdrElfRelocation *) (data + section->offset);
			unsigned int itemCount = section->size / sizeof(LdrElfRelocation);
			
			//Only perform relocations for allocated sections
			if((remoteSection->flags & LDR_ELF_SHF_ALLOC) == 0)
			{
				continue;
			}

			//Process each relocation entry
			for(unsigned int j = 0; j < itemCount; j++, rel++)
			{
				// Ignore NONE relocations
				if(LDR_ELF_REL_TYPE(rel->info) == LDR_ELF_REL_NONE)
				{
					continue;
				}

				// Validate symbol
				if(LDR_ELF_REL_SYM(rel->info) > symTabCount)
				{
					PrintLog(Error, "LdrLoadModule: Module has corrupt relocation table");
					goto error2;
				}

				// Validate relocation offset
				if(rel->offset + 4 > remoteSection->size)
				{
					PrintLog(Error, "LdrLoadModule: Module has corrupt relocation table");
					goto error2;
				}

				// Find symbol and resolve absolute address
				unsigned int symValue;

				if(LDR_ELF_REL_SYM(rel->info) == LDR_ELF_STN_UNDEF)
				{
					//UNDEF is defined to have an absolute address of 0
					symValue = 0;
				}
				else
				{
					LdrElfSymbol * symbol = &symTab[LDR_ELF_REL_SYM(rel->info)];

					//What to do?
					switch(symbol->section)
					{
						case LDR_ELF_SHN_UNDEF:
							//Undefined symbol - lookup in global symbol table
							{
								LdrKSymbol * kernSymbol =
										LdrKSymbolLookup(
												&strTabPtr[symbol->name],
												StrLen(&strTabPtr[symbol->name], 256)
											);

								if(kernSymbol)
								{
									//Use symbol value
									symValue = (unsigned int) kernSymbol->value;
								}
								else
								{
									//Undefined symbol
#warning Must check dependencies before relocations
#warning Which symbol is undefined?
									PrintLog(Error, "LdrLoadModule: Undefined symbol ...");
									goto error2;
								}
							}

							break;

						case LDR_ELF_SHN_ABS:
							//Use abolute value of symbol
							symValue = symbol->value;
							break;

						case LDR_ELF_SHN_COMMON:
							//We do not handle this
							PrintLog(Error, "LdrLoadModule: Module has COMMON symbol");
							goto error2;

						default:
							//Link relative to start of given section
							if(symbol->section < elfHeader->shNumber)
							{
								symValue = (unsigned int) (sectionAddrs[symbol->section].vAddr + symbol->value);
							}
							else
							{
#warning Consistent (and meaningful to sysadmins) error messages
								PrintLog(Error, "LdrLoadModule: Module has corrupt symbol table");
								goto error2;
							}

							break;
					}
				}

				//Perform relocation
				if(LDR_ELF_REL_TYPE(rel->info) == LDR_ELF_REL_32)
				{
					//Add symbol value to memory location
					*((unsigned int *) (sectionAddrs[section->link].vAddr + rel->offset)) +=
							(unsigned int) symValue;
				}
				else if(LDR_ELF_REL_TYPE(rel->info) == LDR_ELF_REL_PC32)
				{
					//Add symbol value minus section offset to memory location
					*((unsigned int *) (sectionAddrs[section->link].vAddr + rel->offset)) +=
							(unsigned int) symValue - sectionAddrs[section->link].loadOff;
				}
				else
				{
					//Illegal relocation type
					PrintLog(Error, "LdrLoadModule: Module has corrupt relocation table");
					goto error2;
				}
			}
		}

		//Advance section
		section = (LdrElfSection *) ((char *) section + elfHeader->shEntSize);
	}

#error TODO
	//Load my symbol table
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
