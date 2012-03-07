/**
 * @file
 * CPU Identification and Specific Functions
 *
 * @date February 2012
 * @author James Cowgill
 */

/*
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
 */

#ifndef CPU_H_
#define CPU_H_

#include "chaff.h"

/**
 * Initializes the CPU specific stuff (like FPU and SSE)
 *
 * @private
 */
void INIT CpuInit();

/**
 * Late CPU initializer to initialize FPU state allocator
 *
 * Call this after setting up the slab allocator
 *
 * @private
 */
void INIT CpuInitLate();

/**
 * Contains information about the signature of the processor
 */
typedef struct CpuIdSignatureType
{
	unsigned int stepping : 4;		///< Processor Stepping (Model Revision Number)
	unsigned int model : 4;			///< Processor Model
	unsigned int family : 4;		///< Processor Family
	unsigned int type : 2;			///< Processor Type

	///@cond
	unsigned int : 2;
	///@endcond

	unsigned int extendedModel : 4;		///< Extended Model (used when model = 0x0F)
	unsigned int extendedFamily : 8;	///< Extended Family (used when family = 0x0F)

	///@cond
	unsigned int : 4;
	///@endcond

} CpuIdSignatureType;

/**
 * True if the CPU supports the CPUID instruction
 */
extern bool CpuHasCpuId;

/**
 * The CPU vendor
 *
 * This is a null-terminated string usually of 12 characters.
 *
 * If #CpuHasCpuId is false, the value of this is "Unknown".
 */
extern char CpuIdVendor[];

/**
 * The highest request which can be made through the CPUID instruction.
 *
 * This is the highest value which can be stored in EAX before CPUID is executed.
 */
extern unsigned int CpuIdHighest;

/**
 * The CPU signature (see ::CpuIdSignatureType)
 */
extern CpuIdSignatureType CpuIdSignature;

/**
 * The CPU features reported by CPUID in the EDX register.
 */
extern unsigned int CpuFeaturesEDX;

/**
 * The CPU features reported by CPUID in the ECX register.
 */
extern unsigned int CpuFeaturesECX;

/**
 * True if the CPU supports the "Denormals are Zero" (DAZ) flag in the MXCSR register
 */
extern bool CpuHasDenormalsAreZero;

/**
 * Returns true if the CPU has a Floating-Point Unit (FPU)
 */
static inline bool CpuHasFpu()
{
	return CpuFeaturesEDX & 1;
}

/**
 * Returns true if the CPU has support for the FXSAVE and FXRESTOR instructions
 *
 * @note This does not imply CpuHasSse()
 */
static inline bool CpuHasFxSave()
{
	return (CpuFeaturesEDX & (1 << 24));
}

/**
 * Returns true if the CPU has support for Streaming SIMD Extensions (SSE)
 *
 * @note CPU support for SSE will only be enabled if FXSAVE is also supported
 */
static inline bool CpuHasSse()
{
	return CpuHasFxSave() && (CpuFeaturesEDX & (1 << 25));
}

/**
 * Number of bytes needed to store FPU state with FNSAVE
 */
#define CPU_EXTRA_FPU 0x6C

/**
 * Number of bytes needed to store FPU and SSE state with FXSAVE
 */
#define CPU_EXTRA_FXSAVE 0x200

/**
 * Once ProcThread::fpuSwitches is above this value, the FPU registers are not saved lazily
 */
#define CPU_FPU_SWITCH_THRESHOLD 10

struct ProcThread;
struct IntrContext;

/**
 * Run when a "No Math Coprocessor" exception occurs and this may update the FPU registers.
 *
 * @private
 */
void CpuNoFpuException(struct IntrContext * intrContext);

/**
 * Called by the scheduler when it selects another thread to run
 *
 * @private
 */
void CpuTaskSwitched();

/**
 * Frees the FPU / SSE state for a thread when it is exited
 *
 * @param thread thread to free state for
 * @private
 */
void CpuFreeFpuState(struct ProcThread * thread);

#endif
