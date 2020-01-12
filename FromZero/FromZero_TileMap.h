#pragma once

#include "MemoryArena.h"
#include "Vector.h"
#include "FromZero_intrinsics.h"

struct Tile_Chunk_Position
{
	int32 TileChunkX;
	int32 TileChunkY;
	int32 TileChunkZ;

	int32 ChunkRelTileX;
	int32 ChunkRelTileY;
};

struct Tile_Chunk
{
	int32 TileChunkX;
	int32 TileChunkY;
	int32 TileChunkZ;

	uint32 *Tiles;

	Tile_Chunk *NextInHash;
};

struct Tile_Map
{
	int32 ChunkShift;
	int32 ChunkMask;
	int32 ChunkDimension;
	
	float TileSideInMeters;

	Tile_Chunk TileChunkHash[4096];
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
	int32 AbsTileX;
	int32 AbsTileY;
	int32 AbsTileZ;

	Vector Offset;
};

bool WorldIsEmptyAtPosition(Tile_Map *TileMap, TileMap_Position *Pos);
void SetTileValue(Memory_Arena *Arena, Tile_Map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ, uint32 TileValue);
TileMap_Position MapIntoTileSpace(Tile_Map *TileMap, TileMap_Position BasePos, Vector Offset);
uint32 GetTileValue(Tile_Map *TileMap, TileMap_Position *Position);
uint32 GetTileValue(Tile_Map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ);
bool PositionsAreOnTheSameTile(TileMap_Position *Position1, TileMap_Position *Position2);
TileMap_Difference Subtract(Tile_Map *TileMap, TileMap_Position *A, TileMap_Position *B);
void InitializeTileMap(Tile_Map *TileMap, float TileSideInMeters);