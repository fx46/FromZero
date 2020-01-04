#pragma once

#include "MemoryArena.h"
#include "Vector.h"
#include "FromZero_intrinsics.h"

struct Tile_Chunk_Position
{
	uint32 TileChunkX;
	uint32 TileChunkY;
	uint32 TileChunkZ;

	uint32 ChunkRelTileX;
	uint32 ChunkRelTileY;
};

struct Tile_Chunk
{
	uint32 *Tiles;
};

struct Tile_Map
{
	Tile_Chunk *TileChunks;
	uint32 ChunkShift;
	uint32 ChunkMask;
	uint32 ChunkDimension;
	uint32 TileChunkCountX;
	uint32 TileChunkCountY;
	uint32 TileChunkCountZ;
	float TileSideInMeters;
};

struct TileMap_Difference
{
	Vector dXY;
	float dZ;
};

struct TileMap_Position
{
	// Fixed point tile locations, the high bits are the tile chunk index, 
	// and the low bits are the tile index in the chunk.
	uint32 AbsTileX;
	uint32 AbsTileY;
	uint32 AbsTileZ;

	Vector Offset;
};

bool WorldIsEmptyAtPosition(Tile_Map *TileMap, TileMap_Position *Pos);
void SetTileValue(Memory_Arena *Arena, Tile_Map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ, uint32 TileValue);
TileMap_Position CanonicalizePosition(Tile_Map *TileMap, TileMap_Position Pos);
uint32 GetTileValue(Tile_Map *TileMap, TileMap_Position *Position);
uint32 GetTileValue(Tile_Map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ);
bool PositionsAreOnTheSameTile(TileMap_Position *Position1, TileMap_Position *Position2);
TileMap_Difference Substract(Tile_Map *TileMap, TileMap_Position *A, TileMap_Position *B);
TileMap_Position Offset(Tile_Map *TileMap, TileMap_Position P, Vector Offset);