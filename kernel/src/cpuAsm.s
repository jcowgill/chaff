;
; cpuAsm.s
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

global CpuInit
global CpuHasCpuId
global CpuIdVendor
global CpuIdSignature
global CpuFeaturesEDX
global CpuFeaturesECX
extern Panic

section .data
CpuHasCpuId:
	;True if CPU supports the CPUID instruction
	dd 0

CpuIdVendor:
	;Cpu Vendor (from CPUID)
	db "Unknown", 0, 0, 0, 0, 0, 0, 0, 0, 0

CpuIdHighest:
	;Highest request CPUID will accept
	dd 0

CpuIdSignature:
	;Signature of CPU (stepping, model, family)
	dd 0

CpuFeaturesEDX:
	;CPU Features as returned by CPUID (or calculated)
	dd 0

CpuFeaturesECX:
	dd 0

Cpu386ErrMsg:
	db "CpuInit: You are running a 386 processor which Chaff does not support.", 0

section .text
CpuInit:
	;Initializes the CPU specific stuff
	push ebx

	;Test for Intel 486
	pushfd
	pop eax				;Save flags into eax
	mov ecx, eax		;Take a copy in ecx (of the correct flags)
	xor eax, 0x40000	;Flip AC bit
	push eax			;Save flags again
	popfd

	;Check if AC was changed
	pushfd
	pop eax
	xor eax, ecx		;Compare to what it should be
	jnz .using486

	;Oh Noes
	; This is a 386 processor which we don't support
	push Cpu386ErrMsg
	call Panic

.using486:
	;Test for CPUID instruction
	push ecx			;Restore original flags
	popfd
	mov eax, ecx
	xor eax, 0x200000	;Flip ID bit
	push eax
	popfd

	;Check if ID was changed
	pushfd
	pop eax
	xor eax, ecx
	je .postIdentify	;No CPUID, finished identification

	;If we're here, we can execute CPUID
	mov dword [CpuHasCpuId], 1

	;Get vendor + store it
	xor eax, eax
	cpuid
	mov dword [CpuIdHighest], eax
	mov dword [CpuIdVendor], ebx
	mov dword [CpuIdVendor + 4], edx
	mov dword [CpuIdVendor + 8], ecx

	;Get cpu features + store them
	mov eax, 1
	cpuid
	mov dword [CpuIdSignature], eax
	mov dword [CpuFeaturesEDX], edx
	mov dword [CpuFeaturesECX], ecx

.postIdentify:
	; ###########################################################
	;CPU identfication finished
	; Setup control registers and other stuff

	;If the FPU feature bit is not set, and we're not using CPUID, probe for an FPU
	mov eax, dword [CpuFeaturesEDX]
	and eax, 1
	jnz .afterProbe

	mov ecx, dword [CpuHasCpuId]
	test ecx, ecx
	jnz .afterProbe

	;Try to detect an FPU manually (without help from CPUID)
	; Turn off FPU disable bits of CR0
	mov ecx, cr0
	and ecx, 0xFFFFFFF3		;Clear EM and TS bits
	mov cr0, ecx

	; Attempt to reset FPU and then read status word
	push 0xDEADBEEF
	fninit
	fnstsw [esp]

	; Status word must be 0 for an FPU
	xor eax, eax
	pop ecx
	test ecx, ecx
	setz al

.hasFPU:
	;Has an FPU - set FPU bit
	mov dword [CpuFeaturesEDX], 1
	mov eax, 1
	ret

.afterProbe:
	;Setup CR0 register (FPU, CPU caches and WP bit)
	mov ecx, cr0
	or ecx, 0x00050028	;Set TS, NE, WP and AM bits
	and ecx, 0x9FFFFFF9	;Clear EM, MP, CD and NW bits

	;Add bits for FPU
	test eax, eax
	jz .noFPU

	;WITH FPU
	or ecx, 0x2		;Set MP bit
	fninit			;Force FPU Reset
	jmp .postSetupFPU

.noFPU:
	;WITHOUT FPU
	or ecx, 0x4		;Set EM bit

.postSetupFPU:
	;Store CR0 state
	mov cr0, ecx

	; ###########################################################
	; Setup CPU Extensions

	mov edx, dword [CpuFeaturesEDX]
	xor ecx, ecx

	;Page Size Extensions (4MB pages)
	test edx, (1 << 3)
	jz .noPSE

	;Enable page size extensions
	or ecx, 0x10	;Set PSE bit

.noPSE:
	;Machine check exception
	test edx, (1 << 7)
	jz .noMachineCheck

	;Enable machine check in CR4
	or ecx, 0x40	;Set MCE bit

.noMachineCheck:
	;Page global bit
	test edx, (1 << 13)
	jz .noPageGlobal

	;Enable page global in CR4
	or ecx, 0x80	;Set PGE bit

.noPageGlobal:
	;SSE instructions (including FXSTOR)
	test edx, (1 << 24)
	jz .noSSE
	test edx, (1 << 25)
	jz .noSSE

	;Enable OSFXSR and OSXMMEXCPT bits in CR4
	or ecx, 0x600

.noSSE:
	;Save in CR4 (if CR4 exists)
	test ecx, ecx
	jz .postCPUExtend

	mov eax, cr4
	or eax, ecx
	mov cr4, eax

.postCPUExtend:
	;Finished
	pop ebx
	ret
