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

namespace Chaff
{
	namespace MemMgr
	{
	    //Physical page type
	    typedef int PhysPage;
	
	    //Page size
		#define PAGE_SIZE 4096
	
	    //This class handles allocating pages of physical memory
	    // Generally the physical memory manager doesn't need to be accessed
	    // Use memory regions for allocating memory
	    class PhysicalMgr
	    {
	    private:
			PhysicalMgr() {}

		public:
		    //Initializes the physical memory manager using the memory info
		    // in the multiboot information structure
			static void Init(multiboot_info_t * bootInfo);
			
			//Allocates physical pages
			// This function can return any free page
			static PhysPage AllocatePages(unsigned int number = 1);

			//Allocates physical pages
			// This function will never return any page with an offset > 16M
			static PhysPage AllocateISAPages(unsigned int number = 1);
			
			//Frees 1 physical page allocated by AllocatePages or AllocateISAPages
			static void FreePages(PhysPage page, unsigned int number = 1);

			//Returns the number of pages in memory
			static unsigned int GetTotalPages();

			//Returns the number of free pages available
			static unsigned int GetFreePages();
		};
		
	    //The structure for a x86 page directory
	    struct PageDirectory
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
	    };

	    //The structure for a x86 page table
	    struct PageTable
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
	    };

	    //Bitmask containing the flags for memory regions
	    enum RegionFlags
	    {
	    	NoAccess = 0,
	    	Readable = 1,
	    	Writable = 2,
	    	Executable = 4,
	    };

	    class MContext;

		//Represents a contiguous region of virtual memory and its properties
		// This class also handles physical memory allocation for the region.
	    // (The lower 4MB of kernel memory is not handled using regions)
		class MRegion
		{
		private:
	    	MContext * myContext;		//Context this region is assigned to

	    	RegionFlags flags;				//The properties of the region

	    	void * start;					//Pointer to start of region
	    	unsigned int length;			//Length of region

	    	IO::FileHandle file;			//File used to read data into memory from
	    	unsigned int fileOffset;		//The offset of file to read (ignored if file is null)
	    	unsigned int fileSize;			//The amount of data to read from the file (ignored if file is null)

		public:
	    	void * GetStartAddress()
	    	{
	    		return start;
	    	}

	    	unsigned int GetLength()
	    	{
	    		return length;
	    	}

	    	//Resizes the region of allocated memory
	    	void Resize(unsigned int newLength);

	    	//Deletes this memory region
	    	void Delete();
		};
		
		//A group of memory regions which make up the virtual memory space for a process
		class MContext
		{
		private:
			//The regions of memory in this context
			List<MRegion> regions;	//Regions in the context
			int kernelVersion;			//Version of the kernel page directory in this context
			PageDirectory * directory;	//The 4KB page directory owned by this memory context

			//Constructor and destructor
			MContext();
			~MContext();

		public:
			//The kernel memory context is handled separately
			static MContext kernelContext;

			//Creates a new blank memory context
			static MContext * InitBlank();

			//Clones this memory context
			MContext * Clone();

			//Creates a new region of blank memory
			MRegion * CreateRegion(void * startAddress, unsigned int length);

			//Creates a new memory mapped region
			MRegion * CreateRegion(void * startAddress, unsigned int length, IO::FileHandle file,
					unsigned int fileOffset, unsigned int fileSize);

			//Finds the region which contains the given address
			// or returns NULL if there isn't one
			MRegion * FindRegion(void * address);

			//Switches to this memory context
			void SwitchTo();

			//Deletes this memory context - DO NOT delete the memory context
			// which is currently in use!
			void Delete();
		};
	}
}

#endif
