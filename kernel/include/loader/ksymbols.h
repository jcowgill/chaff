/**
 * @file
 * Kernel Symbol Table
 *
 * @date March 2012
 * @author James Cowgill
 * @ingroup Loader
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

#ifndef KSYMBOL_H
#define KSYMBOL_H

#include "chaff.h"
#include "list.h"
#include "htable.h"
#include "loader/module.h"

struct multiboot_info;

/**
 * A kernel symbol
 */
typedef struct LdrKSymbol
{
	/**
	 * Value of the symbol (variable or function call)
	 */
	void * value;

	/**
	 * Item in the module list (owning module)
	 */
	ListHead moduleList;

	/**
	 * Item in the global symbol table
	 */
	HashItem tableItem;

} LdrKSymbol;

/**
 * Kernel symbol table
 */
extern HashTable LdrKSymbolTable;

/**
 * Returns the name of the given symbol
 *
 * @param symbol symbol to get name
 * @return the name of the symbol (as a string)
 */
static inline const char * LdrKSymbolName(LdrKSymbol * symbol)
{
	return symbol->tableItem.keyPtr;
}

/**
 * Returns the length of the name of the given symbol
 *
 * @param symbol symbol to get length of
 * @return the length of the symbol name
 */
static inline unsigned int LdrKSymbolNameLen(LdrKSymbol * symbol)
{
	return symbol->tableItem.keyLen;
}

/**
 * Looks up a kernel symbol with the given name
 *
 * @param name name of symbol
 * @param nameLen length of name
 * @retval NULL symbol does not exist
 * @retval symbol the looked up symbol
 */
static inline LdrKSymbol * LdrKSymbolLookup(const char * name, unsigned int nameLen)
{
	HashItem * item = HashTableFind(&LdrKSymbolTable, name, nameLen);

	if(item)
	{
		return HashTableEntry(item, LdrKSymbol, tableItem);
	}
	else
	{
		return NULL;
	}
}

/**
 * Adds a kernel symbol
 *
 * @param name name of symbol
 * @param value value of symbol
 * @param module module the symbol belongs to (or NULL for the kernel)
 *
 * @retval true the symbol was successfully added
 * @retval false the symbol already exists
 */
bool LdrKSymbolAdd(const char * name, void * value, LdrModule * module);

/**
 * Removes all the symbols belonging to a module
 *
 * @param module module to remove symbols from
 */
void LdrKSymbolRemoveModule(LdrModule * module);

/**
 * Loads the kernel symbol table using information in the multiboot header
 *
 * Also sets up the kernel symbol table hash table.
 *
 * @param mHeader multiboot header
 * @private
 */
void INIT LdrReadKernelSymbols(struct multiboot_info * mHeader);

#endif
