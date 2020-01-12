#include "FromZero_intrinsics.h"
#include "FromZero_Platform.h"
#include <intrin0.h>
#include <stdlib.h>
#include <corecrt_math.h>

Bit_Scan_Result FindLeastSignificantSetBit(uint32 Value)
{
	Bit_Scan_Result Result = {};

#if COMPILER_MSVC
	Result.Found = _BitScanForward(reinterpret_cast<unsigned long *>(&Result.Index), Value);
#else // COMPILER_MSVC
	for (uint32 i = 0; i < sizeof(Value) * CHAR_BIT; ++i)
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

int32 RoundFloatToINT32(float Real32)
{
	return static_cast<int32>(roundf(Real32));
}

uint32 RoundFloatToUINT32(float Real32)
{
	return static_cast<uint32>(roundf(Real32));
}

int32 CeilFloatToINT32(float Real32)
{
	return static_cast<int32>(ceilf(Real32));
}

float AbsoluteValue(float F)
{
	return fabs(F);
}

float Fsqrt(float F)
{
	return sqrtf(F);
}

int SignOf(int Value)
{
	return (Value >= 0) ? 1 : -1;
}

int Random()
{
	return rand();
}
