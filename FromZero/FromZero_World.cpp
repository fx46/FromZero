#include "FromZero_World.h"
#include "FromZero_intrinsics.h"

#define TileChunk_Safe_Margin (INT32_MAX / 64)
#define TileChunk_Uninitialized INT32_MAX
#define Tiles_Per_Chunk 16

static bool IsCanonical(World *W, float TileRel)
{
	return TileRel >= -0.5f * W->ChunkSideInMeters && TileRel <=  0.5f * W->ChunkSideInMeters;
}

static bool IsCanonical(World *W, Vector Offset)
{
	return IsCanonical(W, Offset.X) && IsCanonical(W, Offset.Y);
}

static void ReCanonicalizeCoord(World *W, int32 *Tile, float *TileRel)
{
	int32 Offset = RoundFloatToINT32(*TileRel / W->ChunkSideInMeters);
	*Tile += Offset;
	*TileRel -= Offset * W->ChunkSideInMeters;

	assert(IsCanonical(W, *TileRel));
}

World_Chunk * GetWorldChunk(World *W, int32 TileChunkX, int32 TileChunkY, int32 TileChunkZ, Memory_Arena *Arena)
{
	assert(TileChunkX > -TileChunk_Safe_Margin);
	assert(TileChunkY > -TileChunk_Safe_Margin);
	assert(TileChunkZ > -TileChunk_Safe_Margin);
	assert(TileChunkX < TileChunk_Safe_Margin);
	assert(TileChunkY < TileChunk_Safe_Margin);
	assert(TileChunkZ < TileChunk_Safe_Margin);

	uint32 HashValue = 19 * TileChunkX + 7 * TileChunkY + 3 * TileChunkZ;
	uint32 HashSlot = HashValue & ((sizeof(W->WorldChunkHash) / sizeof(*W->WorldChunkHash)) - 1);
	assert(HashSlot < (sizeof(W->WorldChunkHash) / sizeof(*W->WorldChunkHash)));

	World_Chunk *Chunk = &W->WorldChunkHash[HashSlot];
	do 
	{
		if (TileChunkX == Chunk->ChunkX && TileChunkY == Chunk->ChunkY && TileChunkZ == Chunk->ChunkZ)
		{
			break;
		}
		if (Arena && Chunk->ChunkX != TileChunk_Uninitialized && !Chunk->NextInHash)
		{
			Chunk->NextInHash = reinterpret_cast<World_Chunk *>(PushSize(Arena, sizeof(World_Chunk)));
			Chunk = Chunk->NextInHash;
			Chunk->ChunkX = TileChunk_Uninitialized;
		}
		if (Arena && Chunk->ChunkX == TileChunk_Uninitialized)
		{
			Chunk->ChunkX = TileChunkX;
			Chunk->ChunkY = TileChunkY;
			Chunk->ChunkZ = TileChunkZ;
			Chunk->NextInHash = 0;

			break;
		}

		Chunk = Chunk->NextInHash;
	} while (Chunk);

	return Chunk;
}

World_Position MapIntoChunkSpace(World *W, World_Position BasePos, Vector Offset)
{
	World_Position Result = BasePos;

	Result.Offset += Offset;
	ReCanonicalizeCoord(W, &Result.ChunkX, &Result.Offset.X);
	ReCanonicalizeCoord(W, &Result.ChunkY, &Result.Offset.Y);

	return Result;
}

bool PositionsAreOnTheSameTile(World_Position *Position1, World_Position *Position2)
{
	return Position1->ChunkX == Position2->ChunkX && Position1->ChunkY == Position2->ChunkY && Position1->ChunkZ == Position2->ChunkZ;
}

World_Position ChunkPositionFromTilePosition(World *W, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ)
{
	World_Position Result = {};

	Result.ChunkX = AbsTileX / Tiles_Per_Chunk;
	Result.ChunkY = AbsTileY / Tiles_Per_Chunk;
	Result.ChunkZ = AbsTileZ / Tiles_Per_Chunk;
	Result.Offset.X = static_cast<float>(AbsTileX - Result.ChunkX * Tiles_Per_Chunk) * W->TileSideInMeters;
	Result.Offset.Y = static_cast<float>(AbsTileY - Result.ChunkY * Tiles_Per_Chunk) * W->TileSideInMeters;

	return Result;
}

World_Position_Difference Subtract(World *W, World_Position *A, World_Position *B)
{
	World_Position_Difference Result;

	Vector dTileXY = {static_cast<float>(A->ChunkX) - static_cast<float>(B->ChunkX), static_cast<float>(A->ChunkY) - static_cast<float>(B->ChunkY)};
	Result.dXY = dTileXY * W->ChunkSideInMeters + (A->Offset - B->Offset);
	Result.dZ = W->ChunkSideInMeters * (static_cast<float>(A->ChunkZ) - static_cast<float>(B->ChunkZ));

	return Result;
}

void InitializeWorld(World *W, float TileSideInMeters)
{
	W->TileSideInMeters = TileSideInMeters;
	W->ChunkSideInMeters = Tiles_Per_Chunk * TileSideInMeters;
	W->FirstFree = 0;

	for (uint32 ChunkIndex = 0; ChunkIndex < (sizeof(W->WorldChunkHash) / sizeof(*W->WorldChunkHash)); ++ChunkIndex)
	{	// Marking as empty
		W->WorldChunkHash[ChunkIndex].ChunkX = TileChunk_Uninitialized;
		W->WorldChunkHash[ChunkIndex].FirstBlock.EntityCount = 0;
	}
}

static bool AreInSameChunk(World *W, World_Position *P1, World_Position *P2)
{
	assert(IsCanonical(W, P1->Offset));
	assert(IsCanonical(W, P2->Offset));
	return P1->ChunkX == P2->ChunkX && P1->ChunkY == P2->ChunkY && P1->ChunkZ == P2->ChunkZ;
}

void ChangeEntityLocation(Memory_Arena *Arena, World *W, uint32 LowEntityIndex, World_Position *OldPosition, World_Position *NewPosition)
{
	if (OldPosition && AreInSameChunk(W, OldPosition, NewPosition))
	{

	}
	else
	{
		if (OldPosition)
		{
			World_Chunk *Chunk = GetWorldChunk(W, OldPosition->ChunkX, OldPosition->ChunkY, OldPosition->ChunkZ);
			assert(Chunk);
			if (Chunk)
			{
				bool NotFound = true;
				World_Entity_Block *FirstBlock = &Chunk->FirstBlock;
				for (World_Entity_Block *Block = &Chunk->FirstBlock; Block && NotFound; Block = Block->Next)
				{
					for (uint32 Index = 0; Index < Block->EntityCount && NotFound; ++Index)
					{
						if (Block->LowEntityIndex[Index] == LowEntityIndex)
						{
							Block->LowEntityIndex[Index] = FirstBlock->LowEntityIndex[--FirstBlock->EntityCount];
							if (FirstBlock->EntityCount == 0)
							{
								if (FirstBlock->Next)
								{
									World_Entity_Block *NextBlock = FirstBlock->Next;
									*FirstBlock = *NextBlock;

									NextBlock->Next = W->FirstFree;
									W->FirstFree = NextBlock;
								}
							}

							NotFound = false;
						}
					}
				}
			}
		}

		World_Chunk *Chunk = GetWorldChunk(W, NewPosition->ChunkX, NewPosition->ChunkY, NewPosition->ChunkZ, Arena);
		World_Entity_Block *Block = &Chunk->FirstBlock;
		if (Block->EntityCount == (sizeof(Block->LowEntityIndex) / sizeof(*Block->LowEntityIndex)))
		{
			World_Entity_Block *OldBlock = W->FirstFree;
			if (OldBlock)
			{
				W->FirstFree = OldBlock->Next;
			} 
			else
			{
				OldBlock = reinterpret_cast<World_Entity_Block *>(PushSize(Arena, sizeof(World_Entity_Block)));
			}
			*OldBlock = *Block;
			Block->Next = OldBlock;
			Block->EntityCount = 0;
		}

		assert(Block->EntityCount < (sizeof(Block->LowEntityIndex) / sizeof(*Block->LowEntityIndex)));
		Block->LowEntityIndex[Block->EntityCount++] = LowEntityIndex;
	}
}
