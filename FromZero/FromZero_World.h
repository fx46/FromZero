#pragma once

#include "MemoryArena.h"
#include "Vector.h"
#include "FromZero_intrinsics.h"

struct World_Entity_Block
{
	uint32 EntityCount;
	uint32 LowEntityIndex[16];
	World_Entity_Block *Next;
};

struct World_Chunk
{
	int32 ChunkX;
	int32 ChunkY;
	int32 ChunkZ;
	World_Entity_Block FirstBlock;
	World_Chunk *NextInHash;
};

struct World
{
	int32 ChunkShift;
	int32 ChunkMask;
	int32 ChunkDimension;
	float TileSideInMeters;
	World_Chunk WorldChunkHash[4096];
};

struct World_Position_Difference
{
	Vector dXY;
	float dZ;
};

struct World_Position
{
	// Fixed point tile locations, the high bits are the tile chunk index, 
	// and the low bits are the tile index in the chunk.
	int32 AbsTileX;
	int32 AbsTileY;
	int32 AbsTileZ;

	Vector Offset;
};

World_Position MapIntoTileSpace(World *W, World_Position BasePos, Vector Offset);
bool PositionsAreOnTheSameTile(World_Position *Position1, World_Position *Position2);
World_Position_Difference Subtract(World *W, World_Position *A, World_Position *B);
void InitializeWorld(World *W, float TileSideInMeters);