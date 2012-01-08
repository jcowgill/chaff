#include "chaff.h"
#include "process.h"

//TLS functions

//Creates a tls descriptor using the given base pointer as an expand down segment
// Returns PROC_NULL_TLS_DESCRIPTOR is an invalid basePtr is given
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
