#pragma once

#include <windows.h>
#include "MemoryArena.h"

struct Tile_Chunk_Position
{
	UINT32 TileChunkX;
	UINT32 TileChunkY;
	UINT32 TileChunkZ;

	UINT32 ChunkRelTileX;
	UINT32 ChunkRelTileY;
};

struct Tile_Chunk
{
	UINT32 *Tiles;
};

struct Tile_Map
{
	Tile_Chunk *TileChunks;
	UINT32 ChunkShift;
	UINT32 ChunkMask;
	UINT32 ChunkDimension;
	UINT32 TileChunkCountX;
	UINT32 TileChunkCountY;
	UINT32 TileChunkCountZ;
	float TileSideInMeters;
};

struct TileMap_Position
{
	// Fixed point tile locations, the high bits are the tile chunk index, 
	// and the low bits are the tile index in the chunk.
	UINT32 AbsTileX;
	UINT32 AbsTileY;
	UINT32 AbsTileZ;

	float TileRelX;
	float TileRelY;
};

bool WorldIsEmptyAtPosition(Tile_Map *TileMap, TileMap_Position Pos);
void SetTileValue(Memory_Arena *Arena, Tile_Map *TileMap, UINT32 AbsTileX, UINT32 AbsTileY, UINT32 AbsTileZ, UINT32 TileValue);
TileMap_Position CanonicalizePosition(Tile_Map *TileMap, TileMap_Position Pos);
UINT32 GetTileValue(Tile_Map *TileMap, UINT32 AbsTileX, UINT32 AbsTileY, UINT32 AbsTileZ);