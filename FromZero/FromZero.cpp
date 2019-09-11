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

static Tile_Map * GetTileMap(World_Map *World, int TileMapX, int TileMapY)
{
	Tile_Map *TileMap = nullptr;

	if (TileMapX >= 0 && TileMapX < World->NbColumns && TileMapY >= 0 && TileMapY < World->NbRows)
	{
		TileMap = &World->TileMaps[TileMapY * World->NbColumns + TileMapX];
	}

	return TileMap;
}

static UINT32 GetTileValue(Tile_Map *TileMap, int TileX, int TileY)
{
	return TileMap->Tiles[TileY * TileMap->NbColumns + TileX];
}

static bool WorldIsEmptyAtPixel(World_Map *World, INT32 TileMapX, INT32 TileMapY, float X, float Y)
{
	if (Tile_Map *TileMap = GetTileMap(World, TileMapX, TileMapY))
	{
		const int PlayerTileX = static_cast<int>((X - TileMap->UpperLeftX) / TileMap->TileWidth);
		const int PlayerTileY = static_cast<int>((Y - TileMap->UpperLeftY) / TileMap->TileHeight);

		return PlayerTileX >= 0
			&& PlayerTileX < TileMap->NbColumns
			&& PlayerTileY >= 0
			&& PlayerTileY < TileMap->NbRows
			&& GetTileValue(TileMap, PlayerTileX, PlayerTileY) == 0;
	}

	return false;
}

static bool TileIsEmptyAtPixel(Tile_Map *TileMap, float X, float Y)
{
	const int PlayerTileX = static_cast<int>((X - TileMap->UpperLeftX) / TileMap->TileWidth);
	const int PlayerTileY = static_cast<int>((Y - TileMap->UpperLeftY) / TileMap->TileHeight);

	return PlayerTileX >= 0 
		&& PlayerTileX < TileMap->NbColumns 
		&& PlayerTileY >= 0 
		&& PlayerTileY < TileMap->NbRows 
		&& GetTileValue(TileMap, PlayerTileX, PlayerTileY) == 0;
}

void GameUpdateAndRencer(ThreadContext *Thread, PixelBuffer *Buffer, GameInput *Input, GameMemory *Memory)
{
	assert(sizeof(GameState) <= Memory->PermanentStorageSize);

	Tile_Map TileMaps[2][2];

	const int NbRows	= 9;
	const int NbColumns = 17;
	TileMaps[0][0].NbRows		= NbRows;
	TileMaps[0][0].NbColumns	= NbColumns;
	UINT32 Tiles00[NbRows * NbColumns] = {
		1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1,
		1, 0, 0, 0,  0, 0, 1, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1,
		1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0, 1,
		1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 1, 1,
		1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 0, 1, 0, 0,
		1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1,
		1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1,
		1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1,
		1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1
	};

	UINT32 Tiles01[NbRows * NbColumns] = {
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1,
	1, 0, 0, 0,  0, 0, 1, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1,
	1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0, 1,
	1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 1, 1,
	1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 0, 1, 0, 0,
	1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1,
	1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1,
	1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1,
	1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1
	};

	UINT32 Tiles10[NbRows * NbColumns] = {
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1,
	1, 0, 0, 0,  0, 0, 1, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1,
	1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0, 1,
	1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 1, 1,
	0, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 0, 1, 0, 1,
	1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1,
	1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1,
	1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1,
	1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1
	};

	UINT32 Tiles11[NbRows * NbColumns] = {
	1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1,
	1, 0, 0, 0,  0, 0, 1, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1,
	1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0, 1,
	1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 1, 1,
	0, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 0, 1, 0, 1,
	1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1,
	1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1,
	1, 1, 1, 1,  1, 0, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1,
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1
	};

	TileMaps[0][0].Tiles		= Tiles00;
	TileMaps[0][0].UpperLeftX	= -30;
	TileMaps[0][0].UpperLeftY	= 0;
	TileMaps[0][0].TileWidth	= 60;
	TileMaps[0][0].TileHeight	= 60;

	TileMaps[0][1] = TileMaps[0][0];
	TileMaps[0][1].Tiles = Tiles01;

	TileMaps[1][0] = TileMaps[0][0];
	TileMaps[1][0].Tiles = Tiles10;

	TileMaps[1][1] = TileMaps[0][0];
	TileMaps[1][1].Tiles = Tiles11;

	Tile_Map *CurrentTileMap = &TileMaps[0][0];

	World_Map World;
	World.TileMaps = reinterpret_cast<Tile_Map *>(TileMaps);

	const float PlayerWidth = CurrentTileMap->TileWidth * 0.75;
	const float PlayerHeight = CurrentTileMap->TileHeight * 0.75;

	GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	if (!Memory->bIsInitialized)
	{
		State->PlayerX = 100.f;
		State->PlayerY = 290.f;
		Memory->bIsInitialized = true;
	}

	float dPlayerX = 0.0f;
	float dPlayerY = 0.0f;
	float PlayerSpeed = 128.0f; // pixels/seconds

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

	if (TileIsEmptyAtPixel(CurrentTileMap, NewPlayerX - PlayerWidth / 2, NewPlayerY) &&
		TileIsEmptyAtPixel(CurrentTileMap, NewPlayerX + PlayerWidth / 2, NewPlayerY) &&
		TileIsEmptyAtPixel(CurrentTileMap, NewPlayerX, NewPlayerY))
	{
		State->PlayerX = NewPlayerX;
		State->PlayerY = NewPlayerY;
	}

	DrawRectangle(Buffer, 0, 0, static_cast<float>(Buffer->BitmapWidth), static_cast<float>(Buffer->BitmapHeight), 1, 0, 1);

	for (int Row = 0; Row < CurrentTileMap->NbRows; ++Row)
	{
		for (int Colomn = 0; Colomn < CurrentTileMap->NbColumns; ++Colomn)
		{
			float Color = GetTileValue(CurrentTileMap, Colomn, Row) == 1 ? 1.f : 0.5f;
			float MinX = CurrentTileMap->UpperLeftX + static_cast<float>(Colomn) * CurrentTileMap->TileWidth;
			float MinY = CurrentTileMap->UpperLeftY + static_cast<float>(Row) * CurrentTileMap->TileHeight;
			DrawRectangle(Buffer, MinX, MinY, MinX + CurrentTileMap->TileWidth, MinY + CurrentTileMap->TileHeight, Color, Color, Color);
		}
	}

	float PlayerR = 1.0f;
	float PlayerG = 1.0f;
	float PlayerB = 0.0f;
	float PlayerLeft = State->PlayerX - 0.5f * PlayerWidth;
	float PlayerTop = State->PlayerY - PlayerHeight;
	DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + PlayerWidth, PlayerTop + PlayerHeight, PlayerR, PlayerG, PlayerB);
}

void GameGetSoundSamples(ThreadContext *Thread, SoundBuffer *SBuffer, GameMemory *Memory)
{
	GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	OutputSound(SBuffer, 400);
}
