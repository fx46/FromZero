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

static Tile_Chunk * GetTileChunk(World_Map *World, int TileChunkX, int TileChunkY)
{
	Tile_Chunk *TileChunk = nullptr;

	if (TileChunkX >= 0 && TileChunkX < World->TileChunkCountX && TileChunkY >= 0 && TileChunkY < World->TileChunkCountY)
	{
		TileChunk = &World->TileChunks[TileChunkY * World->ChunkDimension + TileChunkX];
	}

	return TileChunk;
}

static UINT32 GetTileValueUnchecked(World_Map *World, Tile_Chunk *TileChunk, UINT32 X, UINT32 Y)
{
	assert(TileChunk)
	assert(X < World->ChunkDimension && Y < World->ChunkDimension)
	
	return TileChunk->Tiles[Y * World->ChunkDimension + X];
}

static UINT32 GetTileValue(World_Map *World, Tile_Chunk *TileChunk, UINT32 TestTileX, UINT32 TestTileY)
{
	UINT32 TileChunkValue = 0;

	if (TileChunk)
	{
		TileChunkValue = GetTileValueUnchecked(World, TileChunk, TestTileX, TestTileY);
	}

	return TileChunkValue;
}

static void CanonicalizeCoord(World_Map *World, UINT32 *Tile, float *TileRel)
{
	INT32 Offset = FloorFloatToINT32(*TileRel / World->TileSideInMeters);
	*Tile += Offset;
	*TileRel -= Offset * World->TileSideInMeters;

	assert(*TileRel >= 0);
	assert(*TileRel <= World->TileSideInMeters);
}

static World_Position CanonicalizePosition(World_Map *World, World_Position Pos)
{
	CanonicalizeCoord(World, &Pos.AbsTileX, &Pos.TileRelX);
	CanonicalizeCoord(World, &Pos.AbsTileY, &Pos.TileRelY);

	return Pos;
}

static Tile_Chunk_Position GetChunkPositionFor(World_Map *World, UINT32 AbsTileX, UINT32 AbsTileY)
{
	Tile_Chunk_Position Result;

	Result.TileChunkX = AbsTileX >> World->ChunkShift; // getting the 24 leftmost bits
	Result.TileChunkY = AbsTileY >> World->ChunkShift;
	Result.ChunkRelTileX = AbsTileX & World->ChunkMask; // getting the 8 rightmost bits
	Result.ChunkRelTileY = AbsTileY & World->ChunkMask;

	return Result;
}

static UINT32 GetTileValue(World_Map *World, UINT32 AbsTileX, UINT32 AbsTileY)
{
	Tile_Chunk_Position ChunkPosition = GetChunkPositionFor(World, AbsTileX, AbsTileY);
	Tile_Chunk *TileMap = GetTileChunk(World, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY);
	UINT32 TileChunkValue = GetTileValue(World, TileMap, ChunkPosition.ChunkRelTileX, ChunkPosition.ChunkRelTileY);
	
	return TileChunkValue;
}

static bool WorldIsEmptyAtPosition(World_Map *World, World_Position Pos)
{
	UINT32 TileChunkValue = GetTileValue(World, Pos.AbsTileX, Pos.AbsTileY);
	
	return TileChunkValue == 0;
}

void GameUpdateAndRencer(/*ThreadContext *Thread,*/ PixelBuffer *Buffer, GameInput *Input, GameMemory *Memory)
{
	assert(sizeof(GameState) <= Memory->PermanentStorageSize);

	const int NbRows	= 256;
	const int NbColumns = 256;

	UINT32 Tiles[NbRows][NbColumns] = {
		{1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1,	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1},
		{1, 0, 0, 0,  0, 0, 1, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1,	1, 0, 0, 0,  0, 0, 1, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0, 1,	1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 1, 1,	1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 1, 1},
		{1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 0, 1, 0, 0,	0, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 0, 1, 0, 1},
		{1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1,	1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1},
		{1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1,	1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1},
		{1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1,	1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1},
		{1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1,	1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
																													
		{1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1,	1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
		{1, 0, 0, 0,  0, 0, 1, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1,	1, 0, 0, 0,  0, 0, 1, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0, 1,	1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 1, 1,	1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 1, 1},
		{1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 0, 1, 0, 0,	0, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 0, 1, 0, 1},
		{1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1,	1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1},
		{1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1,	1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1},
		{1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1,	1, 1, 1, 1,  1, 0, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1},
		{1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1,	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1}
	};

	World_Map World;

	// using 256x256 tile chunks
	World.ChunkShift = 8;
	World.ChunkMask = (1 << World.ChunkShift) - 1;

	Tile_Chunk TileChunk;
	TileChunk.Tiles = reinterpret_cast<UINT32 *>(Tiles);
	World.TileChunks = &TileChunk;
	World.TileChunkCountX = 1;
	World.TileChunkCountY = 1;

	World.TileSideInMeters = 1.4f;
	World.TileSideInPixels = 60;
	World.MetersToPixels = static_cast<float>(World.TileSideInPixels) / World.TileSideInMeters;
	
	World.ChunkDimension = 256;

	float LowerLeftX = -static_cast<float>(World.TileSideInPixels) / 2;
	float LowerLeftY = static_cast<float>(Buffer->BitmapHeight);

	const float PlayerWidth = World.TileSideInMeters * 0.65f;
	const float PlayerHeight = World.TileSideInMeters * 0.65f;

	GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	if (!Memory->bIsInitialized)
	{
		State->PlayerPosition.AbsTileX = 3;
		State->PlayerPosition.AbsTileY = 3;
		State->PlayerPosition.TileRelX = 5.f / World.MetersToPixels;
		State->PlayerPosition.TileRelY = 5.f / World.MetersToPixels;

		Memory->bIsInitialized = true;
	}

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
		dPlayerY += PlayerSpeed;
	}
	if (Input->S)
	{
		dPlayerY -= PlayerSpeed;
	}

	World_Position NewPlayerPosition = State->PlayerPosition;
	NewPlayerPosition.TileRelX += dPlayerX * Input->TimeElapsingOverFrame;
	NewPlayerPosition.TileRelY += dPlayerY * Input->TimeElapsingOverFrame;
	NewPlayerPosition = CanonicalizePosition(&World, NewPlayerPosition);

	World_Position NewPosLeft = NewPlayerPosition;
	NewPosLeft.TileRelX -= PlayerWidth / 2;
	NewPosLeft = CanonicalizePosition(&World, NewPosLeft);

	World_Position NewPosRight = NewPlayerPosition;
	NewPosRight.TileRelX += PlayerWidth / 2;
	NewPosRight = CanonicalizePosition(&World, NewPosRight);

	if (WorldIsEmptyAtPosition(&World, NewPosLeft) &&
		WorldIsEmptyAtPosition(&World, NewPosRight) &&
		WorldIsEmptyAtPosition(&World, NewPlayerPosition))
	{
		State->PlayerPosition = NewPlayerPosition;
	}

	DrawRectangle(Buffer, 0, 0, static_cast<float>(Buffer->BitmapWidth), static_cast<float>(Buffer->BitmapHeight), 1, 0, 1);

	for (INT32 RelRow = -6; RelRow < 6; ++RelRow)
	{
		for (INT32 RelColumn = -9; RelColumn < 9; ++RelColumn)
		{
			UINT32 Column = State->PlayerPosition.AbsTileX + RelColumn;
			UINT32 Row = State->PlayerPosition.AbsTileY + RelRow;

			float Color = GetTileValue(&World, Column, Row) == 1 ? 1.f : 0.5f;
			//if (Row == State->PlayerPosition.AbsTileY && Column == State->PlayerPosition.AbsTileX)
			//	Color = 0.f;
			float MinX = 0.5f * Buffer->BitmapWidth - World.MetersToPixels * State->PlayerPosition.TileRelX + static_cast<float>(RelColumn) * World.TileSideInPixels;
			float MinY = 0.5f * Buffer->BitmapHeight + World.MetersToPixels * State->PlayerPosition.TileRelY - static_cast<float>(RelRow) * World.TileSideInPixels;
			DrawRectangle(Buffer, MinX, MinY - World.TileSideInPixels, MinX + World.TileSideInPixels, MinY, Color, Color, Color);
		}
	}

	float PlayerR = 1.0f;
	float PlayerG = 1.0f;
	float PlayerB = 0.0f;
	float PlayerLeft = 0.5f * Buffer->BitmapWidth - 0.5f * PlayerWidth * World.MetersToPixels;
	float PlayerTop = 0.5f * Buffer->BitmapHeight - PlayerHeight * World.MetersToPixels;
	DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + PlayerWidth * World.MetersToPixels, PlayerTop + PlayerHeight * World.MetersToPixels, PlayerR, PlayerG, PlayerB);
}

void GameGetSoundSamples(/*ThreadContext *Thread,*/ SoundBuffer *SBuffer/*, GameMemory *Memory*/)
{
	//GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	OutputSound(SBuffer, 400);
}
