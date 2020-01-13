#include "FromZero_World.h"
#include "FromZero_intrinsics.h"

#define TileChunk_Safe_Margin (INT32_MAX / 64)
#define TileChunk_Uninitialized INT32_MAX

static void CanonicalizeCoord(World *W, int32 *Tile, float *TileRel)
{
	int32 Offset = RoundFloatToINT32(*TileRel / W->TileSideInMeters);
	*Tile += Offset;
	*TileRel -= Offset * W->TileSideInMeters;

	assert(*TileRel > -0.5f * W->TileSideInMeters);
	assert(*TileRel <  0.5f * W->TileSideInMeters);
}

static World_Chunk * GetTileChunk(World *W, int32 TileChunkX, int32 TileChunkY, int32 TileChunkZ, Memory_Arena *Arena = 0)
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

World_Position MapIntoTileSpace(World *W, World_Position BasePos, Vector Offset)
{
	World_Position Result = BasePos;

	Result.Offset += Offset;
	CanonicalizeCoord(W, &Result.AbsTileX, &Result.Offset.X);
	CanonicalizeCoord(W, &Result.AbsTileY, &Result.Offset.Y);

	return Result;
}

bool PositionsAreOnTheSameTile(World_Position *Position1, World_Position *Position2)
{
	return Position1->AbsTileX == Position2->AbsTileX && Position1->AbsTileY == Position2->AbsTileY && Position1->AbsTileZ == Position2->AbsTileZ;
}

World_Position_Difference Subtract(World *W, World_Position *A, World_Position *B)
{
	World_Position_Difference Result;

	Vector dTileXY = {static_cast<float>(A->AbsTileX) - static_cast<float>(B->AbsTileX), static_cast<float>(A->AbsTileY) - static_cast<float>(B->AbsTileY)};
	Result.dXY = dTileXY * W->TileSideInMeters + (A->Offset - B->Offset);
	Result.dZ = W->TileSideInMeters * (static_cast<float>(A->AbsTileZ) - static_cast<float>(B->AbsTileZ));

	return Result;
}

void InitializeWorld(World *W, float TileSideInMeters)
{
	W->ChunkShift = 4;
	W->ChunkMask = (1 << W->ChunkShift) - 1;
	W->ChunkDimension = 1 << W->ChunkShift;
	W->TileSideInMeters = TileSideInMeters;

	for (uint32 TileChunkIndex = 0; TileChunkIndex < (sizeof(W->WorldChunkHash) / sizeof(*W->WorldChunkHash)); ++TileChunkIndex)
	{	// Marking as empty
		W->WorldChunkHash[TileChunkIndex].ChunkX = TileChunk_Uninitialized;
	}
}
