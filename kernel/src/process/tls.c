/*
 * tls.c
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
 *  Created on: 8 Jan 2012
 *      Author: James
 */

#include "chaff.h"
#include "process.h"

//TLS functions

//Creates a tls descriptor using the given base pointer as an expand down segment
unsigned long long ProcTlsCreateDescriptor(unsigned int basePtr)
{
	//Validate base pointer
	if(basePtr >= 0x1000 && basePtr <= (unsigned int) KERNEL_VIRTUAL_BASE)
	{
		//Create descriptor
		unsigned long long desc = PROC_BASE_TLS_DESCRIPTOR;

		//Store base pointer
		desc |= (basePtr & 0x00FFFFFFULL) << 16;
		desc |= (basePtr & 0xFF000000ULL) << 56;

		//Store segment limit
		unsigned int limit = (0xFFFFFFFF - basePtr) >> 12;
		desc |= (limit & 0x00FFFFULL);
		desc |= (limit & 0xFF0000ULL) << 32;

		return desc;
	}
	else
	{
		return PROC_NULL_TLS_DESCRIPTOR;
	}
}
