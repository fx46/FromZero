#include "FromZero_TileMap.h"
#include "FromZero_intrinsics.h"

#define TileChunk_Safe_Margin (INT32_MAX / 64)
#define TileChunk_Uninitialized INT32_MAX

static void CanonicalizeCoord(Tile_Map *TileMap, int32 *Tile, float *TileRel)
{
	int32 Offset = RoundFloatToINT32(*TileRel / TileMap->TileSideInMeters);
	*Tile += Offset;
	*TileRel -= Offset * TileMap->TileSideInMeters;

	assert(*TileRel > -0.5f * TileMap->TileSideInMeters);
	assert(*TileRel <  0.5f * TileMap->TileSideInMeters);
}

static Tile_Chunk * GetTileChunk(Tile_Map *TileMap, int32 TileChunkX, int32 TileChunkY, int32 TileChunkZ, Memory_Arena *Arena = 0)
{
	assert(TileChunkX > -TileChunk_Safe_Margin);
	assert(TileChunkY > -TileChunk_Safe_Margin);
	assert(TileChunkZ > -TileChunk_Safe_Margin);
	assert(TileChunkX < TileChunk_Safe_Margin);
	assert(TileChunkY < TileChunk_Safe_Margin);
	assert(TileChunkZ < TileChunk_Safe_Margin);

	uint32 HashValue = 19 * TileChunkX + 7 * TileChunkY + 3 * TileChunkZ;
	uint32 HashSlot = HashValue & ((sizeof(TileMap->TileChunkHash) / sizeof(*TileMap->TileChunkHash)) - 1);
	assert(HashSlot < (sizeof(TileMap->TileChunkHash) / sizeof(*TileMap->TileChunkHash)));

	Tile_Chunk *Chunk = &TileMap->TileChunkHash[HashSlot];
	do 
	{
		if (TileChunkX == Chunk->TileChunkX && TileChunkY == Chunk->TileChunkY && TileChunkZ == Chunk->TileChunkZ)
		{
			break;
		}
		if (Arena && Chunk->TileChunkX != TileChunk_Uninitialized && !Chunk->NextInHash)
		{
			Chunk->NextInHash = reinterpret_cast<Tile_Chunk *>(PushSize(Arena, sizeof(Tile_Chunk)));
			Chunk = Chunk->NextInHash;
			Chunk->TileChunkX = TileChunk_Uninitialized;
		}
		if (Arena && Chunk->TileChunkX == TileChunk_Uninitialized)
		{
			uint32 TileCount = TileMap->ChunkDimension * TileMap->ChunkDimension;

			Chunk->TileChunkX = TileChunkX;
			Chunk->TileChunkY = TileChunkY;
			Chunk->TileChunkZ = TileChunkZ;

			Chunk->Tiles = reinterpret_cast<uint32 *>(PushArray(Arena, TileCount, sizeof(uint32)));
			for (uint32 TileIndex = 0; TileIndex < TileCount; ++TileIndex)
			{
				Chunk->Tiles[TileIndex] = 1;
			}

			Chunk->NextInHash = 0;

			break;
		}

		Chunk = Chunk->NextInHash;
	} while (Chunk);

	return Chunk;
}

static uint32 GetTileValueUnchecked(Tile_Map *TileMap, Tile_Chunk *TileChunk, int32 TileX, int32 TileY)
{
	assert(TileChunk);
	assert(TileX < TileMap->ChunkDimension);
	assert(TileY < TileMap->ChunkDimension);

	return TileChunk->Tiles[TileY * TileMap->ChunkDimension + TileX];
}

static void SetTileValueUnchecked(Tile_Map *TileMap, Tile_Chunk *TileChunk, int32 TileX, int32 TileY, int32 TileValue)
{
	assert(TileChunk);
	assert(TileX < TileMap->ChunkDimension);
	assert(TileY < TileMap->ChunkDimension);

	TileChunk->Tiles[TileY * TileMap->ChunkDimension + TileX] = TileValue;
}

static uint32 GetTileValue(Tile_Map *TileMap, Tile_Chunk *TileChunk, uint32 TestTileX, uint32 TestTileY)
{
	uint32 TileChunkValue = 0;

	if (TileChunk && TileChunk->Tiles)
	{
		TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
	}

	return TileChunkValue;
}

static void SetTileValue(Tile_Map *TileMap, Tile_Chunk *TileChunk, uint32 TestTileX, uint32 TestTileY, uint32 TileValue)
{
	if (TileChunk && TileChunk->Tiles)
	{
		SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY, TileValue);
	}
}

static Tile_Chunk_Position GetChunkPositionFor(Tile_Map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	Tile_Chunk_Position Result;

	Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
	Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
	Result.TileChunkZ = AbsTileZ;
	Result.ChunkRelTileX = AbsTileX & TileMap->ChunkMask;
	Result.ChunkRelTileY = AbsTileY & TileMap->ChunkMask;

	return Result;
}

TileMap_Position MapIntoTileSpace(Tile_Map *TileMap, TileMap_Position BasePos, Vector Offset)
{
	TileMap_Position Result = BasePos;

	Result.Offset += Offset;
	CanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.Offset.X);
	CanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Offset.Y);

	return Result;
}

uint32 GetTileValue(Tile_Map *TileMap, TileMap_Position *Position)
{
	return GetTileValue(TileMap, Position->AbsTileX, Position->AbsTileY, Position->AbsTileZ);
}

bool WorldIsEmptyAtPosition(Tile_Map *TileMap, TileMap_Position *Pos)
{
	uint32 TileChunkValue = GetTileValue(TileMap, Pos);

	return TileChunkValue == 1 || TileChunkValue == 3 || TileChunkValue == 4;
}

uint32 GetTileValue(Tile_Map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	Tile_Chunk_Position ChunkPosition = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
	Tile_Chunk *TileChunk = GetTileChunk(TileMap, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY, ChunkPosition.TileChunkZ);
	uint32 TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPosition.ChunkRelTileX, ChunkPosition.ChunkRelTileY);

	return TileChunkValue;
}

void SetTileValue(Memory_Arena *Arena, Tile_Map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ, uint32 TileValue)
{
	Tile_Chunk_Position ChunkPosition = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
	Tile_Chunk *TileChunk = GetTileChunk(TileMap, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY, ChunkPosition.TileChunkZ, Arena);
	SetTileValue(TileMap, TileChunk, ChunkPosition.ChunkRelTileX, ChunkPosition.ChunkRelTileY, TileValue);
}

bool PositionsAreOnTheSameTile(TileMap_Position *Position1, TileMap_Position *Position2)
{
	return Position1->AbsTileX == Position2->AbsTileX && Position1->AbsTileY == Position2->AbsTileY && Position1->AbsTileZ == Position2->AbsTileZ;
}

TileMap_Difference Subtract(Tile_Map *TileMap, TileMap_Position *A, TileMap_Position *B)
{
	TileMap_Difference Result;

	Vector dTileXY = {static_cast<float>(A->AbsTileX) - static_cast<float>(B->AbsTileX), static_cast<float>(A->AbsTileY) - static_cast<float>(B->AbsTileY)};
	Result.dXY = dTileXY * TileMap->TileSideInMeters + (A->Offset - B->Offset);
	Result.dZ = TileMap->TileSideInMeters * (static_cast<float>(A->AbsTileZ) - static_cast<float>(B->AbsTileZ));

	return Result;
}

void InitializeTileMap(Tile_Map *TileMap, float TileSideInMeters)
{
	TileMap->ChunkShift = 4;
	TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
	TileMap->ChunkDimension = 1 << TileMap->ChunkShift;
	TileMap->TileSideInMeters = TileSideInMeters;

	for (uint32 TileChunkIndex = 0; TileChunkIndex < (sizeof(TileMap->TileChunkHash) / sizeof(*TileMap->TileChunkHash)); ++TileChunkIndex)
	{
		// Marking as empty
		TileMap->TileChunkHash[TileChunkIndex].TileChunkX = TileChunk_Uninitialized;
	}
}
