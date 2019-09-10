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
		State->PlayerX = 100.f;
		State->PlayerY = 100.f;
		Memory->bIsInitialized = true;
	}

	const float UpperLeftX = -30;
	const float UpperLeftY = 0;
	const float TileWidth = 60;
	const float TileHeight = 60;
	const int NbTileMapRows = 9;
	const int NbTileMapColumns = 17;
	UINT32 TileMap[NbTileMapRows][NbTileMapColumns] =
	{
		{1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1},
		{1, 0, 0, 0,  0, 0, 1, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 1, 1},
		{0, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 0, 1, 0, 0},
		{1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1},
		{1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1},
		{1, 1, 1, 1,  1, 0, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1},
		{1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1}
	};

	float dPlayerX = 0.0f;
	float dPlayerY = 0.0f;
	float PlayerSpeed = 128.0f; //pixels/seconds

	if (Input->A)
	{
		dPlayerX -= PlayerSpeed;
	}
	if (Input->D)
	{
		dPlayerX += PlayerSpeed;
	}
	if (Input->W)
	{
		dPlayerY -= PlayerSpeed;
	}
	if (Input->S)
	{
		dPlayerY += PlayerSpeed;
	}

	const float NewPlayerX = State->PlayerX + dPlayerX * Input->TimeElapsingOverFrame;
	const float NewPlayerY = State->PlayerY + dPlayerY * Input->TimeElapsingOverFrame;

	const int PlayerTileX = (NewPlayerX - UpperLeftX) / TileWidth;
	const int PlayerTileY = (NewPlayerY - UpperLeftY) / TileHeight;

	bool bIsMovementValid = false;
	if (PlayerTileX >= 0 && PlayerTileX < NbTileMapColumns && PlayerTileY >= 0 && PlayerTileY < NbTileMapRows)
	{
		bIsMovementValid = TileMap[PlayerTileY][PlayerTileX] == 0;
	}

	if (bIsMovementValid)
	{
		State->PlayerX = NewPlayerX;
		State->PlayerY = NewPlayerY;
	}

	DrawRectangle(Buffer, 0, 0, static_cast<float>(Buffer->BitmapWidth), static_cast<float>(Buffer->BitmapHeight), 1, 0, 1);

	for (int Row = 0; Row < NbTileMapRows; ++Row)
	{
		for (int Colomn = 0; Colomn < NbTileMapColumns; ++Colomn)
		{
			float Color = TileMap[Row][Colomn] == 1 ? 1.f : 0.5f;
			float MinX = UpperLeftX + static_cast<float>(Colomn) * TileWidth;
			float MinY = UpperLeftY + static_cast<float>(Row) * TileHeight;
			DrawRectangle(Buffer, MinX, MinY, MinX + TileWidth, MinY + TileHeight, Color, Color, Color);
		}
	}

	float PlayerR = 1.0f;
	float PlayerG = 1.0f;
	float PlayerB = 0.0f;
	float PlayerWidth = TileWidth * 0.75;
	float PlayerHeight = TileHeight * 0.75;
	float PlayerLeft = State->PlayerX - 0.5f * PlayerWidth;
	float PlayerTop = State->PlayerY - PlayerHeight;
	DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + PlayerWidth, PlayerTop + PlayerHeight, PlayerR, PlayerG, PlayerB);
}

void GameGetSoundSamples(ThreadContext *Thread, SoundBuffer *SBuffer, GameMemory *Memory)
{
	GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	OutputSound(SBuffer, 400);
}
