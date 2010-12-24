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
#include "interrupt.h"

/*
 * Memory Layout
 * -------------
 * 00000000 - 00000FFF	Reserved (cannot be mapped)
 * 00001000 - BFFFFFFF	User mode code
 * C0000000 - C03FFFFF	Mapped to first 4MB of physical space. Contains all kernel code.
 * C0400000 - CFFFFFFF	Driver code
 * D0000000 - DFFFFFFF	Kernel heap
 * F0000000 - FF6FFFFF	No use at the moment
 * FF700000 - FFFFFFFF	Memory manager use
 * 	FF700000 - FF7FFFFF		Temporary page mappings
 * 	FF800000 - FFBFFFFF		Physical page references
 * 	FFC00000 - FFFFFFFF		Current page tables
 */

//Memory manager initialisation
void MemManagerInit(multiboot_info_t * bootInfo);

//Physical page type
typedef int PhysPage;

//Page returned when an error occurs
#define INVALID_PAGE (-1)

//Page size
#define PAGE_SIZE 4096

//PHYSICAL MEMORY MANAGER
//----------------------------------
//This handles allocating pages of physical memory
// Generally the physical memory manager doesn't need to be accessed
// Use memory regions for allocating memory

typedef struct
{
	//The number of references to the page
	unsigned int refCount;

} MemPageStatus;

//Page status area
#define MemPageStateTable ((MemPageStatus *) 0xFF800000)
extern MemPageStatus * MemPageStateTableEnd;		//Address after page state table (this can be NULL)


//Allocates physical pages
// This function can return any free page
PhysPage MemPhysicalAlloc(unsigned int number);

//Allocates physical pages
// This function will never return any page with an offset > 16M
PhysPage MemPhysicalAllocISA(unsigned int number);

//Frees 1 physical page allocated by AllocatePages or AllocateISAPages
void MemPhysicalFree(PhysPage page, unsigned int number);

//Number of pages in total and number of free pages of memory
extern unsigned int MemPhysicalTotalPages;
extern unsigned int MemPhysicalFreePages;


//The structure for a x86 page directory
typedef union UTagPageDirectory
{
	//Gets the raw value of the page directory
	unsigned int rawValue;

	struct
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
		// If this is a 4MB page, it MUST be aligned to 4MB (or we'll crash)
		PhysPage pageID				: 20;
	};

} PageDirectory;

//The structure for a x86 page table
typedef union UTagPageTable
{
	//Gets the raw value of the page table
	unsigned int rawValue;

	struct
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

		//Not used by CPU
		// 3 bits of table counter to count number of pages allocated in page table
		// tableCount is used only in the first 5 page table entries
		unsigned int tableCount		: 3;

		//The page frame ID of the 4KB page
		PhysPage pageID				: 20;
	};

} PageTable;

//Bitmask containing the flags for memory regions
// Note that restrictions only apply to user mode
typedef enum ETagRegionFlags
{
	MEM_NOACCESS = 0,
	MEM_READABLE = 1,
	MEM_WRITABLE = 2,
	MEM_EXECUTABLE = 4,
	MEM_FIXED = 8,
	MEM_MAPPEDFILE = 16,
	MEM_CACHEDISABLE = 32,

	MEM_ALLFLAGS = 63

} RegionFlags;

//MEMORY REGIONS AND CONTEXT
//-------------------------------
//Represents a contiguous region of virtual memory and its properties
// This class also handles physical memory allocation for the region.
// (The lower 4MB of kernel memory is not handled using regions)

struct STagMemContext;

typedef struct STagMemRegion
{
	struct list_head listItem;		//Item in regions list

	struct STagMemContext * myContext;	//Context this region is assigned to

	RegionFlags flags;				//The properties of the region

	unsigned int start;				//Pointer to start of region
	unsigned int length;			//Length of region in pages
	PhysPage firstPage;				//First page - used only for fixed regions

	FileHandle file;				//File used to read data into memory from
	unsigned int fileOffset;		//The offset of file to read
	unsigned int fileSize;			//The amount of data to read from the file

} MemRegion;

//A group of memory regions which make up the virtual memory space for a process
typedef struct STagMemContext
{
	struct list_head regions;		//Regions in this context
	unsigned int kernelVersion;		//Version of kernel page directory for this context
	PhysPage physDirectory;			//Page directory physical page

} MemContext;

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
// The memory address range must be withing the bounds of the given region
void MemRegionFreePages(MemRegion * region, void * address, unsigned int length);

//Finds the region which contains the given address
// or returns NULL if there isn't one
MemRegion * MemRegionFind(MemContext * context, void * address);

//Creates a new blank memory region
// If the context given is not the current or kernel context,
//  a temporary memory context switch may occur
// The start address MUST be page aligned
MemRegion * MemRegionCreate(MemContext * context, void * startAddress,
		unsigned int length, RegionFlags flags);

//Creates a new memory mapped region
// If the context given is not the current or kernel context,
//  a temporary memory context switch may occur
// MEM_MAPPEDFILE is implied by this
MemRegion * MemRegionCreateMMap(MemContext * context, void * startAddress,
		unsigned int length, FileHandle file,
		unsigned int fileOffset, unsigned int fileSize, RegionFlags flags);

//Creates a new fixed memory section
// These regions always point to a particular area of physical memory
// If the context given is not the current or kernel context,
//  a temporary memory context switch may occur
// MEM_FIXED is implied by this
MemRegion * MemRegionCreateFixed(MemContext * context, void * startAddress,
		unsigned int length, PhysPage firstPage, RegionFlags flags);

//Resizes the region of allocated memory
// If the context of the region given is not the current or kernel context,
//  a temporary memory context switch may occur
void MemRegionResize(MemRegion * region, unsigned int newLength);

//Deletes the specified region
// If the context of the region given is not the current or kernel context,
//  a temporary memory context switch may occur
void MemRegionDelete(MemRegion * region);

//Kernel context
extern MemContext MemKernelContextData;
#define MemKernelContext (&MemKernelContextData)

//Page fault handler
// This MUST be called from an interrupt context
void MemPageFaultHandler(IntrContext * context);

//Malloc Init
void MAllocInit();

#endif
