#pragma once

struct Vector
{
	float X, Y;
	float &operator[](int Index) { return((&X)[Index]); }

	Vector(float X, float Y) : X(X), Y(Y) {	}
	Vector() : X(0.f), Y(0.f) {	}

	Vector operator-()
	{
		Vector Result;
		Result.X = -X;
		Result.Y = -Y;

		return Result;
	}

	Vector operator-(Vector V)
	{
		Vector Result;
		Result.X = X - V.X;
		Result.Y = Y - V.Y;

		return Result;
	}

	Vector operator+(Vector V)
	{
		Vector Result;
		Result.X = V.X + X;
		Result.Y = V.Y + Y;

		return Result;
	}

	void operator+=(Vector V)
	{
		X += V.X;
		Y += V.Y;
	}

	void operator-=(Vector V)
	{
		X -= V.X;
		Y -= V.Y;
	}

	void operator*=(float F)
	{
		X *= F;
		Y *= F;
	}

	void operator/=(float F)
	{
		X /= F;
		Y /= F;
	}
};

Vector operator* (const float F, const Vector& V);
Vector operator* (const Vector& V, const float F);
float Dot(Vector V1, Vector V2);
float NormSq(Vector V);
float Norm(Vector V);
