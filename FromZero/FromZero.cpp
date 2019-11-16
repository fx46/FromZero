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

static void InitializeArena(Memory_Arena *Arena, size_t Size, UINT8 *Base)
{
	Arena->Size = Size;
	Arena->Base = Base;
	Arena->Used = 0;
}

static void * PushSize(Memory_Arena *Arena, size_t Size)
{
	assert(Arena->Used + Size <= Arena->Size)
	Arena->Used += Size;

	return Arena->Base + Arena->Used;
}

static void * PushArray(Memory_Arena *Arena, size_t Size, UINT32 Count)
{
	return PushSize(Arena, Size * Count);
}

void GameUpdateAndRencer(/*ThreadContext *Thread,*/ PixelBuffer *Buffer, GameInput *Input, GameMemory *Memory)
{
	assert(sizeof(GameState) <= Memory->PermanentStorageSize);

	GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	if (!Memory->bIsInitialized)
	{
		State->PlayerPosition.AbsTileX = 11;
		State->PlayerPosition.AbsTileY = 11;
		State->PlayerPosition.TileRelX = 5.f;
		State->PlayerPosition.TileRelY = 5.f;

		InitializeArena(&State->WorldArena, Memory->PermanentStorageSize - sizeof(GameState), 
			reinterpret_cast<UINT8 *>(Memory->PermanentStorage) + sizeof(GameState));

		// World construction
		State->World = reinterpret_cast<World_Map *>(PushSize(&State->WorldArena, sizeof(World_Map)));
		State->World->TileMap = reinterpret_cast<Tile_Map *>(PushSize(&State->WorldArena, sizeof(Tile_Map)));

		// using 256x256 tile chunks
		State->World->TileMap->ChunkShift = 8;
		State->World->TileMap->ChunkMask = (1 << State->World->TileMap->ChunkShift) - 1;
		State->World->TileMap->ChunkDimension = 1 << State->World->TileMap->ChunkShift;

		State->World->TileMap->TileChunkCountX = 4;
		State->World->TileMap->TileChunkCountY = 4;

		State->World->TileMap->TileChunks
			= reinterpret_cast<Tile_Chunk *>(PushArray(&State->WorldArena, sizeof(Tile_Chunk), State->World->TileMap->TileChunkCountX * State->World->TileMap->TileChunkCountY));

		for (UINT32 Y = 0; Y < State->World->TileMap->TileChunkCountY; ++Y)
		{
			for (UINT32 X = 0; X < State->World->TileMap->TileChunkCountX; ++X)
			{
				State->World->TileMap->TileChunks[Y * State->World->TileMap->TileChunkCountX + X].Tiles
					= reinterpret_cast<UINT32 *>(PushArray(&State->WorldArena, sizeof(UINT32), State->World->TileMap->ChunkDimension * State->World->TileMap->ChunkDimension));
			}
		}

		State->World->TileMap->TileSideInMeters = 1.4f;
		State->World->TileMap->TileSideInPixels = 60;
		State->World->TileMap->MetersToPixels = static_cast<float>(State->World->TileMap->TileSideInPixels) / State->World->TileMap->TileSideInMeters;


		//float LowerLeftX = -static_cast<float>(World.TileSideInPixels) / 2;
		//float LowerLeftY = static_cast<float>(Buffer->BitmapHeight);


		UINT32 TilesPerWidth = 17;
		UINT32 TilesPerHeight = 9;
		for (UINT32 ScreenY = 0; ScreenY < 32; ++ScreenY)
		{
			for (UINT32 ScreenX = 0; ScreenX < 32; ++ScreenX)
			{
				for (UINT32 TileY = 0; TileY < TilesPerHeight; ++TileY)
				{
					for (UINT32 TileX = 0; TileX < TilesPerWidth; ++TileX)
					{
						UINT32 AbsTileX = ScreenX * TilesPerWidth + TileX;
						UINT32 AbsTileY = ScreenY * TilesPerHeight + TileY;
						SetTileValue(&State->WorldArena, State->World->TileMap, AbsTileX, AbsTileY, 0);
					}
				}
			}
		}

		Memory->bIsInitialized = true;
	}

	const float PlayerWidth = State->World->TileMap->TileSideInMeters * 0.65f;
	const float PlayerHeight = State->World->TileMap->TileSideInMeters * 0.65f;

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
	NewPlayerPosition = CanonicalizePosition(State->World->TileMap, NewPlayerPosition);

	TileMap_Position NewPosLeft = NewPlayerPosition;
	NewPosLeft.TileRelX -= PlayerWidth / 2;
	NewPosLeft = CanonicalizePosition(State->World->TileMap, NewPosLeft);

	TileMap_Position NewPosRight = NewPlayerPosition;
	NewPosRight.TileRelX += PlayerWidth / 2;
	NewPosRight = CanonicalizePosition(State->World->TileMap, NewPosRight);

	if (WorldIsEmptyAtPosition(State->World->TileMap, NewPosLeft) &&
		WorldIsEmptyAtPosition(State->World->TileMap, NewPosRight) &&
		WorldIsEmptyAtPosition(State->World->TileMap, NewPlayerPosition))
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

			float Color = GetTileValue(State->World->TileMap, Column, Row) == 1 ? 1.f : 0.5f;
			if (Row == State->PlayerPosition.AbsTileY && Column == State->PlayerPosition.AbsTileX)
				Color = 0.f;
			float CenterX = 0.5f * Buffer->BitmapWidth - State->World->TileMap->MetersToPixels * State->PlayerPosition.TileRelX + static_cast<float>(RelColumn) * State->World->TileMap->TileSideInPixels;
			float CenterY = 0.5f * Buffer->BitmapHeight + State->World->TileMap->MetersToPixels * State->PlayerPosition.TileRelY - static_cast<float>(RelRow) * State->World->TileMap->TileSideInPixels;
			float MinX = CenterX - State->World->TileMap->TileSideInPixels / 2;
			float MinY = CenterY - State->World->TileMap->TileSideInPixels / 2;
			float MaxX = CenterX + State->World->TileMap->TileSideInPixels / 2;
			float MaxY = CenterY + State->World->TileMap->TileSideInPixels / 2;
			DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Color, Color, Color);
		}
	}

	float PlayerR = 1.0f;
	float PlayerG = 1.0f;
	float PlayerB = 0.0f;
	float PlayerLeft = 0.5f * Buffer->BitmapWidth - 0.5f * PlayerWidth * State->World->TileMap->MetersToPixels;
	float PlayerTop = 0.5f * Buffer->BitmapHeight - PlayerHeight * State->World->TileMap->MetersToPixels;
	DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + PlayerWidth * State->World->TileMap->MetersToPixels, PlayerTop + PlayerHeight * State->World->TileMap->MetersToPixels, PlayerR, PlayerG, PlayerB);
}

void GameGetSoundSamples(/*ThreadContext *Thread,*/ SoundBuffer *SBuffer/*, GameMemory *Memory*/)
{
	//GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	OutputSound(SBuffer, 400);
}
