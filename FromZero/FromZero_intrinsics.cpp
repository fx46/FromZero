#include "FromZero_intrinsics.h"
#include "FromZero_Platform.h"
#include "math.h"

Bit_Scan_Result FindLeastSignificantSetBit(UINT32 Value)
{
	Bit_Scan_Result Result = {};

#if COMPILER_MSVC
	Result.Found = _BitScanForward(reinterpret_cast<unsigned long *>(&Result.Index), Value);
#else // COMPILER_MSVC
	for (UINT32 i = 0; i < sizeof(Value) * CHAR_BIT; ++i)
	{
		if (Value & (1 << i))
		{
			Result.Found = true;
			Result.Index = i;
			break;
		}
	}
#endif
	return Result;
}

INT32 RoundFloatToINT32(float Real32)
{
	return static_cast<INT32>(roundf(Real32));
}

UINT32 RoundFloatToUINT32(float Real32)
{
	return static_cast<UINT32>(roundf(Real32));
}

float Fsqrt(float F)
{
	return sqrtf(F);
}

int SignOf(int Value)
{
	return (Value >= 0) ? 1 : -1;
}
