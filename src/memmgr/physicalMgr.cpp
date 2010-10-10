/*
 * physicalMgr.cpp
 *
 *  Created on: 6 Oct 2010
 *      Author: James
 */

#include "chaff.h"
#include "memmgr.h"

using namespace Chaff;
using namespace Chaff::MemMgr;

/*
mmap (len = 0x90, addr = 0xC002c628)

0xC002c628 : 0xC002C628 <Hex>
  Address   0 - 3     4 - 7     8 - B     C - F
  C002C610  00000000  A07F0600  947F0600  6C7F0600
  C002C620  59980100  00000000  14000000  00000000
  C002C630  00000000  00F00900  00000000  01000000
  C002C640  14000000  00F00900  00000000  00100000
  C002C650  00000000  02000000  14000000  00800E00
  C002C660  00000000  00800100  00000000  02000000
  C002C670  14000000  00001000  00000000  0000EF01
  C002C680  00000000  01000000  14000000  0000FF01
  C002C690  00000000  00000100  00000000  03000000
  C002C6A0  14000000  0000FCFF  00000000  00000400
  C002C6B0  00000000  02000000  726F6F74  20286664
  C002C6C0  3029006B  65726E65  6C202F63  68616666
  C002C6D0  2E656C66  00000043  68616666  0000626F
  C002C6E0  6F74006C  202F6368  6166662E  656C6600
  C002C6F0  00000000  00000000  00000000  00000000

grub> displaymem
 EISA Memory BIOS Interface is present
 Address Map BIOS Interface is present
 Lower memory: 636K, Upper memory (to first chipset hole): 31680K
 [Address Range Descriptor entries immediately follow (values are 64-bit)]
   Usable RAM:  Base Address:  0x0 X 4GB + 0x0,
      Length:   0x0 X 4GB + 0x9f000 bytes
   Reserved:  Base Address:  0x0 X 4GB + 0x9f000,
      Length:   0x0 X 4GB + 0x1000 bytes
   Reserved:  Base Address:  0x0 X 4GB + 0xe8000,
      Length:   0x0 X 4GB + 0x18000 bytes
   Usable RAM:  Base Address:  0x0 X 4GB + 0x100000,
      Length:   0x0 X 4GB + 0x1ef0000 bytes
   Reserved:  Base Address:  0x0 X 4GB + 0x1ff0000,
      Length:   0x0 X 4GB + 0x10000 bytes
   Reserved:  Base Address:  0x0 X 4GB + 0xfffc0000,
      Length:   0x0 X 4GB + 0x40000 bytes

If bit 6 in the ‘flags’ word is set, then the ‘mmap_*’ fields are valid,
and indicate the address and length of a buffer containing a memory map of
the machine provided by the bios. ‘mmap_addr’ is the address, and ‘mmap_length’
 is the total size of the buffer. The buffer consists of one or more of the
 following size/structure pairs (‘size’ is really used for skipping to the
 next pair):

             +-------------------+
     -4      | size              |
             +-------------------+
     0       | base_addr         |
     8       | length            |
     16      | type              |
             +-------------------+

where ‘size’ is the size of the associated structure in bytes, which can be
 greater than the minimum of 20 bytes. ‘base_addr’ is the starting address.
 ‘length’ is the size of the memory region in bytes. ‘type’ is the variety of
  address range represented, where a value of 1 indicates available ram, and
	all other values currently indicated a reserved area.

The map provided is guaranteed to list all standard ram that should be available for normal use.


If bit 3 of the ‘flags’ is set, then the ‘mods’ fields indicate to the kernel
what boot modules were loaded along with the kernel image, and where they can
be found. ‘mods_count’ contains the number of modules loaded; ‘mods_addr’
contains the physical address of the first module structure. ‘mods_count’
may be zero, indicating no boot modules were loaded, even if bit 1 of ‘flags’
is set. Each module structure is formatted as follows:

             +-------------------+
     0       | mod_start         |
     4       | mod_end           |
             +-------------------+
     8       | string            |
             +-------------------+
     12      | reserved (0)      |
             +-------------------+

The first two fields contain the start and end addresses of the boot module
itself. The ‘string’ field provides an arbitrary string to be associated with
that particular boot module; it is a zero-terminated ASCII string, just like
the kernel command line. The ‘string’ field may be 0 if there is no string
associated with the module. Typically the string might be a command line
(e.g. if the operating system treats boot modules as executable programs),
 or a pathname (e.g. if the operating system treats boot modules as files
 in a file system), but its exact use is specific to the operating system.
  The ‘reserved’ field must be set to 0 by the boot loader and ignored by
	the operating system.

Example code:
      if (CHECK_FLAG (mbi->flags, 2))
         printf ("cmdline = %s\n", (char *) mbi->cmdline);

       /* Are mods_* valid? /
       if (CHECK_FLAG (mbi->flags, 3))
         {
           multiboot_module_t *mod;
           int i;

           printf ("mods_count = %d, mods_addr = 0x%x\n",
                   (int) mbi->mods_count, (int) mbi->mods_addr);
           for (i = 0, mod = (multiboot_module_t *) mbi->mods_addr;
                i < mbi->mods_count;
                i++, mod++)
             printf (" mod_start = 0x%x, mod_end = 0x%x, cmdline = %s\n",
                     (unsigned) mod->mod_start,
                     (unsigned) mod->mod_end,
                     (char *) mod->cmdline);
         }

       /* Bits 4 and 5 are mutually exclusive! /
       if (CHECK_FLAG (mbi->flags, 4) && CHECK_FLAG (mbi->flags, 5))
         {
           printf ("Both bits 4 and 5 are set.\n");
           return;
         }

       /* Is the symbol table of a.out valid? /
       if (CHECK_FLAG (mbi->flags, 4))
         {
           multiboot_aout_symbol_table_t *multiboot_aout_sym = &(mbi->u.aout_sym);

           printf ("multiboot_aout_symbol_table: tabsize = 0x%0x, "
                   "strsize = 0x%x, addr = 0x%x\n",
                   (unsigned) multiboot_aout_sym->tabsize,
                   (unsigned) multiboot_aout_sym->strsize,
                   (unsigned) multiboot_aout_sym->addr);
         }

       /* Is the section header table of ELF valid? /
       if (CHECK_FLAG (mbi->flags, 5))
         {
           multiboot_elf_section_header_table_t *multiboot_elf_sec = &(mbi->u.elf_sec);

           printf ("multiboot_elf_sec: num = %u, size = 0x%x,"
                   " addr = 0x%x, shndx = 0x%x\n",
                   (unsigned) multiboot_elf_sec->num, (unsigned) multiboot_elf_sec->size,
                   (unsigned) multiboot_elf_sec->addr, (unsigned) multiboot_elf_sec->shndx);
         }

       /* Are mmap_* valid? /
       if (CHECK_FLAG (mbi->flags, 6))
         {
           multiboot_memory_map_t *mmap;

           printf ("mmap_addr = 0x%x, mmap_length = 0x%x\n",
                   (unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
           for (mmap = (multiboot_memory_map_t *) mbi->mmap_addr;
                (unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
                mmap = (multiboot_memory_map_t *) ((unsigned long) mmap
                                         + mmap->size + sizeof (mmap->size)))
             printf (" size = 0x%x, base_addr = 0x%x%x,"
                     " length = 0x%x%x, type = 0x%x\n",
                     (unsigned) mmap->size,
                     mmap->addr >> 32,
                     mmap->addr & 0xffffffff,
                     mmap->len >> 32,
                     mmap->len & 0xffffffff,
                     (unsigned) mmap->type);
         }

MULTIBOOT.H CONTAINS THE MEMORY MAP STRUCTURE!!


 *
 */

//Bitmap pointers
// In the bitmap - 1 is allocated, 0 is not allocated

//Start of bitmap
#define bitmapStart (reinterpret_cast<unsigned int *>(KERNEL_VIRTUAL_BASE))

//End of bitmap
static unsigned int * bitmapEnd;

//Next part of bitmap to allocate from
static unsigned int * bitmapHead;

//Page counters
static unsigned int totalPages;
static unsigned int freePages;

//True if some ISA memory has been freed since the last sweep to the ISA region
static bool isaFreeSinceLastSweep = false;

//Fills a continuous region of the bitmap from startPage to endPage
static void FillBitmap(PhysPage startPage, PhysPage endPage, bool allocate, bool updateTotal = true)
{
	//Ensure pages are in correct order
	if(startPage >= endPage)
	{
		return;
	}

	//Find start and end pointers
	unsigned int * startPtr = bitmapStart + (startPage / 32);
	unsigned int * endPtr = bitmapStart + (endPage / 32);

	//Construct bitmask for allocating at the start and freeing at the end
	unsigned int startAllocMask = 0xFFFFFFFF >> (startPage % 32);
	unsigned int endFreeMask = 0xFFFFFFFF >> (endPage % 32);

	//Handle same pointers
	if(startPtr == endPtr)
	{
		//Get new mask using XOR
		unsigned int mask = startAllocMask ^ endFreeMask;

		//Modify the bitmap
		if(allocate)
		{
			*startPtr |= mask;
		}
		else
		{
			*startPtr &= ~mask;
		}
	}
	else
	{
		//Modify start and end pointers
		if(allocate)
		{
			*startPtr |= startAllocMask;
			*endPtr |= ~endFreeMask;
		}
		else
		{
			*startPtr &= ~startAllocMask;
			*endPtr &= endFreeMask;
		}

		//Modify 4 byte sections in the middle
		for(++startPtr; startPtr < endPtr; ++startPtr)
		{
			*startPtr = allocate ? 0xFFFFFFFF : 0;
		}
	}

	//Update counters
	if(updateTotal)
	{
		if(allocate)
		{
			totalPages -= endPage - startPage;
		}
		else
		{
			totalPages += endPage - startPage;
		}
	}
}

//Iterates over each mmap entry in <in> and stores them in <entryVar>
// The length of the memory map is passed in <len>
// <entryVar> is a multiboot_memory_map_t
// All other vars MUST be unsigned longs
#define MMAP_FOREACH(entryVar, in, len) \
	for(multiboot_memory_map_t * entryVar = reinterpret_cast<multiboot_memory_map_t *>(in); \
		reinterpret_cast<unsigned long>(entryVar) < (in) + (len); \
		entryVar = reinterpret_cast<multiboot_memory_map_t *>( \
			reinterpret_cast<unsigned long>(entryVar) + entryVar->size + sizeof(entryVar->size)))

//Initializes the physical memory manager using the memory info
// in the multiboot information structure
void PhysicalMgr::Init(multiboot_info_t * bootInfo)
{
	//Must have memory info avaliable
	if(bootInfo->flags & MULTIBOOT_INFO_MEM_MAP)
	{
		//Display info
		PrintLog(Info, "Reading physical memory map...\n");

		//1st Pass - find highest memory location to get size of bitmap
	    unsigned long highAddr = 0;
	    
		MMAP_FOREACH(mmapEntry, bootInfo->mmap_addr, bootInfo->mmap_length)
		{
		    //Only map available regions
		    if(mmapEntry->type == MULTIBOOT_MEMORY_AVAILABLE)
		    {
		        //Ignore regions starting in 64 bit
		        // + if region ends in 64 bit - use full highAddr
		        if((mmapEntry->addr >> 32) != 0)
		        {
		            continue;
				}
				else
				{
				    //Regions which end in 64 bits raise the address to the maximum
					if(mmapEntry->addr + mmapEntry->len >= 0xFFFFFFFF)
					{
						highAddr = 0xFFFFFFFF;
					}
					else
					{
						//Update only if this is higher
						if(static_cast<unsigned long>(mmapEntry->addr + mmapEntry->len) > highAddr)
						{
							highAddr = static_cast<unsigned long>(mmapEntry->addr + mmapEntry->len);
						}
					}
				}
			}
		}

		//Determine end location
		// 4 bytes stores 32 pages or 0x20000 data bytes
		// We round this up
		bitmapEnd = bitmapStart + ((highAddr + 0x1FFFF) / 0x20000);

		//Allocate everything
		MemSet(bitmapStart, 0xFF, bitmapEnd - bitmapStart);

		//2nd pass - free available parts of memory
		MMAP_FOREACH(mmapEntry, bootInfo->mmap_addr, bootInfo->mmap_length)
		{
		    //Only free available regions
		    if(mmapEntry->type == MULTIBOOT_MEMORY_AVAILABLE && (mmapEntry->addr >> 32) == 0)
		    {
		    	//Check if region flows into 64 bit region
		    	if(mmapEntry->addr + mmapEntry->len > 0xFFFFFFFF)
		    	{
			    	FillBitmap((mmapEntry->addr + 4095) / 4096, 0x100000000ULL / 4096, false);
		    	}
		    	else
		    	{
					//Mark region as free
					FillBitmap((mmapEntry->addr + 4095) / 4096,
							(mmapEntry->addr + mmapEntry->len + 4095) / 4096, false);
		    	}
			}
		}

		//Reallocate already allocated stuff
		FillBitmap(0, ((bitmapEnd - bitmapStart) + 4095) / 4096, true, false);	//Bitmap
		FillBitmap(0xA0, 0x100, true, false);	//ROM Area
		FillBitmap((unsigned int) KERNEL_VIRTUAL_BASE / 4096,
				((unsigned int) KERNEL_VIRTUAL_BASE / 4096) + 0x400, true, false);	//Kernel 4MB

		//Set head to the beginning
		bitmapHead = bitmapStart;
	}
	else
	{
		Panic("Memory information not given to OS");
	}
}

//Allocates physical pages
// If lower16Meg is true, only pages in the lower 16M of memory will be returned
PhysPage PhysicalMgr::AllocatePages(bool lower16Meg /* = false */)
{
	//Save head at beginning
	unsigned int * startHead = bitmapHead;

	//Go though bitmap to find a page starting with the head
	do
	{
		//Ensure head is in the bitmap
		if(bitmapHead >= bitmapEnd)
		{
			//Restore to beginning
			bitmapHead = bitmapStart;
		}

		//Check if there is a free page
		if(*bitmapHead != 0xFFFFFFFF)
		{
			unsigned int firstFree;

			//If blank, use first page
			if(*bitmapHead == 0)
			{
				firstFree = 0;
			}
			else
			{
				//Yes - somewhere...
				firstFree = BitScanReverse(~*bitmapHead);
			}

			//Set allocated
			*bitmapHead |= 0x80000000 >> firstFree;

			//Return page
			return (bitmapHead - bitmapStart) * 32 + firstFree;
		}

		//Move on head
		++bitmapHead;
	}
	while(bitmapHead != startHead);

	//Out of memory!
	Panic("Out of memory");
}

//Frees 1 physical page allocated by AllocatePage
void PhysicalMgr::FreePages(PhysPage page)
{
	//Check for ISA mem
	if(page < 0x1000)
	{
		isaFreeSinceLastSweep = true;
	}

	//Free from bitmap
	*(bitmapStart + (page / 32)) &= ~(0x80000000 >> (page % 32));
	--freePages;
}

//Returns the number of pages in memory
unsigned int PhysicalMgr::GetTotalPages()
{
	return totalPages;
}

//Returns the number of free pages available
unsigned int PhysicalMgr::GetFreePages()
{
	return freePages;
}
