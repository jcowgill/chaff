;
; schedulerAsm.s
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
;  Created on: 22 Dec 2010
;      Author: James
;

global ProcIntSchedulerSwap
global ProcIntUserThreadEntry
global ProcIntIdleThread
global ProcIntKernelThreadReturn

extern ProcYield
extern ProcExitThread

;This is STDCALL
; void __stdcall ProcIntSchedulerSwap(void * newStackPtr, void ** oldStackPtr)
ProcIntSchedulerSwap:
	;Swaps the stack and returns
	; This cannot be done inline since it could return to somewhere other than DoSchedule

	;Save registers on old kStack
	push ebp
	push ebx
	push esi
	push edi

	;Save old stack
	mov eax, [esp + 0x18]
	mov [eax], esp

	;Load new stack
	mov esp, [esp + 0x14]

	;Pop registers
	pop edi
	pop esi
	pop ebx
	pop ebp

	;Return
	ret 8

;Entry point for user mode threads
; This is called as a result of the ret above in ProcIntSchedulerSwap
ProcIntUserThreadEntry:
	;The stack must be setup like this
	; (esp = top of stack after being setup)
	;
	; [esp + 2C] = 0x23 (user mode data selector)
	; [esp + 28] = User mode stack pointer (initial esp)
	; [esp + 24] = 0x202 (initial eflags - IF with reserved B1)
	; [esp + 20] = 0x1B (user mode code selector)
	; [esp + 1C] = User mode entry point (initial eip)

	; --- Below this is used by ProcIntSchedulerSwap to get to this point ---
	; [esp + 18] = 0 (discarded)
	; [esp + 14] = 0 (discarded)
	; [esp + 10] = ProcIntUserThreadEntry (pointer to this function)
	; [esp + C]  = 0 (initial ebp)
	; [esp + 8]  = 0 (initial ebx)
	; [esp + 4]  = 0 (initial esi)
	; [esp]		 = 0 (initial edi)
	;

	;Set segment selectors
	mov ax, 0x23
	mov ds, ax
	mov es, ax
	mov ax, 0x28
	mov fs, ax
	mov gs, ax

	;Clear uninitialized registers (security)
	mov eax, 0
	mov ecx, eax
	mov edx, eax

	;Jump to user mode
	iret

;Return address for all kernel thread
; Simply calls ProcExitThread
ProcIntKernelThreadReturn:
	;Stack should have 1 arg alloced - replace with return code
	mov [esp], eax

	;Call thread exit
	call ProcExitThread


;Idle thread
ProcIntIdleThread:
	;Loop around waiting for an interrupt then preempting myself
	sti
	hlt
	cli

	call ProcYield
	jmp ProcIntIdleThread
