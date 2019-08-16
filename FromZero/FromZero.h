#pragma once

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
	void *BitmapMemory;
	int BitmapWidth;
	int BitmapHeight;
	int Pitch;
	int BytesPerPixel = 4;
};

struct SoundBuffer
{
	INT16 *Samples;
	int SampleCountToOutput;
	int SamplesPerSecond;
};

struct GameInput
{
	bool W;
	bool A;
	bool S;
	bool D;
};

struct GameMemory
{
	bool bIsInitialized;
	UINT64 PermanentStorageSize;
	void *PermanentStorage;
	UINT64 TransientStorageSize;
	void *TransientStorage;
};

struct GameState
{
	int ToneHz;
	int XOffset;
	int YOffset;
};

void GameUpdateAndRencer(PixelBuffer *Buffer, SoundBuffer *SBuffer, GameInput *Input, GameMemory *Memory);