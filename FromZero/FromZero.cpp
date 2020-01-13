#include "FromZero.h"
#include <intrin.h>
#include "Math.h"
#include <stdint.h>

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
	int32 MaxX = MinX + Bmap->Width;
	int32 MaxY = MinY + Bmap->Height;

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

static HighF_Entity* MakeEntityHighFrequency(GameState *State, uint32 LowIndex)
{
	HighF_Entity *HighEntity = 0;

	LowF_Entity *LowEntity = &State->LowEntities[LowIndex];
	if (LowEntity->HighEntityIndex)
	{
		HighEntity = &State->HighEntities_[LowEntity->HighEntityIndex];
	}
	else
	{
		if (State->HighEntityCount < (sizeof(State->HighEntities_) / sizeof(*State->HighEntities_)))
		{
			uint32 HighIndex = State->HighEntityCount++;
			HighEntity = &State->HighEntities_[HighIndex];

			World_Position_Difference Diff = Subtract(State->W, &LowEntity->Position, &State->CameraPosition);
			HighEntity->Position = Diff.dXY;
			HighEntity->Velocity = Vector(0, 0);
			HighEntity->AbsTileZ = LowEntity->Position.AbsTileZ;
			HighEntity->LowEntityIndex = LowIndex;

			LowEntity->HighEntityIndex = HighIndex;
		}
	}

	return HighEntity;
}

static void MakeEntityLowFrequency(GameState *State, uint32 LowIndex)
{
	LowF_Entity *LowEntity = &State->LowEntities[LowIndex];
	uint32 HighIndex = LowEntity->HighEntityIndex;
	if (HighIndex)
	{
		uint32 LastHighIndex = State->HighEntityCount - 1;
		if (HighIndex != LastHighIndex)
		{
			HighF_Entity *LastHighEntity = &State->HighEntities_[LastHighIndex];
			HighF_Entity *DeletedEntity = &State->HighEntities_[HighIndex];

			*DeletedEntity = *LastHighEntity;
			State->LowEntities[LastHighEntity->LowEntityIndex].HighEntityIndex = HighIndex;
		}
		--State->HighEntityCount;
		LowEntity->HighEntityIndex = 0;
	}
}

static LowF_Entity* GetLowEntity(GameState *State, uint32 Index)
{
	if (Index > 0 && Index < State->LowEntityCount)
	{
		return &State->LowEntities[Index];
	}

	return 0;
}

static Entity GetHighEntity(GameState *State, uint32 LowIndex)
{
	Entity Result = {};
	if(LowIndex > 0 && LowIndex < State->LowEntityCount)
	{
		Result.LowIndex = LowIndex;
		Result.Low = &State->LowEntities[LowIndex];
		Result.High = MakeEntityHighFrequency(State, LowIndex);
	}

	return Result;
}

static void OffsetAndCheckFrequencyByArea(GameState *State, Vector Offset, Rectangle HighFrequencyBounds)
{
	for (uint32 EntityIndex = 1; EntityIndex < State->HighEntityCount;)
	{
		HighF_Entity *High = &State->HighEntities_[EntityIndex];
		High->Position += Offset;
		if (IsInRectangle(HighFrequencyBounds, High->Position))
		{
			++EntityIndex;
		}
		else
		{
			MakeEntityLowFrequency(State, EntityIndex);
		}
	}
}

static uint32 AddLowEntity(GameState *State, Entity_Type Type)
{
	assert(State->LowEntityCount < (sizeof(State->LowEntities) / sizeof(*State->LowEntities)));
	
	uint32 EntityIndex = State->LowEntityCount++;
	State->LowEntities[EntityIndex] = {};
	State->LowEntities[EntityIndex].Type = Type;

	return EntityIndex;
}

static uint32 AddPlayer(GameState *State)
{
	uint32 EntityIndex = AddLowEntity(State, Entity_Type_Player);
	LowF_Entity *E = GetLowEntity(State, EntityIndex);

	E->Position = State->CameraPosition;
	E->Height = 0.5f;
	E->Width = 0.5f;
	E->Collides = true;

	if (State->PlayerEntityIndex == 0)
	{
		State->PlayerEntityIndex = EntityIndex;
	}

	return EntityIndex;
}

static uint32 AddWall(GameState *State, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	uint32 EntityIndex = AddLowEntity(State, Entity_Type_Wall);
	LowF_Entity *E = GetLowEntity(State, EntityIndex);

	E->Position.AbsTileX = AbsTileX;
	E->Position.AbsTileY = AbsTileY;
	E->Position.AbsTileZ = AbsTileZ;
	E->Height = State->W->TileSideInMeters;
	E->Width = State->W->TileSideInMeters;
	E->Collides = true;

	return EntityIndex;
}

static void MovePlayer(GameState *State, Entity E, float Dt, Vector Acceleration)
{
	float PlayerSpeed = 50.0f;
	Acceleration *= PlayerSpeed;

	Acceleration += E.High->Velocity * -8.f;
	
	Vector OldPlayerPosition = E.High->Position;
	Vector PlayerDelta = Acceleration * 0.5f * Dt * Dt + E.High->Velocity * Dt;
	E.High->Velocity = Acceleration * Dt + E.High->Velocity;
	Vector NewPlayerPosition = OldPlayerPosition + PlayerDelta;

	for (uint32 i = 0; i < 4; ++i)
	{
		float MinT = 1.f;
		Vector WallNormal = { };
		uint32 HitHighEntityIndex = 0;

		Vector DesiredPosition = E.High->Position + PlayerDelta;

		for (uint32 TestHighEntityIndex = 1; TestHighEntityIndex < State->HighEntityCount; ++TestHighEntityIndex)
		{
			if (TestHighEntityIndex != E.Low->HighEntityIndex)
			{
				Entity TestEntity;
				TestEntity.High = &State->HighEntities_[TestHighEntityIndex];
				TestEntity.LowIndex = TestEntity.High->LowEntityIndex;
				TestEntity.Low = &State->LowEntities[TestEntity.LowIndex];
				if (TestEntity.Low->Collides)
				{
					float DiameterW = E.Low->Width + TestEntity.Low->Width;
					float DiameterH = E.Low->Height + TestEntity.Low->Height;

					Vector MinCorner = Vector(DiameterW, DiameterH) * -.5f;
					Vector MaxCorner = Vector(DiameterW, DiameterH) *  .5f;

					Vector Rel = E.High->Position - TestEntity.High->Position;

					if (TestWall(MinCorner.X, PlayerDelta.X, PlayerDelta.Y, Rel.X, Rel.Y, &MinT, MinCorner.Y, MaxCorner.Y))
					{
						WallNormal = { -1, 0 };
						HitHighEntityIndex = TestHighEntityIndex;
					}
					if (TestWall(MaxCorner.X, PlayerDelta.X, PlayerDelta.Y, Rel.X, Rel.Y, &MinT, MinCorner.Y, MaxCorner.Y))
					{
						WallNormal = { 1, 0 };
						HitHighEntityIndex = TestHighEntityIndex;
					}
					if (TestWall(MinCorner.Y, PlayerDelta.Y, PlayerDelta.X, Rel.Y, Rel.X, &MinT, MinCorner.X, MaxCorner.X))
					{
						WallNormal = { 0, -1 };
						HitHighEntityIndex = TestHighEntityIndex;
					}
					if (TestWall(MaxCorner.Y, PlayerDelta.Y, PlayerDelta.X, Rel.Y, Rel.X, &MinT, MinCorner.X, MaxCorner.X))
					{
						WallNormal = { 0, 1 };
						HitHighEntityIndex = TestHighEntityIndex;
					}
				}
			}
		}

		E.High->Position += MinT * PlayerDelta;

		if (HitHighEntityIndex)
		{
			E.High->Velocity -= WallNormal * Dot(E.High->Velocity, WallNormal);
			PlayerDelta = DesiredPosition - E.High->Position;
			PlayerDelta -= WallNormal * Dot(PlayerDelta, WallNormal);

			HighF_Entity *HitHigh = &State->HighEntities_[HitHighEntityIndex];
			LowF_Entity *HitLow = &State->LowEntities[HitHigh->LowEntityIndex];
			E.High->AbsTileZ += HitLow->dAbsTileZ;
		}
		else
		{
			break;
		}
	}

	E.Low->Position = MapIntoTileSpace(State->W, State->CameraPosition, E.High->Position);
}

static void SetCamera(GameState *State, World_Position NewCameraPosition)
{
	World_Position_Difference dCameraP = Subtract(State->W, &NewCameraPosition, &State->CameraPosition);
	State->CameraPosition = NewCameraPosition;
	
	int32 TileSpanX = 17 * 3;
	int32 TileSpanY = 9 * 3;
	Rectangle CameraBounds = RectCenterDim(Vector(0, 0), 
		State->W->TileSideInMeters * Vector(static_cast<float>(TileSpanX), static_cast<float>(TileSpanY)));
	Vector EntityOffsetForFrame = -dCameraP.dXY;

	OffsetAndCheckFrequencyByArea(State, EntityOffsetForFrame, CameraBounds);

	int32 MinTileX = NewCameraPosition.AbsTileX - TileSpanX / 2;
	int32 MaxTileX = NewCameraPosition.AbsTileX + TileSpanX / 2;
	int32 MinTileY = NewCameraPosition.AbsTileY - TileSpanY / 2;
	int32 MaxTileY = NewCameraPosition.AbsTileY + TileSpanY / 2;
	for (uint32 EntityIndex = 1; EntityIndex < State->LowEntityCount; ++EntityIndex)
	{
		LowF_Entity *Low = &State->LowEntities[EntityIndex];
		if (Low->HighEntityIndex == 0)
		{
			if (Low->Position.AbsTileZ == NewCameraPosition.AbsTileZ &&
				Low->Position.AbsTileX >= MinTileX &&
				Low->Position.AbsTileX <= MaxTileX &&
				Low->Position.AbsTileY >= MinTileY &&
				Low->Position.AbsTileY <= MaxTileY)
			{
				MakeEntityHighFrequency(State, EntityIndex);
			}
		}
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
		AddLowEntity(State, Entity_Type_Null);	// Null entity for the index 0
		State->HighEntityCount = 1;

		State->Background = LoadBMP("assets/testImage.bmp");
		State->PlayerSprite = LoadBMP("assets/Player.bmp");

		InitializeArena(&State->WorldArena, Memory->PermanentStorageSize - sizeof(GameState), 
			reinterpret_cast<uint8 *>(Memory->PermanentStorage) + sizeof(GameState));

		// World construction
		State->W = reinterpret_cast<World *>(PushSize(&State->WorldArena, sizeof(World)));
		InitializeWorld(State->W, 1.4f);

		uint32 ScreenBaseX = 0;
		uint32 ScreenBaseY = 0;
		uint32 ScreenBaseZ = 0;
		uint32 ScreenX = ScreenBaseX;
		uint32 ScreenY = ScreenBaseY;
		uint32 AbsTileZ = ScreenBaseZ;
		bool bDoorLeft = false;
		bool bDoorRight = false;
		bool bDoorTop = false;
		bool bDoorBottom = false;
		bool bDoorUp = false;
		bool bDoorDown = false;
		bool bSwitchedFloor = false;

		for (uint32 ScreenIndex = 0; ScreenIndex < 2; ++ScreenIndex)
		{
			bDoorLeft = bDoorRight;
			bDoorBottom = bDoorTop;
			bDoorRight = false;
			bDoorTop = false;

			uint32 RandomChoice;
			if (1)	//(bDoorUp || bDoorDown)
				RandomChoice = Random() % 2;
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
				if (AbsTileZ == ScreenBaseZ)
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
					if (TileValue == 2)
					{
						AddWall(State, AbsTileX, AbsTileY, AbsTileZ);
					}
				}
			}
			if (RandomChoice == 2)
			{
				AbsTileZ == ScreenBaseZ ? AbsTileZ = ScreenBaseZ + 1 : AbsTileZ = ScreenBaseZ;
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

		World_Position NewCameraPosition = {};
		NewCameraPosition.AbsTileX = ScreenBaseX * TilesPerWidth + 17 / 2;
		NewCameraPosition.AbsTileY = ScreenBaseY * TilesPerHeight + 9 / 2;
		NewCameraPosition.AbsTileZ = ScreenBaseZ;
		SetCamera(State, NewCameraPosition);

		Memory->bIsInitialized = true;
	}

	const uint32 TileSideInPixels = 60;
	const float MetersToPixels = static_cast<float>(TileSideInPixels) / State->W->TileSideInMeters;
	Vector PlayerAcceleration = {};
	static World_Position testPos;

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
	if (Input->E && State->PlayerEntityIndex == 0)
	{
		State->PlayerEntityIndex = AddPlayer(State);
	}

	if (State->PlayerEntityIndex != 0)
	{
		Entity Player = GetHighEntity(State, State->PlayerEntityIndex);
		MovePlayer(State, Player, Input->TimeElapsingOverFrame, PlayerAcceleration);

		World_Position NewCameraPosition = State->CameraPosition;
		NewCameraPosition.AbsTileZ = State->LowEntities[State->PlayerEntityIndex].Position.AbsTileZ;
		if (Player.High->Position.X > (static_cast<float>(1 + TilesPerWidth / 2) * State->W->TileSideInMeters))
			NewCameraPosition.AbsTileX += TilesPerWidth;
		else if (Player.High->Position.X < (-static_cast<float>(1 + TilesPerWidth / 2) * State->W->TileSideInMeters))
			NewCameraPosition.AbsTileX -= TilesPerWidth;
		if (Player.High->Position.Y > (static_cast<float>(1 + TilesPerHeight / 2) * State->W->TileSideInMeters))
			NewCameraPosition.AbsTileY += TilesPerHeight;
		else if (Player.High->Position.Y < (-static_cast<float>(1 + TilesPerHeight / 2) * State->W->TileSideInMeters))
			NewCameraPosition.AbsTileY -= TilesPerHeight;

		SetCamera(State, NewCameraPosition);
	}

	DrawBitmap(Buffer, &State->Background, 0, 0);

	for (uint32 HighEntityIndex = 1; HighEntityIndex < State->HighEntityCount; ++HighEntityIndex)
	{
		HighF_Entity *HighEntity = &State->HighEntities_[HighEntityIndex];
		LowF_Entity *LowEntity = &State->LowEntities[HighEntity->LowEntityIndex];

		if (LowEntity->Type == Entity_Type_Player)
		{
			float GroundPointX = 0.5f * Buffer->BitmapWidth + HighEntity->Position.X * MetersToPixels;
			float GroundPointY = 0.5f * Buffer->BitmapHeight - HighEntity->Position.Y * MetersToPixels ;
			Vector EntityWidthHeigh = Vector(LowEntity->Width, LowEntity->Height);
			Vector TopLeft = Vector(GroundPointX - 0.5f * MetersToPixels * LowEntity->Width,
				GroundPointY - 0.5f * MetersToPixels * LowEntity->Height);
			DrawRectangle(Buffer, TopLeft, TopLeft + MetersToPixels * EntityWidthHeigh, 1.f, 0.f, 0.f);

			float X = 0.5f * Buffer->BitmapWidth + HighEntity->Position.X * MetersToPixels;
			float Y = 0.5f * Buffer->BitmapHeight - HighEntity->Position.Y * MetersToPixels;// -HighEntity->Z * MetersToPixels;
			DrawBitmap(Buffer, &State->PlayerSprite, X, Y, 30.0f / 2.0f, 35.0f);
		} 
		else
		{
			float GroundPointX = 0.5f * Buffer->BitmapWidth + MetersToPixels * HighEntity->Position.X;
			float GroundPointY = 0.5f * Buffer->BitmapHeight - MetersToPixels * HighEntity->Position.Y;
			Vector EntityWidthHeigh = Vector(LowEntity->Width, LowEntity->Height);
			Vector TopLeft = Vector(GroundPointX - 0.5f * MetersToPixels * LowEntity->Width,
									GroundPointY - 0.5f * MetersToPixels * LowEntity->Height);
			DrawRectangle(Buffer, TopLeft, TopLeft + MetersToPixels * EntityWidthHeigh, 1.f, 1.f, 0.f);
		}
	}
}

void GameGetSoundSamples(/*ThreadContext *Thread,*/ SoundBuffer *SBuffer/*, GameMemory *Memory*/)
{
	//GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	OutputSound(SBuffer, 400);
}
