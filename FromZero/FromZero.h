#pragma once

struct PixelBuffer
{
	void *BitmapMemory;
	int BitmapWidth;
	int BitmapHeight;
	int Pitch;
	int BytesPerPixel = 4;
};

static void RenderGradient(PixelBuffer *Buffer, int XOffset, int YOffset);
void GameUpdateAndRencer(PixelBuffer *Buffer, int XOffset, int YOffset);