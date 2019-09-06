#include "FromZero.h"

static void OutputSound(SoundBuffer *Buffer, int ToneHz)
{
	static float TSine;
	//INT16 ToneVolume = 3000;
	INT16 *SampleOut = Buffer->Samples;
	int WavePeriod = Buffer->SamplesPerSecond / ToneHz;
	const float Tau = 2.0f * 3.14159265359f;

	for (int SampleIndex = 0; SampleIndex < Buffer->SampleCountToOutput; SampleIndex++)
	{
		//float SineValue = sinf(TSine);
		INT16 SampleValue = 0; // static_cast<INT16>(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		TSine += Tau / static_cast<float>(WavePeriod);
		if (TSine > Tau)
		{
			TSine -= Tau;
		}
	}
}

static INT32 RoundFloatToINT32(float Real32)
{
	return static_cast<INT32>(Real32 + 0.5f);
}

static UINT32 RoundFloatToUINT32(float Real32)
{
	return static_cast<UINT32>(Real32 + 0.5f);
}

static void DrawRectangle(PixelBuffer *Buffer, float MinXfloat, float MinYfloat, float MaxXfloat,  float MaxYfloat, float R, float G, float B)
{
	INT32 MinX = RoundFloatToINT32(MinXfloat);
	INT32 MaxX = RoundFloatToINT32(MaxXfloat);
	INT32 MinY = RoundFloatToINT32(MinYfloat);
	INT32 MaxY = RoundFloatToINT32(MaxYfloat);

	if (MinX < 0)
	{
		MinX = 0;
	}

	if (MinY < 0)
	{
		MinY = 0;
	}

	if (MaxX > Buffer->BitmapWidth)
	{
		MaxX = Buffer->BitmapWidth;
	}

	if (MaxY > Buffer->BitmapHeight)
	{
		MaxY = Buffer->BitmapHeight;
	}

	//Bit pattern: 0x AA RR GG BB
	UINT32 Color = RoundFloatToUINT32(R * 255.f) << 16 | RoundFloatToUINT32(G * 255.f) << 8 | RoundFloatToUINT32(B * 255.f);

	UINT32 *Pixel = static_cast<UINT32 *>(Buffer->BitmapMemory) + MinX + MinY * Buffer->Pitch / Buffer->BytesPerPixel;
	const int NbPixelsBetweenRows = (Buffer->Pitch / Buffer->BytesPerPixel) - (MaxX - MinX);

	for (int Y = MinY; Y < MaxY; ++Y)
	{
		for (int X = MinX; X < MaxX; ++X)
		{
			*Pixel++ = Color;
		}
		Pixel += NbPixelsBetweenRows;
	}
}

void GameUpdateAndRencer(ThreadContext *Thread, PixelBuffer *Buffer, GameInput *Input, GameMemory *Memory)
{
	assert(sizeof(GameState) <= Memory->PermanentStorageSize);

	GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	if (!Memory->bIsInitialized)
	{
		Memory->bIsInitialized = true;
	}

	const int NbTileMapRows = 9;
	const int NbTileMapColumns = 16;

	UINT32 TileMap[NbTileMapRows][NbTileMapColumns] =
	{
		{1, 1, 1, 1,  0, 0, 1, 0,  1, 0, 0, 0,  1, 0, 0, 0},
		{0, 0, 0, 0,  0, 0, 1, 0,  1, 0, 0, 0,  0, 1, 0, 0},
		{0, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0},
		{0, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 1},
		{0, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0},
		{0, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0},
		{0, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  1, 0, 0, 0},
		{0, 1, 1, 1,  1, 0, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0},
		{0, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0}
	};

	DrawRectangle(Buffer, 0, 0, static_cast<float>(Buffer->BitmapWidth), static_cast<float>(Buffer->BitmapHeight), 1, 0, 1);

	const float UpperLeftX = 10;
	const float UpperLeftY = 10;
	const float TileWidth = 50;
	const float TileHeight = 50;

	for (int Row = 0; Row < NbTileMapRows; ++Row)
	{
		for (int Colomn = 0; Colomn < NbTileMapColumns; ++Colomn)
		{
			float Color = TileMap[Row][Colomn] == 1 ? 10.f : 0.5f;
			float MinX = UpperLeftX + static_cast<float>(Colomn) * TileWidth;
			float MinY = UpperLeftY + static_cast<float>(Row) * TileHeight;
			DrawRectangle(Buffer, MinX, MinY, MinX + TileWidth, MinY + TileHeight, Color, Color, Color);
		}
	}
}

void GameGetSoundSamples(ThreadContext *Thread, SoundBuffer *SBuffer, GameMemory *Memory)
{
	GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	OutputSound(SBuffer, 400);
}
