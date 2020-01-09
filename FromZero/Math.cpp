#include "Math.h"

Rectangle RectMinMax(Vector Min, Vector Max)
{
	Rectangle Result;
	Result.Min = Min;
	Result.Max = Max;

	return Result;
}

Rectangle RectCenterHalfDim(Vector Center, Vector HalfDim)
{
	Rectangle Result;
	Result.Min = Center - HalfDim;
	Result.Max = Center + HalfDim;

	return Result;
}

Rectangle RectCenterDim(Vector Center, Vector Dim)
{
	return RectCenterHalfDim(Center, 0.5f * Dim);
}

Rectangle RectMinDim(Vector Min, Vector Dim)
{
	Rectangle Result;
	Result.Min = Min;
	Result.Max = Min + Dim;

	return Result;
}

bool IsInRectangle(Rectangle R, Vector V)
{
	return ( V.X >= R.Min.X &&
			 V.Y >= R.Min.Y &&
			 V.X <  R.Max.X &&
			 V.Y <  R.Max.Y );
}
