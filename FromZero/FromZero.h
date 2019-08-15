#pragma once

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
	signed short *Samples;
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

void GameUpdateAndRencer(PixelBuffer *Buffer, SoundBuffer *SBuffer, GameInput *Input);