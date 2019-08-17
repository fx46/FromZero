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
};

struct GameMemory
{
	void *PermanentStorage;
	void *TransientStorage;
	UINT64 PermanentStorageSize;
	UINT64 TransientStorageSize;
	bool bIsInitialized;
};

struct GameState
{
	int ToneHz;
	int XOffset;
	int YOffset;
};

void GameUpdateAndRencer(PixelBuffer *Buffer, SoundBuffer *SBuffer, GameInput *Input, GameMemory *Memory);

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
ReadFileResults ReadFile(const char *Filename);
void FreeFileMemory(void *Memory);
bool WriteFile(const char *Filename, UINT32 MemorySize, void *Memory);
#endif
