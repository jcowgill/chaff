;
;  interruptAsm.s
;
;  Created on: 11 Dec 2010
;      Author: James
;

;Contains entry points for interrupts
; The main interrupt code is in interrupt.c

extern IntrHandler
global IntrISRList
global Isr66
global lidt

section .text

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
