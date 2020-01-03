#pragma once

#include <windows.h>

#if DEBUG
#define assert(Expression) if(!(Expression)) __debugbreak();
#else
#define assert(Expression)
#endif

struct Bit_Scan_Result
{
	bool Found;
	UINT32 Index;
};

Bit_Scan_Result FindLeastSignificantSetBit(UINT32 Value);

INT32 RoundFloatToINT32(float Real32);

UINT32 RoundFloatToUINT32(float Real32);

float Fsqrt(float F);

int SignOf(int Value);