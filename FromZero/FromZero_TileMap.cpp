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

static Tile_Chunk * GetTileChunk(Tile_Map *TileMap, int TileChunkX, int TileChunkY)
{
	Tile_Chunk *TileChunk = nullptr;

	if (TileChunkX >= 0 && TileChunkX < TileMap->TileChunkCountX && TileChunkY >= 0 && TileChunkY < TileMap->TileChunkCountY)
	{
		TileChunk = &TileMap->TileChunks[TileChunkY * TileMap->ChunkDimension + TileChunkX];
	}

	return TileChunk;
}

static UINT32 GetTileValueUnchecked(Tile_Map *TileMap, Tile_Chunk *TileChunk, UINT32 X, UINT32 Y)
{
	assert(TileChunk)
		assert(X < TileMap->ChunkDimension && Y < TileMap->ChunkDimension)

		return TileChunk->Tiles[Y * TileMap->ChunkDimension + X];
}

static UINT32 GetTileValue(Tile_Map *TileMap, Tile_Chunk *TileChunk, UINT32 TestTileX, UINT32 TestTileY)
{
	UINT32 TileChunkValue = 0;

	if (TileChunk)
	{
		TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
	}

	return TileChunkValue;
}

static Tile_Chunk_Position GetChunkPositionFor(Tile_Map *TileMap, UINT32 AbsTileX, UINT32 AbsTileY)
{
	Tile_Chunk_Position Result;

	Result.TileChunkX = AbsTileX >> TileMap->ChunkShift; // getting the 24 leftmost bits
	Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
	Result.ChunkRelTileX = AbsTileX & TileMap->ChunkMask; // getting the 8 rightmost bits
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

	return TileChunkValue == 0;
}