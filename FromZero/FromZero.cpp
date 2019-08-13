#include "FromZero.h"

static void RenderGradient(PixelBuffer *Buffer, int XOffset, int YOffset)
{
	unsigned char* Row = (unsigned char*)Buffer->BitmapMemory;	//8 bit because when we would do "Row + x", the x will be multiplied by the size of the object (pointer arithmetic)
	for (int Y = 0; Y < Buffer->BitmapHeight; ++Y)
	{
		unsigned int *Pixel = (unsigned int*)Row;
		for (int X = 0; X < Buffer->BitmapWidth; ++X)
		{
			unsigned char Blue = (Y - YOffset);
			unsigned char Green = (Y + YOffset);
			unsigned char Red = (Y + 2 * YOffset);

			*Pixel++ = (Red << 16 | Green << 8 | Blue);
		}
		Row += Buffer->Pitch;
	}
}

void GameUpdateAndRencer(PixelBuffer *Buffer, int XOffset, int YOffset)
{
	RenderGradient(Buffer, XOffset, YOffset);
}
