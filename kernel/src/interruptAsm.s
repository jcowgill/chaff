;
; interruptAsm.s
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
;  Created on: 11 Dec 2010
;      Author: James
;

;Contains entry points for interrupts
; The main interrupt code is in interrupt.c

extern IntrHandler
global IntrISRList:data hidden
global Isr66:data hidden
global lidt:function hidden

section .init

;LIDT instruction
lidt:
	;Stack is this
	;   +4       |  +6      |   +8
	; Length (2) | Null (2) | Ptr (2)

	;Move length into garbage area
	mov ax, word [esp + 4]
	mov word [esp + 6], ax

	;Load directly from stack
	lidt [esp + 6]

	ret

section .text

;Basic ISR macros
%macro IsrNoError 1
	Isr%1:
		push 0
		push %1
		jmp IntrCommonEntry
%endmacro

%macro IsrWithError 1
	Isr%1:
		push %1
		jmp IntrCommonEntry
%endmacro

;Declare ISRs
IsrNoError 0
IsrNoError 1
IsrNoError 2
IsrNoError 3
IsrNoError 4
IsrNoError 5
IsrNoError 6
IsrNoError 7
IsrWithError 8
IsrNoError 9
IsrWithError 10
IsrWithError 11
IsrWithError 12
IsrWithError 13
IsrWithError 14
IsrNoError 15
IsrNoError 16
IsrWithError 17

%assign i 18
%rep 30
	IsrNoError i
	%assign i (i + 1)
%endrep

IsrNoError 66		; = 0x42

;ISR address list
align 4
IntrISRList:
	%assign i 0
	%rep 48
		dd Isr %+ i
		%assign i (i + 1)
	%endrep

;Common part of every ISR
IntrCommonEntry:
	;Push registers
	push gs
	push fs
	push es
	push ds
	pushad

	;Update to kernel segments
	mov ax, 0x10
	mov ds, ax
	mov es, ax

	;Clear direction flag (required by ABI)
	cld

	;Call main handler
	call IntrHandler

	;Pop registers
	popad
	pop ds
	pop es
	pop fs
	pop gs

	;Pop interrupt info
	add esp, 8

	;Return
	iretd
