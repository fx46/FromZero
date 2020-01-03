#include "FromZero_TileMap.h"
#include "FromZero_intrinsics.h"

static void CanonicalizeCoord(Tile_Map *TileMap, UINT32 *Tile, float *TileRel)
{
	INT32 Offset = RoundFloatToINT32(*TileRel / TileMap->TileSideInMeters);
	*Tile += Offset;
	*TileRel -= Offset * TileMap->TileSideInMeters;

	assert(*TileRel > -0.5001f * TileMap->TileSideInMeters);
	assert(*TileRel <  0.5001f * TileMap->TileSideInMeters);
}

static Tile_Chunk * GetTileChunk(Tile_Map *TileMap, UINT32 TileChunkX, UINT32 TileChunkY, UINT32 TileChunkZ)
{
	Tile_Chunk *TileChunk = nullptr;

	if (TileChunkX >= 0 && TileChunkX < TileMap->TileChunkCountX && 
		TileChunkY >= 0 && TileChunkY < TileMap->TileChunkCountY &&
		TileChunkZ >= 0 && TileChunkZ < TileMap->TileChunkCountZ)
	{
		TileChunk = &TileMap->TileChunks[TileChunkZ * TileMap->TileChunkCountY * TileMap->TileChunkCountX
										+TileChunkY * TileMap->TileChunkCountX
										+TileChunkX];
	}

	return TileChunk;
}

static UINT32 GetTileValueUnchecked(Tile_Map *TileMap, Tile_Chunk *TileChunk, UINT32 TileX, UINT32 TileY)
{
	assert(TileChunk);
	assert(TileX < TileMap->ChunkDimension);
	assert(TileY < TileMap->ChunkDimension);

	return TileChunk->Tiles[TileY * TileMap->ChunkDimension + TileX];
}

static void SetTileValueUnchecked(Tile_Map *TileMap, Tile_Chunk *TileChunk, UINT32 TileX, UINT32 TileY, UINT32 TileValue)
{
	assert(TileChunk);
	assert(TileX < TileMap->ChunkDimension);
	assert(TileY < TileMap->ChunkDimension);

	TileChunk->Tiles[TileY * TileMap->ChunkDimension + TileX] = TileValue;
}

static UINT32 GetTileValue(Tile_Map *TileMap, Tile_Chunk *TileChunk, UINT32 TestTileX, UINT32 TestTileY)
{
	UINT32 TileChunkValue = 0;

	if (TileChunk && TileChunk->Tiles)
	{
		TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
	}

	return TileChunkValue;
}

static void _SetTileValue(Tile_Map *TileMap, Tile_Chunk *TileChunk, UINT32 TestTileX, UINT32 TestTileY, UINT32 TileValue)
{
	if (TileChunk && TileChunk->Tiles)
	{
		SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY, TileValue);
	}
}

static Tile_Chunk_Position GetChunkPositionFor(Tile_Map *TileMap, UINT32 AbsTileX, UINT32 AbsTileY, UINT32 AbsTileZ)
{
	Tile_Chunk_Position Result;

	Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
	Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
	Result.TileChunkZ = AbsTileZ;
	Result.ChunkRelTileX = AbsTileX & TileMap->ChunkMask;
	Result.ChunkRelTileY = AbsTileY & TileMap->ChunkMask;

	return Result;
}

TileMap_Position CanonicalizePosition(Tile_Map *TileMap, TileMap_Position Pos)
{
	TileMap_Position Result = Pos;

	CanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.Offset.X);
	CanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Offset.Y);

	return Result;
}

UINT32 GetTileValue(Tile_Map *TileMap, TileMap_Position *Position)
{
	return GetTileValue(TileMap, Position->AbsTileX, Position->AbsTileY, Position->AbsTileZ);
}

bool WorldIsEmptyAtPosition(Tile_Map *TileMap, TileMap_Position *Pos)
{
	UINT32 TileChunkValue = GetTileValue(TileMap, Pos);

	return TileChunkValue == 1 || TileChunkValue == 3 || TileChunkValue == 4;
}

UINT32 GetTileValue(Tile_Map *TileMap, UINT32 AbsTileX, UINT32 AbsTileY, UINT32 AbsTileZ)
{
	Tile_Chunk_Position ChunkPosition = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
	Tile_Chunk *TileChunk = GetTileChunk(TileMap, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY, ChunkPosition.TileChunkZ);
	UINT32 TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPosition.ChunkRelTileX, ChunkPosition.ChunkRelTileY);

	return TileChunkValue;
}

void SetTileValue(Memory_Arena *Arena, Tile_Map *TileMap, UINT32 AbsTileX, UINT32 AbsTileY, UINT32 AbsTileZ, UINT32 TileValue)
{
	Tile_Chunk_Position ChunkPosition = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
	if (Tile_Chunk *TileChunk = GetTileChunk(TileMap, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY, ChunkPosition.TileChunkZ))
	{
		assert(ChunkPosition.ChunkRelTileX < TileMap->ChunkDimension && ChunkPosition.ChunkRelTileY < TileMap->ChunkDimension);

		if (!TileChunk->Tiles)
		{
			UINT32 TileCount = TileMap->ChunkDimension * TileMap->ChunkDimension;
			TileChunk->Tiles = reinterpret_cast<UINT32 *>(PushArray(Arena, TileCount, sizeof(UINT32)));
			for (UINT32 TileIndex = 0; TileIndex < TileCount; ++TileIndex)
			{
				TileChunk->Tiles[TileIndex] = 1;
			}
		}

		_SetTileValue(TileMap, TileChunk, ChunkPosition.ChunkRelTileX, ChunkPosition.ChunkRelTileY, TileValue);
	}
}

bool PositionsAreOnTheSameTile(TileMap_Position *Position1, TileMap_Position *Position2)
{
	return Position1->AbsTileX == Position2->AbsTileX && Position1->AbsTileY == Position2->AbsTileY && Position1->AbsTileZ == Position2->AbsTileZ;
}

TileMap_Difference Substract(Tile_Map *TileMap, TileMap_Position *A, TileMap_Position *B)
{
	TileMap_Difference Result;

	Vector dTileXY = {static_cast<float>(A->AbsTileX) - static_cast<float>(B->AbsTileX), static_cast<float>(A->AbsTileY) - static_cast<float>(B->AbsTileY)};
	Result.dXY = dTileXY * TileMap->TileSideInMeters + (A->Offset - B->Offset);
	Result.dZ = TileMap->TileSideInMeters * (static_cast<float>(A->AbsTileZ) - static_cast<float>(B->AbsTileZ));

	return Result;
}

TileMap_Position Offset(Tile_Map *TileMap, TileMap_Position P, Vector Offset)
{
	P.Offset += Offset;
	return CanonicalizePosition(TileMap, P);
}