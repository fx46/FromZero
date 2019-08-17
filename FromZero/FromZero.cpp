#include "FromZero.h"

#include "math.h"

static void OutputSound(SoundBuffer *Buffer, int ToneHz)
{
	static float TSine;
	INT16 ToneVolume = 3000;
	INT16 *SampleOut = Buffer->Samples;
	int WavePeriod = Buffer->SamplesPerSecond / ToneHz;

	for (int SampleIndex = 0; SampleIndex < Buffer->SampleCountToOutput; SampleIndex++)
	{
		float SineValue = sinf(TSine);
		INT16 SampleValue = static_cast<INT16>(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
		TSine += 2.0f * 3.14159265359f / static_cast<float>(WavePeriod);
	}
}

static void RenderGradient(PixelBuffer *Buffer, int XOffset, int YOffset)
{
	UINT8* Row = (UINT8*)Buffer->BitmapMemory;	//8 bit because when we would do "Row + x", the x will be multiplied by the size of the object (pointer arithmetic)
	for (int Y = 0; Y < Buffer->BitmapHeight; ++Y)
	{
		UINT32 *Pixel = reinterpret_cast<UINT32*>(Row);
		for (int X = 0; X < Buffer->BitmapWidth; ++X)
		{
			UINT8 Blue	= static_cast<UINT8>(Y - YOffset);
			UINT8 Green = static_cast<UINT8>(Y + YOffset);
			UINT8 Red	= static_cast<UINT8>(X + 2 * XOffset);

			*Pixel++ = (Red << 16 | Green << 8 | Blue);
		}
		Row += Buffer->Pitch;
	}
}

void GameUpdateAndRencer(PixelBuffer *Buffer, SoundBuffer *SBuffer, GameInput *Input, GameMemory *Memory)
{
	assert(sizeof(GameState) <= Memory->PermanentStorageSize);

	GameState *State = reinterpret_cast<GameState*>(Memory->PermanentStorage);
	if (!Memory->bIsInitialized)
	{
		State->ToneHz = 256;
		Memory->bIsInitialized = true;

#if DEBUG
		const char *Filename = "assets/test.bmp";
		ReadFileResults File = ReadFile(Filename);
		if (File.Contents)
		{
			WriteFile("assets/testOut.bmp", File.ContentsSize, File.Contents);
			FreeFileMemory(File.Contents);
		}
#endif
	}

	if (Input->A)
	{
		State->YOffset++;
		State->ToneHz += 1;
	}
	if (Input->D)
	{
		State->YOffset--;
		State->ToneHz -= 1;
	}

	OutputSound(SBuffer, State->ToneHz);
	RenderGradient(Buffer, State->XOffset, State->YOffset);
}
