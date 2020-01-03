#include "Vector.h"
#include "FromZero_intrinsics.h"

Vector operator* (const float F, const Vector& V)
{
	return Vector(V.X * F, V.Y * F);
}

Vector operator* (const Vector& V, const float F)
{
	return Vector(V.X * F, V.Y * F);
}

float Dot(Vector V1, Vector V2)
{
	return V1.X * V2.X + V1.Y * V2.Y;
}

float NormSq(Vector V)
{
	return Dot(V, V);
}

float Norm(Vector V)
{
	return Fsqrt(Dot(V, V));
}
