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
#include "list.h"

//Determines if a value is a power of 2
#define IS_POWER_OF_2(val) (((val) & ((val) - 1)) == 0)

//Head of the list of modules
static ListHead moduleList = LIST_INLINE_INIT(moduleList);

#warning Add return codes to doxygen listings

//Stores addresses of sections
typedef union LdrSectionAddress
{
	//Offset to load section at (relative to base of module)
	unsigned int loadOff;

	//Virtual address
	char * vAddr;

} LdrSectionAddress;

//Adds a dependency without the circular dependency check
static int AddDependencyNoCheck(LdrModule * from, LdrModule * to);

//Loads a module into the kernel
LdrModule * LdrLoadModule(const void * data, unsigned int len, const char * args)
{
	bool loaded = false;
	LdrElfHeader * elfHeader = (LdrElfHeader *) data;

	//Validate ELF header
	if(len < sizeof(LdrElfHeader) || !LdrElfValidateHeader(elfHeader) ||
			elfHeader->type != LDR_ELF_ET_REL || elfHeader->machine != LDR_ELF_EM_386)
	{
		PrintLog(Error, "LdrLoadModule: Module has invalid ELF header");
		return NULL;
	}

	//Get section header pointer
	LdrElfSection * firstSection = (LdrElfSection *) (elfHeader->shOff + data);

	if(elfHeader->shEntSize != sizeof(LdrElfSection))
	{
		PrintLog(Error, "LdrLoadModule: Module has invalid section table entry size");
		return NULL;
	}

	if(elfHeader->shOff + elfHeader->shNumber * sizeof(LdrElfSection) > len)
	{
		//Not long enough
		PrintLog(Error, "LdrLoadModule: Module has invalid section table");
		return NULL;
	}

	//Maximum of 1024 sections
	if(elfHeader->shNumber > 1024)
	{
		PrintLog(Error, "LdrLoadModule: Module has too many sections (maximum of 1024)");
		return NULL;
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
			goto finally;
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
				goto finally;
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
				goto finally;
			}

			//Get string table entry
			if(section->link >= elfHeader->shNumber)
			{
				PrintLog(Error, "LdrLoadModule: Module has invalid section table");
				goto finally;
			}

			LdrElfSection * strTabSection = &firstSection[section->link];

			//Validate string table
			if(strTabSection->type != LDR_ELF_SHT_STRTAB)
			{
				PrintLog(Error, "LdrLoadModule: Module has invalid section table");
				goto finally;
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
				goto finally;
			}

			//Ensure remote section is valid
			if(section->link >= elfHeader->shNumber)
			{
				PrintLog(Error, "LdrLoadModule: Module has invalid section table");
				goto finally;
			}
		}
	}

	//Validate sizes and core tables
	if(symTab == NULL || allocBytes + strTabLen > LDR_MAX_MODULE_SIZE)
	{
		PrintLog(Error, "LdrLoadModule: Module has invalid section table");
		goto finally;
	}

	//Allocate data for module and module info structure
	LdrModule * moduleInfo = MemKZAlloc(sizeof(LdrModule));
	ListHeadInit(&moduleInfo->symbols);
	ListHeadInit(&moduleInfo->modules);
	moduleInfo->dataStart = MemVirtualAlloc(allocBytes + strTabLen);

	//Copy string table to end
	char * strTabPtr = (char *) moduleInfo->dataStart + allocBytes;
	MemCpy(strTabPtr, strTab, strTabLen);

	//Calculate load addresses for each section
	for(unsigned int i = 0; i < elfHeader->shNumber; i++)
	{
		sectionAddrs[i].vAddr = (char *) (moduleInfo->dataStart + sectionAddrs[i].loadOff);
	}

	//Load all sections into memory
	section = firstSection;
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
	for(unsigned int i = 0; i < elfHeader->shNumber; i++, section++)
	{
		//Relocation?
		if(section->type == LDR_ELF_SHT_REL)
		{
			//Process section
			LdrElfSection * remoteSection = &firstSection[section->info];
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

									//Add dependency on the module
									int depRetVal = AddDependencyNoCheck(moduleInfo, kernSymbol->module);
									if(depRetVal != 0 && depRetVal != -EEXIST)
									{
										PrintLog(Error, "LdrLoadModule: Too many module dependencies");
										goto error2;
									}
								}
								else
								{
									//Undefined symbol
#warning Which symbol is undefined?
									PrintLog(Error, "LdrLoadModule: Undefined symbol ...");
									goto error2;
								}
							}

							break;

						case LDR_ELF_SHN_ABS:
							//Use absolute value of symbol
							symValue = symbol->value;
							break;

						case LDR_ELF_SHN_COMMON:
							//We do not handle this
							PrintLog(Error, "LdrLoadModule: Modules cannot be loaded with COMMON symbols (hint: pass -d to ld)");
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
							(unsigned int) symValue - (sectionAddrs[section->link].loadOff + rel->offset);
				}
				else
				{
					//Illegal relocation type
					PrintLog(Error, "LdrLoadModule: Module has corrupt relocation table");
					goto error2;
				}
			}
		}
	}

	//Load global and special symbols from the symbol table
	bool displayedWeakWarning = false;
	LdrModuleInitFunc initFunc = NULL;

	for(unsigned int i = 0; i < symTabCount; i++)
	{
		const char * symbolName;
		void * symbolValue;

		//Ignore section, file and undefined symbols
		if(LDR_ELF_ST_TYPE(symTab[i].info) == LDR_ELF_STT_SECTION ||
			LDR_ELF_ST_TYPE(symTab[i].info) == LDR_ELF_STT_FILE ||
			symTab[i].section == LDR_ELF_SHN_UNDEF)
		{
			continue;
		}

		//No common symbols
		if(symTab[i].section == LDR_ELF_SHN_COMMON)
		{
			PrintLog(Error, "LdrLoadModule: Modules cannot be loaded with COMMON symbols (hint: pass -d to ld)");
			goto error3;
		}

		//Only interpret global and weak symbols
		switch(LDR_ELF_ST_BIND(symTab[i].info))
		{
			case LDR_ELF_STB_WEAK:
				//Display warning
				if(!displayedWeakWarning)
				{
					PrintLog(Warning, "LdrLoadModule: Weak symbols are treated as globals");
					displayedWeakWarning = true;
				}

				//Fallthrough

			case LDR_ELF_STB_GLOBAL:
				//Global symbol
				// Calculate symbol value
				switch(symTab[i].section)
				{
					case LDR_ELF_SHN_ABS:
						//Absolute symbol
						symbolValue = (void *) symTab[i].value;
						break;

					default:
						//Link relative to start of given section
						if(symTab[i].section < elfHeader->shNumber)
						{
							symbolValue = sectionAddrs[symTab[i].section].vAddr + symTab[i].value;
						}
						else
						{
							PrintLog(Error, "LdrLoadModule: Module has corrupt symbol table");
							goto error3;
						}

						break;
				}

				// Get symbol name
				if(symTab[i].name >= strTabLen)
				{
					//Invalid name
					PrintLog(Error, "LdrLoadModule: Symbol has invalid name");
					goto error3;
				}

				symbolName = &strTabPtr[symTab[i].name];

				// Is it a special symbol?
				if(StrCmp(symbolName, "ModuleInit") == 0)
				{
					//Init function
					initFunc = symbolValue;
				}
				else if(StrCmp(symbolName, "ModuleCleanup") == 0)
				{
					//Cleanup function
					moduleInfo->cleanup = symbolValue;
				}
				else if(StrCmp(symbolName, "ModuleName") == 0)
				{
					//Module name
					moduleInfo->name = (const char *) symbolValue;
				}
				else
				{
					// Normal symbol to be added to the symbol table
					if(!LdrKSymbolAdd(symbolName, symbolValue, moduleInfo))
					{
	#warning Print symbol name
						PrintLog(Error, "LdrLoadModule: Exported symbol is already defined");
						goto error3;
					}
				}

				break;
		}
	}

	//Modules must have a name
	if(moduleInfo->name == NULL)
	{
		PrintLog(Error, "LdrLoadModule: Modules must define a ModuleName variable containing a string");
		goto error3;
	}

	//Add module to global modules list
	ListHeadAddLast(&moduleInfo->modules, &moduleList);

	//Execute init function
	if(initFunc)
	{
		initFunc(moduleInfo, args ? args : "");
	}

	loaded = true;
	goto finally;

error3:
	//Free symbols
	LdrKSymbolRemoveModule(moduleInfo);

error2:
	//Free module memory
	MemVirtualFree(moduleInfo->dataStart);
	MemKFree(moduleInfo);

finally:
	//Free section addresses
	MemKFree(sectionAddrs);

	//Return correct value
	return loaded ? moduleInfo : NULL;
}

//Adds a dependency
int LdrAddDependency(LdrModule * from, LdrModule * to)
{
	//Do circular dependency check
	for(int i = 0; i < LDR_MAX_DEPENDENCIES; i++)
	{
		if(to->deps[i] == NULL)
		{
			break;
		}
		else if(to->deps[i] == from)
		{
			return -ELOOP;
		}
	}

	//Forward to other adder
	return AddDependencyNoCheck(from, to);
}

//Adds a dependency without the circular dependency check
static int AddDependencyNoCheck(LdrModule * from, LdrModule * to)
{
	//Ignore requests for the NULL module (ie the kernel)
	if(to == NULL)
	{
		return 0;
	}

	//Already have a reference?
	int arrayPos = 0;

	while(arrayPos < LDR_MAX_DEPENDENCIES)
	{
		//What is this module?
		if(from->deps[arrayPos] == to)
		{
			return -EEXIST;
		}
		else if(from->deps[arrayPos] == NULL)
		{
			//Insert here at the end of the list
			from->deps[arrayPos] = to;
			to->depRefCount++;
			return 0;
		}
	}

	//No space left in array
#warning TODO print module name here
	PrintLog(Warning, "LdrAddDependency: no space left in dependency array");
	return -ENOSPC;
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
	for(int i = 0; i < LDR_MAX_DEPENDENCIES; i++)
	{
		//Is there a module?
		if(module->deps[i] == NULL)
		{
			//No - end check
			break;
		}
		else
		{
			//Decrement ref count
			module->deps[i]->depRefCount--;
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
