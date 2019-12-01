#include "FromZero_intrinsics.h"

INT32 RoundFloatToINT32(float Real32)
{
	return static_cast<INT32>(roundf(Real32));
}

UINT32 RoundFloatToUINT32(float Real32)
{
	return static_cast<UINT32>(roundf(Real32));
}