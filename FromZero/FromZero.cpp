#include "FromZero.h"
#include "FromZero_intrinsics.h"

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

static void CanonicalizeCoord(World_Map *World, INT32 TileCount, INT32 *TileMap, INT32 *Tile, float *TileRel)
{
	INT32 Offset = FloorFloatToINT32(*TileRel / World->TileSideInMeters);
	*Tile += Offset;
	*TileRel -= Offset * World->TileSideInMeters;

	assert(*TileRel >= 0);
	assert(*TileRel < World->TileSideInMeters);

	if (*Tile < 0)
	{
		*Tile += TileCount;
		--*TileMap;
	}
	else if (*Tile >= TileCount)
	{
		*Tile -= TileCount;
		++*TileMap;
	}
}

static Canonical_Position CanonicalizePosition(World_Map *World, Canonical_Position Pos)
{
	CanonicalizeCoord(World, World->TilesNbColumns, &Pos.TileMapX, &Pos.TileX, &Pos.TileRelX);
	CanonicalizeCoord(World, World->TilesNbRows, &Pos.TileMapY, &Pos.TileY, &Pos.TileRelY);

	return Pos;
}

static bool WorldIsEmptyAtPixel(World_Map *World, Canonical_Position Pos)
{
	Tile_Map *TileMap = GetTileMap(World, Pos.TileMapX, Pos.TileMapY);
	return TileIsEmpty(World, TileMap, Pos.TileX, Pos.TileY);
}

void GameUpdateAndRencer(/*ThreadContext *Thread,*/ PixelBuffer *Buffer, GameInput *Input, GameMemory *Memory)
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

	World.TileSideInMeters = 1.4f;
	World.TileSideInPixels = 60;
	World.MetersToPixels = static_cast<float>(World.TileSideInPixels) / World.TileSideInMeters;
	
	World.TilesNbRows = NbRows;
	World.TilesNbColumns = NbColumns;

	World.UpperLeftX = -static_cast<float>(World.TileSideInPixels) / 2;
	World.UpperLeftY = 0;

	const float PlayerWidth = World.TileSideInMeters * 0.65f;
	const float PlayerHeight = World.TileSideInMeters * 0.65f;

	TileMaps[0][0].Tiles = Tiles00;
	TileMaps[0][1].Tiles = Tiles10;
	TileMaps[1][0].Tiles = Tiles01;
	TileMaps[1][1].Tiles = Tiles11;

	GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	if (!Memory->bIsInitialized)
	{
		State->PlayerPosition.TileMapX = 0;
		State->PlayerPosition.TileMapY = 0;
		State->PlayerPosition.TileX = 3;
		State->PlayerPosition.TileY = 3;
		State->PlayerPosition.TileRelX = 5.f / World.MetersToPixels;
		State->PlayerPosition.TileRelY = 5.f / World.MetersToPixels;

		Memory->bIsInitialized = true;
	}

	Tile_Map *CurrentTileMap = GetTileMap(&World, State->PlayerPosition.TileMapX, State->PlayerPosition.TileMapY);
	assert(CurrentTileMap);

	float dPlayerX = 0.0f;
	float dPlayerY = 0.0f;
	float PlayerSpeed = 6.0f;

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

	Canonical_Position NewPlayerPosition = State->PlayerPosition;
	NewPlayerPosition.TileRelX += dPlayerX * Input->TimeElapsingOverFrame;
	NewPlayerPosition.TileRelY += dPlayerY * Input->TimeElapsingOverFrame;
	NewPlayerPosition = CanonicalizePosition(&World, NewPlayerPosition);

	Canonical_Position NewPosLeft = NewPlayerPosition;
	NewPosLeft.TileRelX -= PlayerWidth / 2;
	NewPosLeft = CanonicalizePosition(&World, NewPosLeft);

	Canonical_Position NewPosRight = NewPlayerPosition;
	NewPosRight.TileRelX += PlayerWidth / 2;
	NewPosRight = CanonicalizePosition(&World, NewPosRight);

	if (WorldIsEmptyAtPixel(&World, NewPosLeft) &&
		WorldIsEmptyAtPixel(&World, NewPosRight) &&
		WorldIsEmptyAtPixel(&World, NewPlayerPosition))
	{
		State->PlayerPosition = NewPlayerPosition;
	}

	DrawRectangle(Buffer, 0, 0, static_cast<float>(Buffer->BitmapWidth), static_cast<float>(Buffer->BitmapHeight), 1, 0, 1);

	for (int Row = 0; Row < World.TilesNbRows; ++Row)
	{
		for (int Column = 0; Column < World.TilesNbColumns; ++Column)
		{
			float Color = GetTileValue(&World, CurrentTileMap, Column, Row) == 1 ? 1.f : 0.5f;
			if (Row == State->PlayerPosition.TileY && Column == State->PlayerPosition.TileX)
				Color = 0.f;
			float MinX = World.UpperLeftX + static_cast<float>(Column) * World.TileSideInPixels;
			float MinY = World.UpperLeftY + static_cast<float>(Row) * World.TileSideInPixels;
			DrawRectangle(Buffer, MinX, MinY, MinX + World.TileSideInPixels, MinY + World.TileSideInPixels, Color, Color, Color);
		}
	}

	float PlayerR = 1.0f;
	float PlayerG = 1.0f;
	float PlayerB = 0.0f;
	float PlayerLeft = World.UpperLeftX + World.TileSideInPixels * State->PlayerPosition.TileX + World.MetersToPixels * State->PlayerPosition.TileRelX - 0.5f * PlayerWidth * World.MetersToPixels;
	float PlayerTop = World.UpperLeftY + World.TileSideInPixels * State->PlayerPosition.TileY + World.MetersToPixels * State->PlayerPosition.TileRelY - PlayerHeight * World.MetersToPixels;
	DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + PlayerWidth * World.MetersToPixels, PlayerTop + PlayerHeight * World.MetersToPixels, PlayerR, PlayerG, PlayerB);
}

void GameGetSoundSamples(/*ThreadContext *Thread,*/ SoundBuffer *SBuffer/*, GameMemory *Memory*/)
{
	//GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	OutputSound(SBuffer, 400);
}
