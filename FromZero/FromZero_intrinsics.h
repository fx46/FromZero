#pragma once

#if DEBUG
#define assert(Expression) if(!(Expression)) __debugbreak();
#else
#define assert(Expression)
#endif

#define INT32_MAX        2147483647i32

typedef signed char         int8;
typedef signed short        int16;
typedef signed int          int32;
typedef signed __int64      int64;
typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned __int64    uint64;

struct Bit_Scan_Result
{
	bool Found;
	uint32 Index;
};

Bit_Scan_Result FindLeastSignificantSetBit(uint32 Value);

int32 RoundFloatToINT32(float Real32);

uint32 RoundFloatToUINT32(float Real32);

int32 CeilFloatToINT32(float Real32);

float Fsqrt(float F);

int SignOf(int Value);

int Random();