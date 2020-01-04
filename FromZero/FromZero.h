#pragma once

#include "FromZero_TileMap.h"
#include "FromZero_intrinsics.h"
#include "Vector.h"

struct PixelBuffer
{
	void *BitmapMemory;	//Pixels are 32-bits wide (BB GG RR XX)
	int BitmapWidth;
	int BitmapHeight;
	int Pitch;
	int BytesPerPixel = 4;
};

//struct ThreadContext
//{
//	int PlaceHolder;
//};

struct SoundBuffer
{
	int SampleCountToOutput;
	int SamplesPerSecond;
	int16 *Samples;
};

struct GameInput
{
	bool W;
	bool A;
	bool S;
	bool D;
	bool Q;
	bool E;
	bool Shift;

	float TimeElapsingOverFrame;
};

struct GameMemory
{
	void *PermanentStorage;
	void *TransientStorage;
	uint64 PermanentStorageSize;
	uint64 TransientStorageSize;
	bool bIsInitialized;
};

struct World_Map
{
	Tile_Map *TileMap;
};

struct Bitmap
{
	int Width;
	int Height;
	uint32 *Pixels;
};

struct GameState
{
	World_Map *World;
	Memory_Arena WorldArena;
	TileMap_Position PlayerPosition;
	TileMap_Position CameraPosition;
	Bitmap Background;
	Bitmap PlayerSprite;
	Vector PlayerVelocity;
};

#pragma pack(push, 1)
struct Bitmap_Header
{
	uint16 FileType;
	uint32 FileSize;
	uint16 Reserved1;
	uint16 Reserved2;
	uint32 BitmapOffset;
	uint32 Size;
	int32 Width;
	int32 Height;
	uint16 Planes;
	uint16 BitsPerPixel;
	uint32 Compression;
	uint32 SizeOfBitmap;
	uint32 HorzResolution;
	uint32 VertResolution;
	uint32 ColorsUsed;
	uint32 ColorsImportant;
	uint32 RedMask;
	uint32 GreenMask;
	uint32 BlueMask;
};
#pragma pack(pop)

void GameUpdateAndRencer(/*ThreadContext *Thread,*/ PixelBuffer *Buffer, GameInput *Input, GameMemory *Memory);
void GameGetSoundSamples(/*ThreadContext *Thread,*/ SoundBuffer *SBuffer/*, GameMemory *Memory*/);

inline uint32 SafeTruncateUINT64(uint64 Value)
{
	assert(Value <= 0xFFFFFFFF);
	return static_cast<uint32>(Value);
}

struct ReadFileResults
{
	void *Contents;
	uint32 ContentsSize;
};

ReadFileResults ReadFile(/*ThreadContext *Thread,*/ const char *Filename);
void FreeFileMemory(/*ThreadContext *Thread,*/ void *Memory);
bool WriteFile(/*ThreadContext *Thread,*/ const char *Filename, uint32 MemorySize, void *Memory);
