#pragma once

#include <windows.h>
#include "math.h"

#if DEBUG
#define assert(Expression) if(!(Expression)) __debugbreak();
#else
#define assert(Expression)
#endif

INT32 RoundFloatToINT32(float Real32);

UINT32 RoundFloatToUINT32(float Real32);

// static INT32 FloorFloatToINT32(float Real32)
// {
// 	return static_cast<int>(floorf(Real32));
// }

// static float Sin(float Angle)
// {
// 	return sinf(Angle);
// }
// 
// static float Cos(float Angle)
// {
// 	return cosf(Angle);
// }