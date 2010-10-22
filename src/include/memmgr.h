/*
 * chaff.h
 *
 *  Created on: 30 Sep 2010
 *      Author: James
 */

#ifndef MEMMGR_H_
#define MEMMGR_H_

//Memory management code

#include "io.h"
#include "list.h"
#include "multiboot.h"

//Physical page type
typedef int PhysPage;

//Page size
#define PAGE_SIZE 4096

//PHYSICAL MEMORY MANAGER
//----------------------------------
//This handles allocating pages of physical memory
// Generally the physical memory manager doesn't need to be accessed
// Use memory regions for allocating memory

//Initializes the physical memory manager using the memory info
// in the multiboot information structure
void MemPhysicalInit(multiboot_info_t * bootInfo);

//Allocates physical pages
// This function can return any free page
PhysPage MemPhysicalAlloc(unsigned int number);

//Allocates physical pages
// This function will never return any page with an offset > 16M
PhysPage MemPhysicalAllocISA(unsigned int number);

//Frees 1 physical page allocated by AllocatePages or AllocateISAPages
void MemPhysicalFree(PhysPage page, unsigned int number);

//Returns the number of pages in memory
unsigned int MemPhysicalGetTotal();

//Returns the number of free pages available
unsigned int MemPhysicalGetFree();


//The structure for a x86 page directory
typedef struct STagPageDirectory
{
	//1 if page is in use
	unsigned int present		: 1;

	//1 if page can be written to (otherwise writing causes a page fault)
	unsigned int writable		: 1;

	//1 if page is available from user mode
	unsigned int userMode		: 1;

	//1 if all writes should bypass the cache
	unsigned int writeThrough	: 1;

	//1 if all reads should bypass the cache
	unsigned int cacheDisable	: 1;

	//1 if the page has been accessed since this was last set to 0
	unsigned int accessed		: 1;

	//1 if the page has been written to since this was last set to 0
	// Ignored if this is a page table entry
	unsigned int dirty			: 1;

	//1 if this entry references a 4MB page rather than a page table
	unsigned int hugePage		: 1;

	//1 if this page should not be invalidated when changing CR3
	// Must enable this feature in a CR4 bit first
	// Ignored if this is a page table entry
	unsigned int global			: 1;

	unsigned int /* can be used by OS */	: 3;

	//The page frame ID of the 4MB page or page table
	// (as returned by pmem_allocpage)
	// If this is a 4MB page, it MUST be aligned to 4MB (or we'll crash)
	unsigned int pageID			: 20;

} PageDirectory;

//The structure for a x86 page table
struct STagPageTable
{
	//1 if page is in use
	unsigned int present		: 1;

	//1 if page can be written to (otherwise writing causes a page fault)
	unsigned int writable		: 1;

	//1 if page is available from user mode
	unsigned int userMode		: 1;

	//1 if all writes should bypass the cache
	unsigned int writeThrough	: 1;

	//1 if all reads should bypass the cache
	unsigned int cacheDisable	: 1;

	//1 if the page has been accessed since this was last set to 0
	unsigned int accessed		: 1;

	//1 if the page has been written to since this was last set to 0
	unsigned int dirty			: 1;
	unsigned int /* reserved */	: 1;

	//1 if this page should not be invalidated when changing CR3
	// Must enable this feature in a CR4 bit first
	unsigned int global			: 1;

	unsigned int /* can be used by OS */	: 3;

	//The page frame ID of the 4KB page
	// (as returned by pmem_allocpage)
	unsigned int pageID			: 20;

} PageTable;

//Bitmask containing the flags for memory regions
// Note that restrictions only apply to user mode
typedef enum ETagRegionFlags
{
	NoAccess = 0,
	Readable = 1,
	Writable = 2,
	Executable = 4,
	Fixed = 8,

} RegionFlags;

//MEMORY REGIONS AND CONTEXT
//-------------------------------
//Represents a contiguous region of virtual memory and its properties
// This class also handles physical memory allocation for the region.
// (The lower 4MB of kernel memory is not handled using regions)

struct STagMemContext;

typedef struct STagMemRegion
{
	ListItem item;					//Item in regions list

	struct StagMemContext * myContext;	//Context this region is assigned to

	RegionFlags flags;				//The properties of the region

	void * start;					//Pointer to start of region
	unsigned int length;			//Length of region in pages
	PhysPage firstPage;				//First page - used only for fixed regions

	FileHandle file;				//File used to read data into memory from
	unsigned int fileOffset;		//The offset of file to read (ignored if file is null)
	unsigned int fileSize;			//The amount of data to read from the file (ignored if file is null)

} MemRegion;

//A group of memory regions which make up the virtual memory space for a process
typedef struct STagMemContext
{
	ListItem regions;				//List head containing all the regions
	unsigned int kernelVersion;		//Version of kernel page directory for this context
	PageDirectory * directory;		//Page directory (4KB)

} MemContext;

//Initialises the kernel memory context
void MemContextInitKernel();

//Creates a new blank memory context
MemContext * MemContextInit();

//Clones this memory context
MemContext * MemContextClone();

//Switches to this memory context
void MemContextSwitchTo(MemContext * context);

//Deletes this memory context - DO NOT delete the memory context
// which is currently in use!
void MemContextDelete(MemContext * context);

//Frees the pages associated with a given region of memory without destroying the region
// (pages will automatically be allocated when memory is referenced)
void MemContextFreePages(MemContext * context, void * address, unsigned int length);

//Finds the region which contains the given address
// or returns NULL if there isn't one
MemRegion * MemRegionFind(MemContext * context, void * address);

//Creates a new memory mapped region
// If the context given is not the current or kernel context,
//  a temporary memory context switch may occur
MemRegion * MemRegionCreateMMap(MemContext * context, void * startAddress,
		unsigned int length, FileHandle file,
		unsigned int fileOffset, unsigned int fileSize);

//Creates a new blank memory region
MemRegion * MemRegionCreate(MemContext * context, void * startAddress,
		unsigned int length)
{
	return MemRegionCreateMMap(context, startAddress, length, 0, 0, 0);
}

//Creates a new fixed memory section
// These regions always point to a particular area of physical memory
// If the context given is not the current or kernel context,
//  a temporary memory context switch may occur
MemRegion * MemRegionCreateFixed(MemContext * context, void * startAddress,
		unsigned int length, PhysPage firstPage);

//Resizes the region of allocated memory
// If the context of the region given is not the current or kernel context,
//  a temporary memory context switch may occur
void MemRegionResize(MemRegion * region, unsigned int newLength);

//Deletes the specified region
// If the context of the region given is not the current or kernel context,
//  a temporary memory context switch may occur
void MemRegionDelete(MemRegion * region);

//Kernel context
// The page directory is NULL in this context
extern MemContext MemKernelContextData;
#define MemKernelContext (&MemKernelContextData)

#endif
