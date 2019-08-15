#pragma once

#include <windows.h>

struct SoundOutput
{
	UINT32 RunningSampleIndex = 0;
	float TSine;
	int SamplesPerSecond = 48000;
	int BytesPerSample = sizeof(INT16) * 2;
	int SecondaryBufferSize = SamplesPerSecond * BytesPerSample;	//buffer size for 1 second
	int LatencySampleCount = SamplesPerSecond / 15;
};

struct WindowsPixelBuffer
{
	BITMAPINFO BitmapInfo;
	void *BitmapMemory;
	int BitmapWidth;
	int BitmapHeight;
	int Pitch;
	int BytesPerPixel = 4;
};

struct WindowDimension
{
	int Width;
	int Height;
};