/*
 * linker.ld
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
 */
 
ENTRY(_loader)
OUTPUT_FORMAT(elf32-i386)

SECTIONS {
   /* The kernel will live at 3GB + 1MB in the virtual
      address space, which will be mapped to 1MB in the
      physical address space. */
   . = 0xC0100000;
   
   /* .text, .data and .bss contain the main kernel code and data */
   .text ALIGN (0x1000) : AT(ADDR(.text) - 0xC0000000) {
       obj/kernel/src/loader.o(.text)
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
   
   .init ALIGN(0x1000) : AT(ADDR(.init) - 0xC0000000) {
       _kernel_init_start = .;
       *(.init)
   }

   _kernel_end = .;
   _kernel_end_page = (ALIGN(0x1000) - 0xC0000000) >> 12;
}
