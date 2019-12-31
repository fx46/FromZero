#pragma once

struct Vector
{
	float X, Y;
	float &operator[](int Index) { return((&X)[Index]); }

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

	Vector operator*(float F)
	{
		Vector Result;
		Result.X = F * X;
		Result.Y = F * Y;

		return Result;
	}

	void operator*=(float F)
	{
		X *= F;
		Y *= F;
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
};

float Dot(Vector V1, Vector V2);
