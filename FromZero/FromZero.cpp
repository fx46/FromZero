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

static void DrawBitmap(PixelBuffer *Buffer, Bitmap *Bmap, float RealX, float RealY, float AlignX = 0, float AlignY = 0)
{
	RealX -= AlignX;
	RealY -= AlignY;
	INT32 MinX = RoundFloatToINT32(RealX);
	INT32 MinY = RoundFloatToINT32(RealY);
	INT32 MaxX = RoundFloatToINT32(RealX + static_cast<float>(Bmap->Width));
	INT32 MaxY = RoundFloatToINT32(RealY + static_cast<float>(Bmap->Height));

	int SourceOffsetX = 0;
	if (MinX < 0)
	{
		SourceOffsetX = -MinX;
		MinX = 0;
	}

	int SourceOffsetY = 0;
	if (MinY < 0)
	{
		SourceOffsetY = -MinY;
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

	UINT32 *SourceRow = Bmap->Pixels + Bmap->Width * (Bmap->Height - 1);
	SourceRow += -SourceOffsetY * Bmap->Width + SourceOffsetX;
	UINT8 *DestRow = reinterpret_cast<UINT8 *>(Buffer->BitmapMemory) + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch;
	for (INT32 Y = MinY; Y < MaxY; ++Y)
	{
		UINT32 *Dest = reinterpret_cast<UINT32 *>(DestRow);
		UINT32 *Source = SourceRow;
		for (INT32 X = MinX; X < MaxX; ++X)
		{
			float A = static_cast<float>((*Source >> 24) & 0xFF) / 255.0f;
			float SourceR = static_cast<float>((*Source >> 16) & 0xFF);
			float SourceG = static_cast<float>((*Source >> 8) & 0xFF);
			float SourceB = static_cast<float>((*Source) & 0xFF);
			float DestR = static_cast<float>((*Dest >> 16) & 0xFF);
			float DestG = static_cast<float>((*Dest >> 8) & 0xFF);
			float DestB = static_cast<float>((*Dest) & 0xFF);

			float R = (1.0f - A) * DestR + A * SourceR;
			float G = (1.0f - A) * DestG + A * SourceG;
			float B = (1.0f - A) * DestB + A * SourceB;

			*Dest++ = (static_cast<UINT32>(R + 0.5f) << 16 | static_cast<UINT32>(G + 0.5f) << 8 | static_cast<UINT32>(B + 0.5f));
			++Source;
		}
		DestRow += Buffer->Pitch;
		SourceRow -= Bmap->Width;
	}
}

static void DrawRectangle(PixelBuffer *Buffer, Vector Min, Vector Max, float R, float G, float B)
{
	INT32 MinX = RoundFloatToINT32(Min.X);
	INT32 MaxX = RoundFloatToINT32(Max.X);
	INT32 MinY = RoundFloatToINT32(Min.Y);
	INT32 MaxY = RoundFloatToINT32(Max.Y);

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

	UINT8 *Row = static_cast<UINT8 *>(Buffer->BitmapMemory) + MinX *Buffer->BytesPerPixel + MinY * Buffer->Pitch;

	for (int Y = MinY; Y < MaxY; ++Y)
	{
		UINT32 *Pixel = reinterpret_cast<UINT32 *>(Row);
		for (int X = MinX; X < MaxX; ++X)
		{
			*Pixel++ = Color;
		}
		Row += Buffer->Pitch;
	}
}

static Bitmap LoadBMP(const char *FileName)
{
	Bitmap Result = {};
	ReadFileResults ReadResult = ReadFile(FileName);

	if (ReadResult.ContentsSize != 0)
	{
		Bitmap_Header *Header = static_cast<Bitmap_Header *>(ReadResult.Contents);
		UINT32 *Pixels = reinterpret_cast<UINT32 *>(static_cast<UINT8 *>(ReadResult.Contents) + Header->BitmapOffset);
		Result.Pixels = Pixels;
		Result.Height = Header->Height;
		Result.Width = Header->Width;
		assert(Header->Compression == 3);

		UINT32 AlphaMask = ~(Header->RedMask | Header->GreenMask | Header->BlueMask);

		Bit_Scan_Result RedScan   = FindLeastSignificantSetBit(Header->RedMask);
		Bit_Scan_Result GreenScan = FindLeastSignificantSetBit(Header->GreenMask);
		Bit_Scan_Result BlueScan  = FindLeastSignificantSetBit(Header->BlueMask);
		Bit_Scan_Result AlphaScan = FindLeastSignificantSetBit(AlphaMask);

		assert(RedScan.Found);
		assert(GreenScan.Found);
		assert(BlueScan.Found);
		assert(AlphaScan.Found);

		UINT32 *SourceDest = Pixels;
		for (int Y = 0; Y < Header->Height; ++Y)
		{
			for (int X = 0; X < Header->Width; X++)
			{
				*SourceDest++ = (_rotl(*SourceDest & Header->RedMask,	16 - static_cast<int>(RedScan.Index))   |
								 _rotl(*SourceDest & Header->GreenMask, 8  - static_cast<int>(GreenScan.Index)) |
								 _rotl(*SourceDest & Header->BlueMask,	0  - static_cast<int>(BlueScan.Index))  |
								 _rotl(*SourceDest & AlphaMask,			24 - static_cast<int>(AlphaScan.Index)));
			}
		}
	}

	return Result;
}

static void TestWall(float WallX, float PlayerDeltaX, float PlayerDeltaY, float RelX, float RelY, float *MinT, float MinY, float MaxY)
{
	float Epsilon = 0.0001f;
	if (PlayerDeltaX != 0.f)
	{
		float TResult = (WallX - RelX) / PlayerDeltaX;
		float Y = RelY + TResult * PlayerDeltaY;
		if ((TResult >= 0.f) && (*MinT > TResult))
		{
			if ((Y >= MinY) && (Y <= MaxY))
			{
				*MinT = ((TResult - Epsilon) > 0.0f) ? (TResult - Epsilon) : 0.0f;
			}
		}
	}
}

void GameUpdateAndRencer(/*ThreadContext *Thread,*/ PixelBuffer *Buffer, GameInput *Input, GameMemory *Memory)
{
	assert(sizeof(GameState) <= Memory->PermanentStorageSize);

	GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	UINT32 TilesPerWidth = 17;
	UINT32 TilesPerHeight = 9;

	if (!Memory->bIsInitialized)
	{
		State->Background = LoadBMP("assets/testImage.bmp");
		State->PlayerSprite = LoadBMP("assets/Player.bmp");
		State->CameraPosition.AbsTileX = TilesPerWidth / 2;
		State->CameraPosition.AbsTileY = TilesPerHeight / 2;
		State->PlayerPosition.AbsTileX = 1;
		State->PlayerPosition.AbsTileY = 3;
		State->PlayerPosition.Offset.X = 0.f;
		State->PlayerPosition.Offset.Y = 0.f;

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

		UINT32 ScreenX = 0;
		UINT32 ScreenY = 0;
		UINT32 AbsTileZ = 0;
		bool bDoorLeft = false;
		bool bDoorRight = false;
		bool bDoorTop = false;
		bool bDoorBottom = false;
		bool bDoorUp = false;
		bool bDoorDown = false;
		bool bSwitchedFloor = false;

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

			if (bSwitchedFloor)
			{
				bDoorUp = !bDoorUp;
				bDoorDown = !bDoorDown;
				bSwitchedFloor = false;
			}
			else
			{
				bDoorUp = false;
				bDoorDown = false;
			}

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
				bSwitchedFloor = true;
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

	const UINT32 TileSideInPixels = 60;
	const float MetersToPixels = static_cast<float>(TileSideInPixels) / State->World->TileMap->TileSideInMeters;
	const float PlayerWidth = State->World->TileMap->TileSideInMeters * 0.65f;
	const float PlayerHeight = State->World->TileMap->TileSideInMeters * 0.65f;
	Vector PlayerAcceleration = {};
	static TileMap_Position testPos;

	if (Input->A)
	{
		PlayerAcceleration.X = -1.0f;
	}
	if (Input->D)
	{
		PlayerAcceleration.X = 1.0f;
	}
	if (Input->W)
	{
		PlayerAcceleration.Y = 1.0f;
	}
	if (Input->S)
	{
		PlayerAcceleration.Y = -1.0f;
	}

	if (NormSq(PlayerAcceleration) > 1.0f)
	{
		PlayerAcceleration /= Norm(PlayerAcceleration);
	}

	float PlayerSpeed = 50.0f;
	if (Input->Shift)
	{
		PlayerSpeed *= 5.f;
	}
	PlayerAcceleration *= PlayerSpeed;
	PlayerAcceleration += State->PlayerVelocity * -8.f;

	TileMap_Position OldPlayerPosition = State->PlayerPosition;
	Vector PlayerDelta = PlayerAcceleration * 0.5f * Input->TimeElapsingOverFrame * Input->TimeElapsingOverFrame
		+ State->PlayerVelocity * Input->TimeElapsingOverFrame;
	State->PlayerVelocity = PlayerAcceleration * Input->TimeElapsingOverFrame + State->PlayerVelocity;
	TileMap_Position NewPlayerPosition = Offset(State->World->TileMap, OldPlayerPosition, PlayerDelta);

	UINT32 StartTileX = OldPlayerPosition.AbsTileX;
	UINT32 StartTileY = OldPlayerPosition.AbsTileY;
	UINT32 EndTileX = NewPlayerPosition.AbsTileX;
	UINT32 EndTileY = NewPlayerPosition.AbsTileY;

	int DeltaX = SignOf(EndTileX - StartTileX);
	int DeltaY = SignOf(EndTileY - StartTileY);

	UINT32 AbsTileZ = State->PlayerPosition.AbsTileZ;
	float MinT = 1.f;

	UINT32 AbsTileY = StartTileY;
	for(;;)
	{
		UINT32 AbsTileX = StartTileX;
		for (;;)
		{
			TileMap_Position TestTilePosition = { AbsTileX , AbsTileY, AbsTileZ };
			if (!WorldIsEmptyAtPosition(State->World->TileMap, &TestTilePosition))
			{
				Vector MinCorner = Vector(State->World->TileMap->TileSideInMeters, State->World->TileMap->TileSideInMeters) * -.5f;
				Vector MaxCorner = Vector(State->World->TileMap->TileSideInMeters, State->World->TileMap->TileSideInMeters) *  .5f;

				TileMap_Difference RelOldPlayerPosition = Substract(State->World->TileMap, &OldPlayerPosition, &TestTilePosition);
				Vector Rel = RelOldPlayerPosition.dXY;

				TestWall(MinCorner.X, PlayerDelta.X, PlayerDelta.Y, Rel.X, Rel.Y, &MinT, MinCorner.Y, MaxCorner.Y);
				TestWall(MaxCorner.X, PlayerDelta.X, PlayerDelta.Y, Rel.X, Rel.Y, &MinT, MinCorner.Y, MaxCorner.Y);
				TestWall(MinCorner.Y, PlayerDelta.Y, PlayerDelta.X, Rel.Y, Rel.X, &MinT, MinCorner.X, MaxCorner.X);
				TestWall(MaxCorner.Y, PlayerDelta.Y, PlayerDelta.X, Rel.Y, Rel.X, &MinT, MinCorner.X, MaxCorner.X);
			}

			if (AbsTileX == EndTileX)
				break;
			else
				AbsTileX += DeltaX;
		}

		if (AbsTileY == EndTileY)
			break;
		else
			AbsTileY += DeltaY;
	}

	State->PlayerPosition = Offset(State->World->TileMap, OldPlayerPosition, MinT * PlayerDelta);

	if (!PositionsAreOnTheSameTile(&OldPlayerPosition, &State->PlayerPosition))
	{
		UINT32 TileValue = GetTileValue(State->World->TileMap, &State->PlayerPosition);
		if (TileValue == 3)
			++State->PlayerPosition.AbsTileZ;
		else if (TileValue == 4)
			--State->PlayerPosition.AbsTileZ;
	}

	TileMap_Difference PlayerPosToCam = Substract(State->World->TileMap, &State->PlayerPosition, &State->CameraPosition);
	float PlayerX = 0.5f * Buffer->BitmapWidth + PlayerPosToCam.dXY.X * MetersToPixels;
	float PlayerY = 0.5f * Buffer->BitmapHeight - PlayerPosToCam.dXY.Y * MetersToPixels;
	State->CameraPosition.AbsTileZ = State->PlayerPosition.AbsTileZ;
	if (PlayerPosToCam.dXY.X > (static_cast<float>(1 + TilesPerWidth / 2) * State->World->TileMap->TileSideInMeters))
		State->CameraPosition.AbsTileX += TilesPerWidth;
	else if (PlayerPosToCam.dXY.X < (-static_cast<float>(1 + TilesPerWidth / 2) * State->World->TileMap->TileSideInMeters))
		State->CameraPosition.AbsTileX -= TilesPerWidth;
	if (PlayerPosToCam.dXY.Y > (static_cast<float>(1 + TilesPerHeight / 2) * State->World->TileMap->TileSideInMeters))
		State->CameraPosition.AbsTileY += TilesPerHeight;
	else if (PlayerPosToCam.dXY.Y < (-static_cast<float>(1 + TilesPerHeight / 2) * State->World->TileMap->TileSideInMeters))
		State->CameraPosition.AbsTileY -= TilesPerHeight;

	DrawBitmap(Buffer, &State->Background, 0, 0);
	
	for (INT32 RelRow = -10; RelRow < 10; ++RelRow)
	{
		for (INT32 RelColumn = -20; RelColumn < 20; ++RelColumn)
		{
			UINT32 Column = State->CameraPosition.AbsTileX + RelColumn;
			UINT32 Row = State->CameraPosition.AbsTileY + RelRow;

			UINT32 TileID = GetTileValue(State->World->TileMap, Column, Row, State->CameraPosition.AbsTileZ);
			if (TileID > 1)
			{
				float Color = TileID == 2 ? 1.f : 0.5f;
				if (TileID > 2)
					Color = 0.25f;
				if (Row == State->CameraPosition.AbsTileY && Column == State->CameraPosition.AbsTileX)
					Color = 0.f;

				Vector Center = { 0.5f * Buffer->BitmapWidth - MetersToPixels * State->CameraPosition.Offset.X + static_cast<float>(RelColumn) * TileSideInPixels,
								  0.5f * Buffer->BitmapHeight + MetersToPixels * State->CameraPosition.Offset.Y - static_cast<float>(RelRow) * TileSideInPixels };
				Vector TileSide = { TileSideInPixels / 2 , TileSideInPixels / 2 };
				Vector Min = Center - TileSide;
				Vector Max = Center + TileSide;
				DrawRectangle(Buffer, Min, Max, Color, Color, Color);
			}
		}
	}

	DrawBitmap(Buffer, &State->PlayerSprite, PlayerX, PlayerY, 30.0f / 2.0f, 50.0f);
}

void GameGetSoundSamples(/*ThreadContext *Thread,*/ SoundBuffer *SBuffer/*, GameMemory *Memory*/)
{
	//GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	OutputSound(SBuffer, 400);
}
