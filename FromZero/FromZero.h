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

void GameUpdateAndRencer(PixelBuffer *Buffer, int XOffset, int YOffset, SoundBuffer *SBuffer, int ToneHz);