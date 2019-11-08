#include "FromZero.h"
#include "FromZero_intrinsics.h"
#include "FromZero_TileMap.h"

#include "FromZero_TileMap.cpp"

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

	Tile_Map TileMap;
	World_Map World;
	World.TileMap = &TileMap;

	// using 256x256 tile chunks
	World.TileMap->ChunkShift = 8;
	World.TileMap->ChunkMask = (1 << World.TileMap->ChunkShift) - 1;

	Tile_Chunk TileChunk;
	TileChunk.Tiles = reinterpret_cast<UINT32 *>(Tiles);
	World.TileMap->TileChunks = &TileChunk;
	World.TileMap->TileChunkCountX = 1;
	World.TileMap->TileChunkCountY = 1;

	World.TileMap->TileSideInMeters = 1.4f;
	World.TileMap->TileSideInPixels = 60;
	World.TileMap->MetersToPixels = static_cast<float>(World.TileMap->TileSideInPixels) / World.TileMap->TileSideInMeters;
	
	World.TileMap->ChunkDimension = 256;

	//float LowerLeftX = -static_cast<float>(World.TileSideInPixels) / 2;
	//float LowerLeftY = static_cast<float>(Buffer->BitmapHeight);

	const float PlayerWidth = World.TileMap->TileSideInMeters * 0.65f;
	const float PlayerHeight = World.TileMap->TileSideInMeters * 0.65f;

	GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	if (!Memory->bIsInitialized)
	{
		State->PlayerPosition.AbsTileX = 3;
		State->PlayerPosition.AbsTileY = 3;
		State->PlayerPosition.TileRelX = 5.f / World.TileMap->MetersToPixels;
		State->PlayerPosition.TileRelY = 5.f / World.TileMap->MetersToPixels;

		Memory->bIsInitialized = true;
	}

	float dPlayerX = 0.0f;
	float dPlayerY = 0.0f;
	float PlayerSpeed = 6.0f;

	if (Input->Shift)
	{
		PlayerSpeed *= 1.5f;
	}

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


	TileMap_Position NewPlayerPosition = State->PlayerPosition;
	NewPlayerPosition.TileRelX += dPlayerX * Input->TimeElapsingOverFrame;
	NewPlayerPosition.TileRelY += dPlayerY * Input->TimeElapsingOverFrame;
	NewPlayerPosition = CanonicalizePosition(World.TileMap, NewPlayerPosition);

	TileMap_Position NewPosLeft = NewPlayerPosition;
	NewPosLeft.TileRelX -= PlayerWidth / 2;
	NewPosLeft = CanonicalizePosition(World.TileMap, NewPosLeft);

	TileMap_Position NewPosRight = NewPlayerPosition;
	NewPosRight.TileRelX += PlayerWidth / 2;
	NewPosRight = CanonicalizePosition(World.TileMap, NewPosRight);

	if (WorldIsEmptyAtPosition(World.TileMap, NewPosLeft) &&
		WorldIsEmptyAtPosition(World.TileMap, NewPosRight) &&
		WorldIsEmptyAtPosition(World.TileMap, NewPlayerPosition))
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

			float Color = GetTileValue(World.TileMap, Column, Row) == 1 ? 1.f : 0.5f;
			if (Row == State->PlayerPosition.AbsTileY && Column == State->PlayerPosition.AbsTileX)
				Color = 0.f;
			float CenterX = 0.5f * Buffer->BitmapWidth - World.TileMap->MetersToPixels * State->PlayerPosition.TileRelX + static_cast<float>(RelColumn) * World.TileMap->TileSideInPixels;
			float CenterY = 0.5f * Buffer->BitmapHeight + World.TileMap->MetersToPixels * State->PlayerPosition.TileRelY - static_cast<float>(RelRow) * World.TileMap->TileSideInPixels;
			float MinX = CenterX - World.TileMap->TileSideInPixels / 2;
			float MinY = CenterY - World.TileMap->TileSideInPixels / 2;
			float MaxX = CenterX + World.TileMap->TileSideInPixels / 2;
			float MaxY = CenterY + World.TileMap->TileSideInPixels / 2;
			DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Color, Color, Color);
		}
	}

	float PlayerR = 1.0f;
	float PlayerG = 1.0f;
	float PlayerB = 0.0f;
	float PlayerLeft = 0.5f * Buffer->BitmapWidth - 0.5f * PlayerWidth * World.TileMap->MetersToPixels;
	float PlayerTop = 0.5f * Buffer->BitmapHeight - PlayerHeight * World.TileMap->MetersToPixels;
	DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + PlayerWidth * World.TileMap->MetersToPixels, PlayerTop + PlayerHeight * World.TileMap->MetersToPixels, PlayerR, PlayerG, PlayerB);
}

void GameGetSoundSamples(/*ThreadContext *Thread,*/ SoundBuffer *SBuffer/*, GameMemory *Memory*/)
{
	//GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	OutputSound(SBuffer, 400);
}
