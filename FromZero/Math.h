#pragma once
#include "Vector.h"

struct Rectangle
{
	Vector Min;
	Vector Max;
};

Rectangle RectMinMax(Vector Min, Vector Max);
Rectangle RectCenterHalfDim(Vector Center, Vector HalfDim);
Rectangle RectCenterDim(Vector Center, Vector Dim);
Rectangle RectMinDim(Vector Min, Vector Dim);
bool IsInRectangle(Rectangle R, Vector V);
Vector GetMinCorner(Rectangle R);
Vector GetMaxCorner(Rectangle R);
Vector GetCenter(Rectangle R);