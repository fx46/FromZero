#pragma once

#include <windows.h>

struct Tile_Chunk_Position
{
	UINT32 TileChunkX;
	UINT32 TileChunkY;

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
	float TileSideInMeters;
};

struct TileMap_Position
{
	// Fixed point tile locations, the high bits are the tile chunk index, 
	// and the low bits are the tile index in the chunk.
	UINT32 AbsTileX;
	UINT32 AbsTileY;

	float TileRelX;
	float TileRelY;
};