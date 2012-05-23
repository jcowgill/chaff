/*
 * ksymbols.c
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
 *  Created on: 9 Mar 2012
 *      Author: James
 */

#include "chaff.h"
#include "multiboot.h"
#include "loader/elf.h"
#include "loader/ksymbols.h"
#include "mm/kmemory.h"

//Kernel symbol table
HashTable LdrKSymbolTable;

//Slab allocator for symbol objects
static MemCache * symbolCache;

//Adds a kernel symbol
bool LdrKSymbolAdd(const char * name, void * value, LdrModule * module)
{
	//Create symbol object
	LdrKSymbol * symbol = MemSlabAlloc(symbolCache);

	symbol->value = value;
	symbol->module = module;
	ListHeadInit(&symbol->moduleList);

	//Add to hash table
	if(HashTableInsert(&LdrKSymbolTable, &symbol->tableItem, name, StrLen(name, 256)))
	{
		//Add to symbol chain
		if(module)
		{
			ListHeadAddLast(&symbol->moduleList, &module->symbols);
		}

		return true;
	}
	else
	{
		//Symbol alredy exists
		MemSlabFree(symbolCache, symbol);
		return false;
	}
}

//Removes all the symbols belonging to a module
void LdrKSymbolRemoveModule(LdrModule * module)
{
	//Iterate through symbol list destroying modules
	LdrKSymbol * symbol, * tmpSymbol;
	bool failedRemove = false;

	ListForEachEntrySafe(symbol, tmpSymbol, &module->symbols, moduleList)
	{
		//Remove symbol
		failedRemove |= HashTableRemoveItem(&LdrKSymbolTable, &symbol->tableItem);

		//Free symbol
		MemSlabFree(symbolCache, symbol);
	}

	//Clear list
	ListHeadInit(&module->symbols);

	//Print warning if symbols could not be removed
	if(failedRemove)
	{
#warning Print module name here
		PrintLog(Warning, "LdrKSymbolRemoveModule: failed to remove some symbols for module ");
	}
}

//Read kernel symbols
void INIT LdrReadKernelSymbols(multiboot_info_t * mHeader)
{
	//Create slab cache for symbols
	symbolCache = MemSlabCreate(sizeof(LdrKSymbol), 0);

	//Symbols must be passed
	if((mHeader->flags & MULTIBOOT_INFO_ELF_SHDR) == 0)
	{
		Panic("LdrReadKernelSymbols: boot loader did not pass any kernel symbols");
	}

	//Find symbol table
	LdrElfSection * section = (LdrElfSection *) (mHeader->u.elf_sec.addr + KERNEL_VIRTUAL_BASE);
	for(unsigned int i = 0; i < mHeader->u.elf_sec.num; i++)
	{
		//Symbol table?
		if(section->type == LDR_ELF_SHT_SYMTAB)
		{
			//Yes, get the associated string table
			// Validate entry size and string table index
			if(section->entSize == 0 || section->link >= mHeader->u.elf_sec.num)
			{
				Panic("LdrReadKernelSymbols: Corrupt section header in kernel image");
			}

			// Get table
			LdrElfSection * strTable =
					(LdrElfSection *) (mHeader->u.elf_sec.size * section->link +
							mHeader->u.elf_sec.addr + KERNEL_VIRTUAL_BASE);

			// Is it a string table?
			if(strTable->type != LDR_ELF_SHT_STRTAB)
			{
				Panic("LdrReadKernelSymbols: Corrupt section header in kernel image");
			}

			// Clone string table into kernel space
			char * kStrTable = MemVirtualAlloc(strTable->size);
			MemCpy(kStrTable, strTable->addr + KERNEL_VIRTUAL_BASE, strTable->size);

			//Get symbol table offsets
			LdrElfSymbol * symbol = (LdrElfSymbol *)
					(section->info * section->entSize + section->addr + KERNEL_VIRTUAL_BASE);
			LdrElfSymbol * endOfTable = (LdrElfSymbol *) (section->addr + section->size + KERNEL_VIRTUAL_BASE);

			//Start search at first global symbol
			for(; symbol < endOfTable; symbol = (LdrElfSymbol *) ((char *) symbol + section->entSize))
			{
				//Add symbol
				if(!LdrKSymbolAdd(&kStrTable[symbol->name], (void *) symbol->value, NULL))
				{
					Panic("LdrReadKernelSymbols: Corrupt symbol table in kernel image");
				}
			}

			//Finished adding symbols
			return;
		}

		//Advance section
		section = (LdrElfSection *) ((char *) section + mHeader->u.elf_sec.size);
	}

	//No symbol table?!
	Panic("LdrReadKernelSymbols: No symbol table found in kernel image");
}
