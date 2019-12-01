#include "FromZero_TileMap.h"
#include "FromZero.h"
#include "FromZero_intrinsics.h"

static void CanonicalizeCoord(Tile_Map *TileMap, UINT32 *Tile, float *TileRel)
{
	INT32 Offset = RoundFloatToINT32(*TileRel / TileMap->TileSideInMeters);
	*Tile += Offset;
	*TileRel -= Offset * TileMap->TileSideInMeters;

	assert(*TileRel >= -TileMap->TileSideInMeters / 2);
	assert(*TileRel <= TileMap->TileSideInMeters / 2);
}

static TileMap_Position CanonicalizePosition(Tile_Map *TileMap, TileMap_Position Pos)
{
	CanonicalizeCoord(TileMap, &Pos.AbsTileX, &Pos.TileRelX);
	CanonicalizeCoord(TileMap, &Pos.AbsTileY, &Pos.TileRelY);

	return Pos;
}

static Tile_Chunk * GetTileChunk(Tile_Map *TileMap, UINT32 TileChunkX, UINT32 TileChunkY)
{
	Tile_Chunk *TileChunk = nullptr;

	if (TileChunkX >= 0 && TileChunkX < TileMap->TileChunkCountX && TileChunkY >= 0 && TileChunkY < TileMap->TileChunkCountY)
	{
		TileChunk = &TileMap->TileChunks[TileChunkY * TileMap->TileChunkCountX + TileChunkX];
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

static void SetTileValue(Tile_Map *TileMap, Tile_Chunk *TileChunk, UINT32 TestTileX, UINT32 TestTileY, UINT32 TileValue)
{
	if (TileChunk && TileChunk->Tiles)
	{
		SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY, TileValue);
	}
}

static Tile_Chunk_Position GetChunkPositionFor(Tile_Map *TileMap, UINT32 AbsTileX, UINT32 AbsTileY)
{
	Tile_Chunk_Position Result;

	Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
	Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
	Result.ChunkRelTileX = AbsTileX & TileMap->ChunkMask;
	Result.ChunkRelTileY = AbsTileY & TileMap->ChunkMask;

	return Result;
}

static UINT32 GetTileValue(Tile_Map *TileMap, UINT32 AbsTileX, UINT32 AbsTileY)
{
	Tile_Chunk_Position ChunkPosition = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY);
	Tile_Chunk *TileChunk = GetTileChunk(TileMap, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY);
	UINT32 TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPosition.ChunkRelTileX, ChunkPosition.ChunkRelTileY);

	return TileChunkValue;
}

static bool WorldIsEmptyAtPosition(Tile_Map *TileMap, TileMap_Position Pos)
{
	UINT32 TileChunkValue = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY);

	return TileChunkValue == 1;
}

static void SetTileValue(Memory_Arena *Arena, Tile_Map *TileMap, UINT32 AbsTileX, UINT32 AbsTileY, UINT32 TileValue)
{
	Tile_Chunk_Position ChunkPosition = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY);
	if (Tile_Chunk *TileChunk = GetTileChunk(TileMap, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY))
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
		//day 35 30:00
		SetTileValue(TileMap, TileChunk, ChunkPosition.ChunkRelTileX, ChunkPosition.ChunkRelTileY, TileValue);
	}
}
