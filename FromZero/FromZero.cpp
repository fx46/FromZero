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

	GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	if (!Memory->bIsInitialized)
	{
		State->PlayerPosition.AbsTileX = 1;
		State->PlayerPosition.AbsTileY = 1;
		State->PlayerPosition.TileRelX = 5.f;
		State->PlayerPosition.TileRelY = 5.f;

		InitializeArena(&State->WorldArena, Memory->PermanentStorageSize - sizeof(GameState), 
			reinterpret_cast<UINT8 *>(Memory->PermanentStorage) + sizeof(GameState));

		// World construction
		State->World = reinterpret_cast<World_Map *>(PushSize(&State->WorldArena, sizeof(World_Map)));
		State->World->TileMap = reinterpret_cast<Tile_Map *>(PushSize(&State->WorldArena, sizeof(Tile_Map)));
		State->World->TileMap->ChunkShift = 4;
		State->World->TileMap->ChunkMask = (1 << State->World->TileMap->ChunkShift) - 1;
		State->World->TileMap->ChunkDimension = 1 << State->World->TileMap->ChunkShift;
		State->World->TileMap->TileChunkCountX = 128;
		State->World->TileMap->TileChunkCountY = 128;
		State->World->TileMap->TileChunkCountZ = 2;
		State->World->TileMap->TileChunks 
			= reinterpret_cast<Tile_Chunk *>(PushArray(&State->WorldArena, 
														sizeof(Tile_Chunk), 
														State->World->TileMap->TileChunkCountX * 
														State->World->TileMap->TileChunkCountY *
														State->World->TileMap->TileChunkCountZ));
		State->World->TileMap->TileSideInMeters = 1.4f;

		UINT32 TilesPerWidth = 17;
		UINT32 TilesPerHeight = 9;
		UINT32 ScreenX = 0;
		UINT32 ScreenY = 0;
		UINT32 AbsTileZ = 0;
		bool bDoorLeft = false;
		bool bDoorRight = false;
		bool bDoorTop = false;
		bool bDoorBottom = false;
		bool bDoorUp = false;
		bool bDoorDown = false;

		for (UINT32 ScreenIndex = 0; ScreenIndex < 20; ++ScreenIndex)
		{
			bDoorLeft = bDoorRight;
			bDoorBottom = bDoorTop;
			bDoorRight = false;
			bDoorTop = false;

			UINT32 RandomChoice;
			if (bDoorUp || bDoorDown)
				RandomChoice  = rand() % 2;
			else
				RandomChoice = rand() % 3;

			if (RandomChoice == 2)
			{
				if (AbsTileZ == 0)
				{
					bDoorUp = true;
					bDoorDown = false;
				}
				else
				{
					bDoorDown = true;
					bDoorUp = false;
				}
			}
			else
			{
				bDoorUp = false;
				bDoorDown = false;
			}

			if (RandomChoice == 1)
			{
				bDoorRight = true;
				bDoorTop = false;
			}
			else if (RandomChoice == 0)
			{
				bDoorRight = false;
				bDoorTop = true;
			}

			for (UINT32 TileY = 0; TileY < TilesPerHeight; ++TileY)
			{
				for (UINT32 TileX = 0; TileX < TilesPerWidth; ++TileX)
				{
					UINT32 AbsTileX = ScreenX * TilesPerWidth + TileX;
					UINT32 AbsTileY = ScreenY * TilesPerHeight + TileY;

					UINT32 TileValue = 1;
					if (TileX == 0 || TileY == 0 || TileX == TilesPerWidth - 1 || TileY == TilesPerHeight - 1)
					{
						TileValue = 2;
					}

					if (bDoorLeft && TileX == 0 && TileY == TilesPerHeight / 2)
					{
						TileValue = 1;
					}
					if (bDoorRight && TileX == TilesPerWidth - 1 && TileY == TilesPerHeight / 2)
					{
						TileValue = 1;
					}
					if (bDoorTop && TileY == TilesPerHeight - 1 && TileX == TilesPerWidth / 2)
					{
						TileValue = 1;
					}
					if (bDoorBottom && TileY == 0 && TileX == TilesPerWidth / 2)
					{
						TileValue = 1;
					}
					if (TileX == 2 && TileY == 2)
					{
						if (bDoorUp)
							TileValue = 3;
						else if (bDoorDown)
							TileValue = 4;
					}

					SetTileValue(&State->WorldArena, State->World->TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
				}
			}
			if (RandomChoice == 2)
			{
				AbsTileZ == 0 ? AbsTileZ = 1 : AbsTileZ = 0;
			}
			else if (RandomChoice == 1)
			{
				++ScreenX;
			}
			else
			{
				++ScreenY;
			}
		}
		Memory->bIsInitialized = true;
	}

	const UINT32 TileSideInPixels = 10;
	const float MetersToPixels = static_cast<float>(TileSideInPixels) / State->World->TileMap->TileSideInMeters;
	const float PlayerWidth = State->World->TileMap->TileSideInMeters * 0.65f;
	const float PlayerHeight = State->World->TileMap->TileSideInMeters * 0.65f;

	float dPlayerX = 0.0f;
	float dPlayerY = 0.0f;
	float PlayerSpeed = 6.0f;
	static TileMap_Position testPos;

	if (Input->Shift)
	{
		PlayerSpeed *= 10.f;
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

	for (INT32 RelRow = -100; RelRow < 100; ++RelRow)
	{
		for (INT32 RelColumn = -100; RelColumn < 100; ++RelColumn)
		{
			UINT32 Column = State->PlayerPosition.AbsTileX + RelColumn;
			UINT32 Row = State->PlayerPosition.AbsTileY + RelRow;

			UINT32 TileID = GetTileValue(State->World->TileMap, Column, Row, State->PlayerPosition.AbsTileZ);
			if (TileID > 0)
			{
				float Color = TileID == 2 ? 1.f : 0.5f;
				if (TileID > 2)
					Color = 0.25f;
				if (Row == State->PlayerPosition.AbsTileY && Column == State->PlayerPosition.AbsTileX)
					Color = 0.f;
				float CenterX = 0.5f * Buffer->BitmapWidth - MetersToPixels * State->PlayerPosition.TileRelX + static_cast<float>(RelColumn) * TileSideInPixels;
				float CenterY = 0.5f * Buffer->BitmapHeight + MetersToPixels * State->PlayerPosition.TileRelY - static_cast<float>(RelRow) * TileSideInPixels;
				float MinX = CenterX - TileSideInPixels / 2;
				float MinY = CenterY - TileSideInPixels / 2;
				float MaxX = CenterX + TileSideInPixels / 2;
				float MaxY = CenterY + TileSideInPixels / 2;
				DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Color, Color, Color);
			}
		}
	}

	float PlayerR = 1.0f;
	float PlayerG = 1.0f;
	float PlayerB = 0.0f;
	float PlayerLeft = 0.5f * Buffer->BitmapWidth - 0.5f * PlayerWidth * MetersToPixels;
	float PlayerTop = 0.5f * Buffer->BitmapHeight - PlayerHeight * MetersToPixels;
	DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + PlayerWidth * MetersToPixels, PlayerTop + PlayerHeight * MetersToPixels, PlayerR, PlayerG, PlayerB);
}

void GameGetSoundSamples(/*ThreadContext *Thread,*/ SoundBuffer *SBuffer/*, GameMemory *Memory*/)
{
	//GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	OutputSound(SBuffer, 400);
}
