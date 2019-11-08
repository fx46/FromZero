#pragma once

#include <windows.h>
#include "math.h"

static INT32 RoundFloatToINT32(float Real32)
{
	return static_cast<INT32>(roundf(Real32));
}

static UINT32 RoundFloatToUINT32(float Real32)
{
	return static_cast<UINT32>(roundf(Real32));
}

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