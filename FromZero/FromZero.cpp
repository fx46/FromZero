#include "FromZero.h"

#include "math.h"

static void OutputSound(SoundBuffer *Buffer, int ToneHz)
{
	static float TSine;
	signed short ToneVolume = 3000;
	signed short *SampleOut = Buffer->Samples;
	int WavePeriod = Buffer->SamplesPerSecond / ToneHz;

	for (int SampleIndex = 0; SampleIndex < Buffer->SampleCountToOutput; SampleIndex++)
	{
		float SineValue = sinf(TSine);
		signed short SampleValue = (signed short)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
		TSine += 2.0f * 3.14159265359f / static_cast<float>(WavePeriod);
	}
}

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

void GameUpdateAndRencer(PixelBuffer *Buffer, int XOffset, int YOffset, SoundBuffer *SBuffer, int ToneHz)
{
	OutputSound(SBuffer, ToneHz);
	RenderGradient(Buffer, XOffset, YOffset);
}
