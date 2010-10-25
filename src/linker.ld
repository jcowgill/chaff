ENTRY(_loader)
OUTPUT_FORMAT(elf32-i386)

SECTIONS {
   /* The kernel will live at 3GB + 1MB in the virtual
      address space, which will be mapped to 1MB in the
      physical address space. */
   . = 0xC0100000;
   
   /* .text, .data and .bss contain the main kernel code and data */
   .text ALIGN (0x1000) : AT(ADDR(.text) - 0xC0000000) {
       obj/loader.obj(.text)
       *(.text)
   }
   
   .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - 0xC0000000) {
       *(.rodata*)
   }

   .data ALIGN (0x1000) : AT(ADDR(.data) - 0xC0000000) {
		*(.data*)
		*(.gnu.linkonce.d*)
   }

   .bss : AT(ADDR(.bss) - 0xC0000000) {
       _sbss = .;
       *(COMMON*)
       *(.bss)
       *(.gnu.linkonce.b*)
       _ebss = .;
   }

   _kernel_end = .;
   _kernel_end_page = (ALIGN(0x1000) - 0xC0000000) >> 12;
}
