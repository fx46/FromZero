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
	float TileSideInMeters;
	float ChunkSideInMeters;
	World_Chunk WorldChunkHash[4096];
	World_Entity_Block *FirstFree;
};

struct World_Position_Difference
{
	Vector dXY;
	float dZ;
};

struct World_Position
{
	int32 ChunkX;
	int32 ChunkY;
	int32 ChunkZ;

	Vector Offset;
};

World_Chunk * GetWorldChunk(World *W, int32 TileChunkX, int32 TileChunkY, int32 TileChunkZ, Memory_Arena *Arena = 0);
World_Position MapIntoChunkSpace(World *W, World_Position BasePos, Vector Offset);
bool PositionsAreOnTheSameTile(World_Position *Position1, World_Position *Position2);
World_Position_Difference Subtract(World *W, World_Position *A, World_Position *B);
void InitializeWorld(World *W, float TileSideInMeters);
World_Position ChunkPositionFromTilePosition(World *W, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ);
void ChangeEntityLocation(Memory_Arena *Arena, World *W, uint32 LowEntityIndex, World_Position *OldPosition, World_Position *NewPosition);