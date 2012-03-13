/**
 * @file
 * ELF structures and defines
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

#ifndef ELF_H
#define ELF_H

#include "chaff.h"

/**
 * @name ELF File Types
 * @{
 */
#define LDR_ELF_ET_NONE	0	///< No file type
#define LDR_ELF_ET_REL	1	///< Relocatable object
#define LDR_ELF_ET_EXEC	2	///< Executable object
#define LDR_ELF_ET_DYN	3	///< Shared object
#define LDR_ELF_ET_CORE	4	///< Core dump
/**
 * @}
 */

///ELF 386 machine code
#define LDR_ELF_EM_386 3

/**
 * The ELF header (occurs at the start of every ELF file)
 */
typedef struct LdrElfHeader
{
	unsigned char ident[16];	///< ELF identification
	unsigned short type;		///< File type
	unsigned short machine;		///< Machine architecture
	unsigned int version;		///< ELF version number
	unsigned int entry;			///< Entry point
	unsigned int phOff;			///< Offset of program header (or 0 if there isn't one)
	unsigned int shOff;			///< Offset of section header (or 0 if there isn't one)
	unsigned int flags;			///< File flags
	unsigned short headerSize;	///< ELF header size
	unsigned short phEntSize;	///< Size of each entry in the program header
	unsigned short phNumber;	///< Number of entries in the program header
	unsigned short shEntSize;	///< Size of each entry in the section header
	unsigned short shNumber;	///< Number of entries in the section header
	unsigned short shStrNdx;	///< Section number which holds the string table for the section names

} LdrElfHeader;

/**
 * Validates the identification bytes of the ELF header to ensure this is an ELF file
 *
 * @param header header to validate
 * @retval true if this is an ELF file
 * @retval false if thie is not a valid ELF file
 */
static inline bool LdrElfValidateHeader(LdrElfHeader * header)
{
	return header->ident[0] == 0x7F &&
			header->ident[1] == 'E' &&
			header->ident[2] == 'L' &&
			header->ident[3] == 'F' &&
			header->ident[4] == 1 &&
			header->ident[5] == 1 &&
			header->ident[6] == 1;
}

/**
 * @name Special section numbers
 * @{
 */
#define LDR_ELF_SHN_UNDEF 	0		///< Undefined Section
#define LDR_ELF_SHN_ABS 	0xFFF1	///< Absolute Value
#define LDR_ELF_SHN_COMMON 	0xFFF2	///< COMMON symbols

/**
 * @}
 * @name Section types
 * @{
 */
#define LDR_ELF_SHT_NULL 		0	///< No section data
#define LDR_ELF_SHT_PROGBITS	1	///< Program defined data
#define LDR_ELF_SHT_SYMTAB 		2	///< Symbol table
#define LDR_ELF_SHT_STRTAB 		3	///< String table
#define LDR_ELF_SHT_RELA 		4	///< Relocations with offset
#define LDR_ELF_SHT_HASH 		5	///< Symbol hash table (dynamic only)
#define LDR_ELF_SHT_DYNAMIC 	6	///< Dynamic linking data (dynamic only)
#define LDR_ELF_SHT_NOTE 		7	///< Notes
#define LDR_ELF_SHT_NOBITS 		8	///< Filled with 0s
#define LDR_ELF_SHT_REL 		9	///< Relocations without offset
#define LDR_ELF_SHT_DYNSYM	 	11	///< Dynamic symbol table (dynamic only)

/**
 * @}
 * @name Section flags
 * @{
 */
#define LDR_ELF_SHF_WRITE		1	///< Writable
#define LDR_ELF_SHF_ALLOC		2	///< Occupies virtual memory
#define LDR_ELF_SHF_EXEC		4	///< Executable

/**
 * @}
 */

/**
 * Information about an ELF section
 */
typedef struct LdrElfSection
{
	unsigned int name;		///< Index in string table
	unsigned int type;		///< Section type
	unsigned int flags;		///< Flags
	unsigned int addr;		///< Virtual address
	unsigned int offset;	///< File offset
	unsigned int size;		///< Size of section
	unsigned int link;		///< Reference to another section (depends on section type)
	unsigned int info;		///< Section information (depends on section type)
	unsigned int addrAlign;	///< Section alignment (virtual)
	unsigned int entSize;	///< Size of items in the section (tables only)

} LdrElfSection;

/**
 * Information about an ELF symbol
 */
typedef struct LdrElfSymbol
{
	unsigned int name;		///< Symbol name (index in string table)
	unsigned int value;		///< Symbol value (depends on type)
	unsigned int size;		///< Symbol size (depends on type)
	unsigned int info;		///< Symbol type and attributes
	unsigned int other;		///< Unused
	unsigned int section;	///< Section index of the section this symbol belongs in

} LdrElfSymbol;

#endif
