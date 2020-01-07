#include "FromZero.h"
#include <intrin.h>

static void OutputSound(SoundBuffer *Buffer, int ToneHz)
{
	static float TSine;
	//INT16 ToneVolume = 3000;
	int16 *SampleOut = Buffer->Samples;
	int WavePeriod = Buffer->SamplesPerSecond / ToneHz;
	const float Tau = 2.0f * 3.14159265359f;

	for (int SampleIndex = 0; SampleIndex < Buffer->SampleCountToOutput; SampleIndex++)
	{
		//float SineValue = sinf(TSine);
		int16 SampleValue = 0; // static_cast<INT16>(SineValue * ToneVolume);
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
	int32 MinX = RoundFloatToINT32(RealX);
	int32 MinY = RoundFloatToINT32(RealY);
	int32 MaxX = RoundFloatToINT32(RealX + static_cast<float>(Bmap->Width));
	int32 MaxY = RoundFloatToINT32(RealY + static_cast<float>(Bmap->Height));

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

	uint32 *SourceRow = Bmap->Pixels + Bmap->Width * (Bmap->Height - 1);
	SourceRow += -SourceOffsetY * Bmap->Width + SourceOffsetX;
	uint8 *DestRow = reinterpret_cast<uint8 *>(Buffer->BitmapMemory) + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch;
	for (int32 Y = MinY; Y < MaxY; ++Y)
	{
		uint32 *Dest = reinterpret_cast<uint32 *>(DestRow);
		uint32 *Source = SourceRow;
		for (int32 X = MinX; X < MaxX; ++X)
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

			*Dest++ = (static_cast<uint32>(R + 0.5f) << 16 | static_cast<uint32>(G + 0.5f) << 8 | static_cast<uint32>(B + 0.5f));
			++Source;
		}
		DestRow += Buffer->Pitch;
		SourceRow -= Bmap->Width;
	}
}

static void DrawRectangle(PixelBuffer *Buffer, Vector Min, Vector Max, float R, float G, float B)
{
	int32 MinX = RoundFloatToINT32(Min.X);
	int32 MaxX = RoundFloatToINT32(Max.X);
	int32 MinY = RoundFloatToINT32(Min.Y);
	int32 MaxY = RoundFloatToINT32(Max.Y);

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
	uint32 Color = RoundFloatToUINT32(R * 255.f) << 16 | RoundFloatToUINT32(G * 255.f) << 8 | RoundFloatToUINT32(B * 255.f);

	uint8 *Row = static_cast<uint8 *>(Buffer->BitmapMemory) + MinX *Buffer->BytesPerPixel + MinY * Buffer->Pitch;

	for (int Y = MinY; Y < MaxY; ++Y)
	{
		uint32 *Pixel = reinterpret_cast<uint32 *>(Row);
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
		uint32 *Pixels = reinterpret_cast<uint32 *>(static_cast<uint8 *>(ReadResult.Contents) + Header->BitmapOffset);
		Result.Pixels = Pixels;
		Result.Height = Header->Height;
		Result.Width = Header->Width;
		assert(Header->Compression == 3);

		uint32 AlphaMask = ~(Header->RedMask | Header->GreenMask | Header->BlueMask);

		Bit_Scan_Result RedScan   = FindLeastSignificantSetBit(Header->RedMask);
		Bit_Scan_Result GreenScan = FindLeastSignificantSetBit(Header->GreenMask);
		Bit_Scan_Result BlueScan  = FindLeastSignificantSetBit(Header->BlueMask);
		Bit_Scan_Result AlphaScan = FindLeastSignificantSetBit(AlphaMask);

		assert(RedScan.Found);
		assert(GreenScan.Found);
		assert(BlueScan.Found);
		assert(AlphaScan.Found);

		uint32 *SourceDest = Pixels;
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

static bool TestWall(float WallX, float PlayerDeltaX, float PlayerDeltaY, float RelX, float RelY, float *MinT, float MinY, float MaxY)
{
	bool Hit = false;
	float Epsilon = 0.001f;
	if (PlayerDeltaX != 0.f)
	{
		float TResult = (WallX - RelX) / PlayerDeltaX;
		float Y = RelY + TResult * PlayerDeltaY;
		if ((TResult >= 0.f) && (*MinT > TResult))
		{
			if ((Y >= MinY) && (Y <= MaxY))
			{
				*MinT = ((TResult - Epsilon) > 0.0f) ? (TResult - Epsilon) : 0.0f;
				Hit = true;
			}
		}
	}

	return Hit;
}

static Entity* GetEntity(GameState *State, uint32 Index)
{
	Entity *E = 0;

	if (Index > 0 && Index < (sizeof(State->Entities) / sizeof(*State->Entities)))
	{
		E = &State->Entities[Index];
	}

	return E;
}

static void InitializeEntity(Entity *E)
{
	E->Exists = true;
	E->Position.AbsTileX = 1;
	E->Position.AbsTileY = 3;
	E->Height = 0.5f;
	E->Width = 0.5f;
}

static uint32 AddEntity(GameState *State)
{
	assert(State->EntityCount < (sizeof(State->Entities) / sizeof(*State->Entities)));

	uint32 EntityIndex = State->EntityCount++;
	Entity *E = &State->Entities[EntityIndex];
	*E = {};

	return EntityIndex;
}

static void MoveEntity(GameState *State, Entity *E, float Dt, Vector Acceleration)
{
	float PlayerSpeed = 50.0f;
	Acceleration *= PlayerSpeed;

	Acceleration += E->Velocity * -8.f;

	Vector PlayerDelta = Acceleration * 0.5f * Dt * Dt + E->Velocity * Dt;
	E->Velocity = Acceleration * Dt + E->Velocity;
	TileMap_Position OldPlayerPosition = E->Position;
	TileMap_Position NewPlayerPosition = Offset(State->World->TileMap, OldPlayerPosition, PlayerDelta);

	uint32 MinTileX = OldPlayerPosition.AbsTileX < NewPlayerPosition.AbsTileX ? OldPlayerPosition.AbsTileX : NewPlayerPosition.AbsTileX;
	uint32 MinTileY = OldPlayerPosition.AbsTileY < NewPlayerPosition.AbsTileY ? OldPlayerPosition.AbsTileY : NewPlayerPosition.AbsTileY;
	uint32 MaxTileX = OldPlayerPosition.AbsTileX > NewPlayerPosition.AbsTileX ? OldPlayerPosition.AbsTileX : NewPlayerPosition.AbsTileX;
	uint32 MaxTileY = OldPlayerPosition.AbsTileY > NewPlayerPosition.AbsTileY ? OldPlayerPosition.AbsTileY : NewPlayerPosition.AbsTileY;

	uint32 PlayerTileWidth = CeilFloatToINT32(E->Width / State->World->TileMap->TileSideInMeters);
	uint32 PlayerTileHeight = CeilFloatToINT32(E->Height / State->World->TileMap->TileSideInMeters);
	MinTileX -= PlayerTileWidth;
	MinTileY -= PlayerTileHeight;
	MaxTileX += PlayerTileWidth;
	MaxTileY += PlayerTileHeight;

	uint32 AbsTileZ = E->Position.AbsTileZ;

	float RemainingT = 1.f;
	for (uint32 i = 0; i < 4 && RemainingT > 0.f; ++i)
	{
		float MinT = 1.f;
		Vector WallNormal = { };

		assert((MaxTileX - MinTileX) < 32);
		assert((MaxTileY - MinTileY) < 32);

		for (uint32 AbsTileY = MinTileY; AbsTileY <= MaxTileY; ++AbsTileY)
		{
			for (uint32 AbsTileX = MinTileX; AbsTileX <= MaxTileX; ++AbsTileX)
			{
				TileMap_Position TestTilePosition = { AbsTileX , AbsTileY, AbsTileZ };
				if (!WorldIsEmptyAtPosition(State->World->TileMap, &TestTilePosition))
				{
					float DiameterW = State->World->TileMap->TileSideInMeters + E->Width;
					float DiameterH = State->World->TileMap->TileSideInMeters + E->Height;
					Vector MinCorner = Vector(DiameterW, DiameterH) * -.5f;
					Vector MaxCorner = Vector(DiameterW, DiameterH) *  .5f;

					TileMap_Difference RelOldPlayerPosition = Substract(State->World->TileMap, &E->Position, &TestTilePosition);
					Vector Rel = RelOldPlayerPosition.dXY;

					if (TestWall(MinCorner.X, PlayerDelta.X, PlayerDelta.Y, Rel.X, Rel.Y, &MinT, MinCorner.Y, MaxCorner.Y))
						WallNormal = { -1, 0 };
					if (TestWall(MaxCorner.X, PlayerDelta.X, PlayerDelta.Y, Rel.X, Rel.Y, &MinT, MinCorner.Y, MaxCorner.Y))
						WallNormal = { 1, 0 };
					if (TestWall(MinCorner.Y, PlayerDelta.Y, PlayerDelta.X, Rel.Y, Rel.X, &MinT, MinCorner.X, MaxCorner.X))
						WallNormal = { 0, -1 };
					if (TestWall(MaxCorner.Y, PlayerDelta.Y, PlayerDelta.X, Rel.Y, Rel.X, &MinT, MinCorner.X, MaxCorner.X))
						WallNormal = { 0, 1 };
				}
			}
		}

		E->Position = Offset(State->World->TileMap, E->Position, MinT * PlayerDelta);
		E->Velocity -= WallNormal * Dot(E->Velocity, WallNormal);
		PlayerDelta -= WallNormal * Dot(PlayerDelta, WallNormal);
		RemainingT -= MinT * RemainingT;
	}
	
	if (!PositionsAreOnTheSameTile(&OldPlayerPosition, &E->Position))
	{
		uint32 TileValue = GetTileValue(State->World->TileMap, &E->Position);
		if (TileValue == 3)
			++E->Position.AbsTileZ;
		else if (TileValue == 4)
			--E->Position.AbsTileZ;
	}
}

void GameUpdateAndRencer(/*ThreadContext *Thread,*/ PixelBuffer *Buffer, GameInput *Input, GameMemory *Memory)
{
	assert(sizeof(GameState) <= Memory->PermanentStorageSize);

	GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	uint32 TilesPerWidth = 17;
	uint32 TilesPerHeight = 9;

	if (!Memory->bIsInitialized)
	{
		State->Background = LoadBMP("assets/testImage.bmp");
		State->PlayerSprite = LoadBMP("assets/Player.bmp");
		State->CameraPosition.AbsTileX = TilesPerWidth / 2;
		State->CameraPosition.AbsTileY = TilesPerHeight / 2;

		State->CameraFollowingEntityIndex = AddEntity(State);
		State->PlayerEntity = &State->Entities[State->CameraFollowingEntityIndex];
		InitializeEntity(State->PlayerEntity);

		InitializeArena(&State->WorldArena, Memory->PermanentStorageSize - sizeof(GameState), 
			reinterpret_cast<uint8 *>(Memory->PermanentStorage) + sizeof(GameState));

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

		uint32 ScreenX = 0;
		uint32 ScreenY = 0;
		uint32 AbsTileZ = 0;
		bool bDoorLeft = false;
		bool bDoorRight = false;
		bool bDoorTop = false;
		bool bDoorBottom = false;
		bool bDoorUp = false;
		bool bDoorDown = false;
		bool bSwitchedFloor = false;

		for (uint32 ScreenIndex = 0; ScreenIndex < 20; ++ScreenIndex)
		{
			bDoorLeft = bDoorRight;
			bDoorBottom = bDoorTop;
			bDoorRight = false;
			bDoorTop = false;

			uint32 RandomChoice;
			if (bDoorUp || bDoorDown)
				RandomChoice  = Random() % 2;
			else
				RandomChoice = Random() % 3;

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

			for (uint32 TileY = 0; TileY < TilesPerHeight; ++TileY)
			{
				for (uint32 TileX = 0; TileX < TilesPerWidth; ++TileX)
				{
					uint32 AbsTileX = ScreenX * TilesPerWidth + TileX;
					uint32 AbsTileY = ScreenY * TilesPerHeight + TileY;

					uint32 TileValue = 1;
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

	const uint32 TileSideInPixels = 60;
	const float MetersToPixels = static_cast<float>(TileSideInPixels) / State->World->TileMap->TileSideInMeters;
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
	if (Input->Shift)
	{
		PlayerAcceleration *= 5.f;
	}

	MoveEntity(State, State->PlayerEntity, Input->TimeElapsingOverFrame, PlayerAcceleration);

	State->CameraPosition.AbsTileZ = State->PlayerEntity->Position.AbsTileZ;
	TileMap_Difference PlayerPosToCam = Substract(State->World->TileMap, &State->PlayerEntity->Position, &State->CameraPosition);
	if (PlayerPosToCam.dXY.X > (static_cast<float>(1 + TilesPerWidth / 2) * State->World->TileMap->TileSideInMeters))
		State->CameraPosition.AbsTileX += TilesPerWidth;
	else if (PlayerPosToCam.dXY.X < (-static_cast<float>(1 + TilesPerWidth / 2) * State->World->TileMap->TileSideInMeters))
		State->CameraPosition.AbsTileX -= TilesPerWidth;
	if (PlayerPosToCam.dXY.Y > (static_cast<float>(1 + TilesPerHeight / 2) * State->World->TileMap->TileSideInMeters))
		State->CameraPosition.AbsTileY += TilesPerHeight;
	else if (PlayerPosToCam.dXY.Y < (-static_cast<float>(1 + TilesPerHeight / 2) * State->World->TileMap->TileSideInMeters))
		State->CameraPosition.AbsTileY -= TilesPerHeight;

	DrawBitmap(Buffer, &State->Background, 0, 0);
	
	for (int32 RelRow = -10; RelRow < 10; ++RelRow)
	{
		for (int32 RelColumn = -20; RelColumn < 20; ++RelColumn)
		{
			uint32 Column = State->CameraPosition.AbsTileX + RelColumn;
			uint32 Row = State->CameraPosition.AbsTileY + RelRow;

			uint32 TileID = GetTileValue(State->World->TileMap, Column, Row, State->CameraPosition.AbsTileZ);
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

	Entity *E = State->Entities;
	for (uint32 EntityIndex = 0; EntityIndex < State->EntityCount; ++EntityIndex, ++E)
	{
		if (E->Exists)
		{
			TileMap_Difference EntityPosToCam = Substract(State->World->TileMap, &E->Position, &State->CameraPosition);
			float X = 0.5f * Buffer->BitmapWidth + EntityPosToCam.dXY.X * MetersToPixels;
			float Y = 0.5f * Buffer->BitmapHeight - EntityPosToCam.dXY.Y * MetersToPixels;
			DrawBitmap(Buffer, &State->PlayerSprite, X, Y, 30.0f / 2.0f, 50.0f);
		}
	}
}

void GameGetSoundSamples(/*ThreadContext *Thread,*/ SoundBuffer *SBuffer/*, GameMemory *Memory*/)
{
	//GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	OutputSound(SBuffer, 400);
}
