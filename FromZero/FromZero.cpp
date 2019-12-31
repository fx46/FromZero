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
		Bit_Scan_Result RedShift   = FindLeastSignificantSetBit(Header->RedMask);
		Bit_Scan_Result GreenShift = FindLeastSignificantSetBit(Header->GreenMask);
		Bit_Scan_Result BlueShift  = FindLeastSignificantSetBit(Header->BlueMask);
		Bit_Scan_Result AlphaShift = FindLeastSignificantSetBit(AlphaMask);

		assert(RedShift.Found);
		assert(GreenShift.Found);
		assert(BlueShift.Found);
		assert(AlphaShift.Found);

		UINT32 *SourceDest = Pixels;
		for (int Y = 0; Y < Header->Height; ++Y)
		{
			for (int X = 0; X < Header->Width; X++)
			{
				*SourceDest++ = (*SourceDest >> AlphaShift.Index & 0xFF) << 24 |
					(*SourceDest >> RedShift.Index				 & 0xFF) << 16 |
					(*SourceDest >> GreenShift.Index			 & 0xFF) << 8  |
					(*SourceDest >> BlueShift.Index				 & 0xFF);
			}
		}
	}

	return Result;
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
		State->PlayerPosition.AbsTileY = 1;
		State->PlayerPosition.Offset.X = 5.f;
		State->PlayerPosition.Offset.Y = 5.f;

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

	DrawBitmap(Buffer, &State->Background, 0, 0);

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

	if ((PlayerAcceleration.X != 0.f) && (PlayerAcceleration.Y != 0.f))
	{
		PlayerAcceleration *= 0.707106781187f;
	}

	float PlayerSpeed = 40.0f;
	if (Input->Shift)
	{
		PlayerSpeed *= 5.f;
	}
	PlayerAcceleration *= PlayerSpeed;
	PlayerAcceleration += State->PlayerVelocity * -5.f;

	TileMap_Position NewPlayerPosition = State->PlayerPosition;
	NewPlayerPosition.Offset = PlayerAcceleration * 0.5f * Input->TimeElapsingOverFrame * Input->TimeElapsingOverFrame
								+ State->PlayerVelocity * Input->TimeElapsingOverFrame + NewPlayerPosition.Offset;
	State->PlayerVelocity = PlayerAcceleration * Input->TimeElapsingOverFrame + State->PlayerVelocity;
	
	NewPlayerPosition = CanonicalizePosition(State->World->TileMap, NewPlayerPosition);

	TileMap_Position NewPosLeft = NewPlayerPosition;
	NewPosLeft.Offset.X -= PlayerWidth / 2;
	NewPosLeft = CanonicalizePosition(State->World->TileMap, NewPosLeft);

	TileMap_Position NewPosRight = NewPlayerPosition;
	NewPosRight.Offset.X += PlayerWidth / 2;
	NewPosRight = CanonicalizePosition(State->World->TileMap, NewPosRight);

	bool Collided = false;
	TileMap_Position CollisionPosition = {};
	if (!WorldIsEmptyAtPosition(State->World->TileMap, &NewPlayerPosition))
	{
		CollisionPosition = NewPlayerPosition;
		Collided = true;
	}
	if (!WorldIsEmptyAtPosition(State->World->TileMap, &NewPosLeft))
	{
		CollisionPosition = NewPosLeft;
		Collided = true;
	}
	if (!WorldIsEmptyAtPosition(State->World->TileMap, &NewPosRight))
	{
		CollisionPosition = NewPosRight;
		Collided = true;
	}

	if (Collided)
	{
		Vector Normal = { 0, 0 };
		if (CollisionPosition.AbsTileX < State->PlayerPosition.AbsTileX)
		{
			Normal = { 1, 0 };
		}
		if (CollisionPosition.AbsTileX > State->PlayerPosition.AbsTileX)
		{
			Normal = { -1, 0 };
		}
		if (CollisionPosition.AbsTileY < State->PlayerPosition.AbsTileY)
		{
			Normal = { 0, 1 };
		}
		if (CollisionPosition.AbsTileY > State->PlayerPosition.AbsTileY)
		{
			Normal = { 0, -1 };
		}

		State->PlayerVelocity -= Normal * Dot(State->PlayerVelocity, Normal) * 2;
	}
	else
	{
		if (!PositionsAreOnTheSameTile(&State->PlayerPosition, &NewPlayerPosition))
		{
			UINT32 TileValue = GetTileValue(State->World->TileMap, &NewPlayerPosition);
			if (TileValue == 3)
				++NewPlayerPosition.AbsTileZ;
			else if (TileValue == 4)
				--NewPlayerPosition.AbsTileZ;
		}
		State->PlayerPosition = NewPlayerPosition;
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
