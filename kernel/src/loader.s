;
; loader.s
;
;  Copyright 2012 James Cowgill
;
;  Licensed under the Apache License, Version 2.0 (the "License");
;  you may not use this file except in compliance with the License.
;  You may obtain a copy of the License at
;
;      http://www.apache.org/licenses/LICENSE-2.0
;
;  Unless required by applicable law or agreed to in writing, software
;  distributed under the License is distributed on an "AS IS" BASIS,
;  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;  See the License for the specific language governing permissions and
;  limitations under the License.
;
;

global _loader
global TssESP0
global GdtTLS
extern kMain
extern kernelPageDirectory

;Constants
STACK_SIZE equ 0x4000
KERNEL_VIRTUAL_BASE equ 0xC0000000	;Position kernel is loaded into
KERNEL_PAGE_OFFSET equ (KERNEL_VIRTUAL_BASE >> 20)	;Offset of kernel 4MB area in page directory

section .bss nobits alloc noexec write align=4096
align 4096
startupStack:
	resb STACK_SIZE		;Allocate 16k of startup stack

;Global TSS
TssStart:
	resb 4

TssESP0:
	resb 4

TssSS0:
	resb 2

	;Rest of TSS
	resb 94				;Total size = 104 bytes

section .data
align 8
gdt:
	;Global Descriptor Table
	; Since this is constant, it can be set up in the loader

	;The first 8 bytes are not used by the CPU, so we can store
	; the GDT pointer here
	dw 0	;Dummy word to align GDT pointer properly

gdt_ptr:
	dw gdt_end - gdt - 1
	dd gdt		;PTR to GDT (linear address - does not need to be fixed due to paging)

	;Limit, Limit, Base, Base, Base, Permissions + Type, Limit + Constant Flags, Base
	db 0xFF, 0xFF, 0, 0, 0, 0x9A, 0xCF, 0	;Selector 08h - Ring 0 Code
	db 0xFF, 0xFF, 0, 0, 0, 0x92, 0xCF, 0	;Selector 10h - Ring 0 Data
	db 0xFF, 0xFF, 0, 0, 0, 0xFA, 0xCB, 0	;Selector 1Bh - Ring 3 Code (limited to user mode area)
	db 0xFF, 0xFF, 0, 0, 0, 0xF2, 0xCB, 0	;Selector 23h - Ring 3 Data (limited to user mode area)
GdtTLS:
	db 0x00, 0x00, 0, 0, 0, 0xF6, 0xC0, 0	;Selector 2Bh - Ring 3 Thread Local Storage (limited to user mode area)
											; This selector is modified by the scheduler
	db 0x68,    0, 0, 0, 0, 0x89,    0, 0	;Selector 30h - Global Task State Segment
		;TSS entry - assumes: TSS is 104 bytes.

	;Positions of bytes to be loaded with the TSS's offset
	; b1 Last Significant - b4 Most Significant
	tss_b1 equ (gdt + 0x30 + 2)
	tss_b2 equ (gdt + 0x30 + 3)
	tss_b3 equ (gdt + 0x30 + 4)
	tss_b4 equ (gdt + 0x30 + 7)

gdt_end:
	;Don't modify this marker

section .text
	;Multiboot Header
	; Required for GRUB to boot this OS
	dd 0x1BADB002				;Magic Number
	dd 4 | 2					;Flags (populate memory and video modes)
	dd 0xE4524FF8				;Checksum

	times 5 dd 0				;Use load address from ELF file

	dd 1						;Video mode (1 = text)
	dd 80						;Number of Columns
	dd 25						;Number of Rows
	dd 0						;Depth / Bits per pixel (or 0 for text mode)


_loader:
	;Before we setup paging, we must offset any symbols with the KERNEL_VIRTUAL_BASE
	; Also, DO NOT USE EAX OR EBX HERE - it contains the multiboot pointer

	; Setup boot page directory
	mov ecx, (kernelPageDirectory - KERNEL_VIRTUAL_BASE)
	mov dword [ecx], 0x00000183		;This number is: 4MB page, rw, global, mapped to address 0x0
	mov dword [ecx + KERNEL_PAGE_OFFSET], 0x00000183

	; Load boot page directory
	mov cr3, ecx

	; Set PSE bit to enable 4MB paging + other bits for later
	;  MCE - 6	Machine Check Enable
	;  PSE - 4	Page Size Extensions
	mov ecx, 0x50
	mov cr4, ecx

	; Set PG bit to enable paging + other bits for later (this must be done last)
	;  PG - 31	Enable Paging
	;  AM - 18	Alignment Check
	;  WP - 16	Write Protect
	;  NE - 5	Numeric Error
	;  EM - 2	No math coprocessor
	;  PE - 0	Protection Enable
	mov ecx, 0x80050025
	mov cr0, ecx

	; Now we can set the PGE - 7 Page Global Flag
	mov ecx, 0x150
	mov cr4, ecx

	;WE HAVE NOW SETUP PAGING
	; Load TSS SS0
	mov word [TssSS0], 0x10

	; Load TSS location into GDT
	mov ecx, TssStart
	mov [tss_b1], cl
	mov [tss_b2], ch
	shr ecx, 16
	mov [tss_b3], cl
	mov [tss_b4], ch

	; Load our GDT
	lgdt [gdt_ptr]

	; Load TSS
	mov cx, 0x30
	ltr cx

	;Load new selectors
	mov cx, 0x10
	mov ds, cx
	mov es, cx
	mov ss, cx

	;Load CS selector
	jmp 0x08:highLoader

highLoader:
	;Unmap first identity page
	mov dword [kernelPageDirectory], 0x00000000
	invlpg [0]

	;Setup the stack pointer
	mov esp, startupStack + STACK_SIZE

	;Correct the multiboot info pointer
	add ebx, KERNEL_VIRTUAL_BASE

	;Call the main procedure
	push ebx
	push eax
	call kMain

	;If we get here, something bad has happened...
	jmp $
