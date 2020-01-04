#pragma once

#include <windows.h>
#include "FromZero.h"

struct SoundOutput
{
	uint32 RunningSampleIndex = 0;
	int SamplesPerSecond = 48000;
	int BytesPerSample = sizeof(int16) * 2;
	int SecondaryBufferSize = SamplesPerSecond * BytesPerSample;	//buffer size for 1 second
	int SafetyBytes;
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

struct DebugTimeMarker
{
	DWORD OutputPlayCursor;
	DWORD OutputWriteCursor;
	DWORD OutputLocation;
	DWORD OutputByteCount;
	DWORD ExpectedFlipPlayCursor;

	DWORD FlipPlayCursor;
	DWORD FlipWriteCursor;
};

struct WindowsState
{
	uint64 TotalSize;
	void *GameMemoryBlock;
};