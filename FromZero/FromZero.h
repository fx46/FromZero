#pragma once

#include <math.h>

typedef signed char         INT8;
typedef signed short        INT16;
typedef signed int          INT32;
typedef signed __int64      INT64;
typedef unsigned char       UINT8;
typedef unsigned short      UINT16;
typedef unsigned int        UINT32;
typedef unsigned __int64    UINT64;

#if DEBUG
#define assert(Expression) if(!(Expression)) __debugbreak();
#else
#define assert(Expression)
#endif

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
	INT16 *Samples;
};

struct GameInput
{
	bool W;
	bool A;
	bool S;
	bool D;
	bool Q;
	bool E;

	float TimeElapsingOverFrame;
};

struct GameMemory
{
	void *PermanentStorage;
	void *TransientStorage;
	UINT64 PermanentStorageSize;
	UINT64 TransientStorageSize;
	bool bIsInitialized;
};

struct World_Position
{
	int TileMapX;
	int TileMapY;

	int TileX;
	int TileY;

	float TileRelX;
	float TileRelY;
};

struct GameState
{
	World_Position PlayerPosition;

	//float PlayerX;
	//float PlayerY;
	//int PlayerTileMapX;
	//int PlayerTileMapY;
};

struct Tile_Map
{
	UINT32 *Tiles;
};

struct World_Map
{
	float TileSideInMeters;
	float MetersToPixels;
	INT32 TileSideInPixels;

	int TilesNbRows;
	int TilesNbColumns;

	float LowerLeftX;
	float LowerLeftY;

	int NbTileMapsRows;
	int NbTileMapsColumns;

	Tile_Map *TileMaps;
};

void GameUpdateAndRencer(/*ThreadContext *Thread,*/ PixelBuffer *Buffer, GameInput *Input, GameMemory *Memory);
void GameGetSoundSamples(/*ThreadContext *Thread,*/ SoundBuffer *SBuffer/*, GameMemory *Memory*/);

inline UINT32 SafeTruncateUINT64(UINT64 Value)
{
	assert(Value <= 0xFFFFFFFF);
	return static_cast<UINT32>(Value);
}

#if DEBUG
struct ReadFileResults
{
	void *Contents;
	UINT32 ContentsSize;
};

ReadFileResults ReadFile(/*ThreadContext *Thread,*/ const char *Filename);
void FreeFileMemory(/*ThreadContext *Thread,*/ void *Memory);
bool WriteFile(/*ThreadContext *Thread,*/ const char *Filename, UINT32 MemorySize, void *Memory);
#endif
