#include "FromZero.h"
#include "math.h"

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

	if (TileMapX >= 0 && TileMapX < World->NbTileMapsColumns && TileMapY >= 0 && TileMapY < World->NbTileMapsRows)
	{
		TileMap = &World->TileMaps[TileMapY * World->NbTileMapsColumns + TileMapX];
	}

	return TileMap;
}

static UINT32 GetTileValue(World_Map *World, Tile_Map *TileMap, int X, int Y)
{
	assert(TileMap)
	assert(X >= 0 && X < World->TilesNbColumns && Y >= 0 && Y < World->TilesNbRows)
	
	return TileMap->Tiles[Y * World->TilesNbColumns + X];
}

static bool TileIsEmpty(World_Map *World, Tile_Map *TileMap, int TestTileX, int TestTileY)
{
	if (!TileMap) return false;

	return TestTileX >= 0
		&& TestTileX < World->TilesNbColumns
		&& TestTileY >= 0
		&& TestTileY < World->TilesNbRows
		&& GetTileValue(World, TileMap, TestTileX, TestTileY) == 0;
}

static Canonical_Position GetWrappedWorldLocation(World_Map *World, Raw_Position Pos)
{
	Canonical_Position Result;
	Result.TileMapX = Pos.TileMapX;
	Result.TileMapY= Pos.TileMapY;

	float X = Pos.X - World->UpperLeftX;
	float Y = Pos.Y - World->UpperLeftY;
	Result.TileX = static_cast<int>(floor(X / World->TileWidth));
	Result.TileY = static_cast<int>(floor(Y / World->TileHeight));
	Result.TileRelX = X - Result.TileX * World->TileWidth;
	Result.TileRelY = Y - Result.TileY * World->TileHeight;

	assert(Result.TileRelX >= 0)
	assert(Result.TileRelY >= 0)
	assert(Result.TileRelX < World->TileWidth)
	assert(Result.TileRelY < World->TileHeight)

	if (Result.TileX < 0)
	{
		Result.TileX += World->TilesNbColumns;
		--Result.TileMapX;
	}
	else if (Result.TileX >= World->TilesNbColumns)
	{
		Result.TileX -= World->TilesNbColumns;
		++Result.TileMapX;
	}
	if (Result.TileY < 0)
	{
		Result.TileY += World->TilesNbRows;
		--Result.TileMapY;
	}
	else if (Result.TileY >= World->TilesNbRows)
	{
		Result.TileY -= World->TilesNbRows;
		++Result.TileMapY;
	}

	return Result;
}

static bool WorldIsEmptyAtPixel(World_Map *World, Raw_Position Pos)
{
	Canonical_Position WrappedLocation = GetWrappedWorldLocation(World, Pos);
	Tile_Map *TileMap = GetTileMap(World, WrappedLocation.TileMapX, WrappedLocation.TileMapY);
	return TileIsEmpty(World, TileMap, WrappedLocation.TileX, WrappedLocation.TileY);
}

void GameUpdateAndRencer(ThreadContext *Thread, PixelBuffer *Buffer, GameInput *Input, GameMemory *Memory)
{
	assert(sizeof(GameState) <= Memory->PermanentStorageSize);

	const int NbRows	= 9;
	const int NbColumns = 17;

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
		1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1,
		1, 0, 0, 0,  0, 0, 1, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1,
		1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0, 1,
		1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 1, 1,
		1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 0, 1, 0, 0,
		1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1,
		1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1,
		1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1,
		1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1
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

	Tile_Map TileMaps[2][2];

	World_Map World;
	World.TileMaps = reinterpret_cast<Tile_Map *>(TileMaps);
	World.NbTileMapsColumns = 2;
	World.NbTileMapsRows = 2;
	
	World.TilesNbRows = NbRows;
	World.TilesNbColumns = NbColumns;

	World.UpperLeftX =-30;
	World.UpperLeftY = 0;
	World.TileWidth	 = 60;
	World.TileHeight = 60;

	const float PlayerWidth = World.TileWidth * 0.75f;
	const float PlayerHeight = World.TileHeight * 0.75f;

	TileMaps[0][0].Tiles = Tiles00;
	TileMaps[0][1].Tiles = Tiles10;
	TileMaps[1][0].Tiles = Tiles01;
	TileMaps[1][1].Tiles = Tiles11;

	GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	if (!Memory->bIsInitialized)
	{
		State->PlayerX = 100.f;
		State->PlayerY = 290.f;
		Memory->bIsInitialized = true;
	}

	Tile_Map *CurrentTileMap = GetTileMap(&World, State->PlayerTileMapX, State->PlayerTileMapY);
	assert(CurrentTileMap);

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

	Raw_Position Pos = { State->PlayerTileMapX, State->PlayerTileMapY, NewPlayerX, NewPlayerY };
	Raw_Position PosLeft = Pos;
	PosLeft.X -= PlayerWidth / 2;
	Raw_Position PosRight = Pos;
	PosRight.X += PlayerWidth / 2;

	if (WorldIsEmptyAtPixel(&World, PosLeft) &&
		WorldIsEmptyAtPixel(&World, PosRight) &&
		WorldIsEmptyAtPixel(&World, Pos))
	{
		Canonical_Position CanPos = GetWrappedWorldLocation(&World, Pos);
		State->PlayerTileMapX = CanPos.TileMapX;
		State->PlayerTileMapY = CanPos.TileMapY;
		State->PlayerX = World.UpperLeftX + World.TileWidth * CanPos.TileX + CanPos.TileRelX;
		State->PlayerY = World.UpperLeftY + World.TileHeight * CanPos.TileY + CanPos.TileRelY;
	}

	DrawRectangle(Buffer, 0, 0, static_cast<float>(Buffer->BitmapWidth), static_cast<float>(Buffer->BitmapHeight), 1, 0, 1);

	for (int Row = 0; Row < World.TilesNbRows; ++Row)
	{
		for (int Colomn = 0; Colomn < World.TilesNbColumns; ++Colomn)
		{
			float Color = GetTileValue(&World, CurrentTileMap, Colomn, Row) == 1 ? 1.f : 0.5f;
			float MinX = World.UpperLeftX + static_cast<float>(Colomn) * World.TileWidth;
			float MinY = World.UpperLeftY + static_cast<float>(Row) * World.TileHeight;
			DrawRectangle(Buffer, MinX, MinY, MinX + World.TileWidth, MinY + World.TileHeight, Color, Color, Color);
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
