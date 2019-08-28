#include "FromZero_Windows.h"

#include <dsound.h>
#include <malloc.h>
#include <math.h>

#include "FromZero.h"

static INT64 PerformanceCountFrequency;
static WindowsPixelBuffer GlobalBuffer;
static LPDIRECTSOUNDBUFFER SecondaryBuffer;
static SoundOutput SoundConfig;
static GameInput Input = {};
static bool bRunning = true;

typedef HRESULT WINAPI direct_sound_create(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);	//declaring function signature as a type

static void InitDSound(HWND WindowHandle, UINT32 SamplesPerSecond, UINT32 BufferSize)
{
	if (HMODULE DSoundLibrary = LoadLibraryA("dsound.dll"))
	{
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec  * WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			if (SUCCEEDED(DirectSound->SetCooperativeLevel(WindowHandle, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
					{
						//This is not actually a buffer, it is used to get a handle on the sound card to set the correct format. Maybe not needed?
						OutputDebugStringA("Primary buffer format set.\n");
					}
				}
			}

			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
			if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &SecondaryBuffer, 0)))
			{
				OutputDebugStringA("Secondary buffer created.\n");
			}
		}
	}
}

static WindowDimension GetWindowDimention(HWND WindowHandle)
{
	RECT ClientRect;
	GetClientRect(WindowHandle, &ClientRect);
	WindowDimension Result;
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return Result;
}

static void ResizeDIBSection(WindowsPixelBuffer *Buffer, int Width, int Height)
{
	if (Buffer->BitmapMemory)
	{
		VirtualFree(Buffer->BitmapMemory, 0, MEM_RELEASE);
	}

	Buffer->BitmapWidth = Width;
	Buffer->BitmapHeight = Height;
	Buffer->BitmapInfo.bmiHeader.biSize = sizeof(Buffer->BitmapInfo.bmiHeader);
	Buffer->BitmapInfo.bmiHeader.biWidth = Buffer->BitmapWidth;
	Buffer->BitmapInfo.bmiHeader.biHeight = -Buffer->BitmapHeight; //negative so the bitmap is a top-down DIB with the origin at the upper left corner.
	Buffer->BitmapInfo.bmiHeader.biPlanes = 1;
	Buffer->BitmapInfo.bmiHeader.biBitCount = 32;
	Buffer->BitmapInfo.bmiHeader.biCompression = BI_RGB;
	Buffer->BitmapMemory = VirtualAlloc(0, Buffer->BytesPerPixel * Buffer->BitmapWidth * Buffer->BitmapHeight, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	Buffer->Pitch = Buffer->BitmapWidth * Buffer->BytesPerPixel;
}

static void DisplayBufferToWindow(WindowsPixelBuffer *Buffer, WindowDimension Dimension, HDC DeviceContext)
{
	StretchDIBits(DeviceContext, 0, 0, Dimension.Width, Dimension.Height, 0, 0, Buffer->BitmapWidth, Buffer->BitmapHeight, Buffer->BitmapMemory, &Buffer->BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

static LRESULT CALLBACK MainWindowCallback(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;

	switch (Message)
	{
		case WM_SIZE:
		{
			OutputDebugStringA("WM_SIZE\n");
		} break;

		case WM_DESTROY:
		{
			bRunning = false;
			OutputDebugStringA("WM_DESTROY\n");
		} break;

		case WM_CLOSE:
		{
			bRunning = false;
			OutputDebugStringA("WM_CLOSE\n");
		} break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT P;
			HDC DeviceContext = BeginPaint(WindowHandle, &P);
			DisplayBufferToWindow(&GlobalBuffer, GetWindowDimention(WindowHandle), DeviceContext);
			EndPaint(WindowHandle, &P);
		} break;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			WPARAM VKCode = WParam;
			//BOOL WasDown = ((LParam & (1 << 30)) != 0);
			bool IsDown = ((LParam & (1 << 31)) == 0);

			if (VKCode == 'W')
			{
				OutputDebugStringA("W Pressed!\n");
			}
			else if (VKCode == 'A')
			{
				OutputDebugStringA("A Pressed!\n");
				Input.A = IsDown;
			}
			else if (VKCode == 'S')
			{
				OutputDebugStringA("S Pressed!\n");
			}
			else if (VKCode == 'D')
			{
				OutputDebugStringA("D Pressed!\n");
				Input.D = IsDown;
			}
			else if (VKCode == 'Q')
			{
				OutputDebugStringA("Q Pressed!\n");
			}
			else if (VKCode == 'E')
			{
				OutputDebugStringA("E Pressed!\n");
			}
			else if (VKCode == VK_DOWN)
			{
				OutputDebugStringA("VK_DOWN Pressed!\n");
			}
			else if (VKCode == VK_UP)
			{
				OutputDebugStringA("VK_UP Pressed!\n");
			}
			else if (VKCode == VK_RIGHT)
			{
				OutputDebugStringA("VK_RIGHT Pressed!\n");
			}
			else if (VKCode == VK_LEFT)
			{
				OutputDebugStringA("VK_LEFT Pressed!\n");
			}
			else if (VKCode == VK_ESCAPE)
			{
				OutputDebugStringA("VK_ESCAPE Pressed!\n");
			}
			else if (VKCode == VK_SPACE)
			{
				OutputDebugStringA("VK_SPACE Pressed!\n");
			}

			bool AltKeyWasDown = LParam & (1 << 29);
			if ((VKCode == VK_F4) && AltKeyWasDown)
			{
				bRunning = false;
			}
		} break;

		default:
		{
			OutputDebugStringA("Default message received.\n");
			Result = DefWindowProc(WindowHandle, Message, WParam, LParam);
		} break;
	}
	return Result;
}

static void WinClearSoundBuffer(SoundOutput *Sound)
{
	void *Region1;
	DWORD Region1size;
	void *Region2;
	DWORD Region2size;

	if (SUCCEEDED(SecondaryBuffer->Lock(0, Sound->SecondaryBufferSize, &Region1, &Region1size, &Region2, &Region2size, 0)))
	{
		UINT8 *DestSample = static_cast<UINT8*>(Region1);
		for (DWORD ByteIndex = 0; ByteIndex < Region1size; ByteIndex++)
		{
			*DestSample++ = 0;
		}
		DestSample = static_cast<UINT8*>(Region2);
		for (DWORD ByteIndex = 0; ByteIndex < Region2size; ByteIndex++)
		{
			*DestSample++ = 0;
		}

		SecondaryBuffer->Unlock(Region1, Region1size, Region2, Region2size);
	}
}

static void FillSoundBuffer(SoundOutput *Sound, DWORD BytesToLock, DWORD BytesToWrite, SoundBuffer *SBuffer)
{
	void *Region1;
	DWORD Region1size;
	void *Region2;
	DWORD Region2size;

	if (SUCCEEDED(SecondaryBuffer->Lock(BytesToLock, BytesToWrite, &Region1, &Region1size, &Region2, &Region2size, 0)))
	{
		DWORD Region1SampleCount = Region1size / Sound->BytesPerSample;
		INT16 *DestSample = static_cast<INT16*>(Region1);
		INT16 *SourceSample = SBuffer->Samples;
		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			Sound->RunningSampleIndex++;
		}

		DWORD Region2SampleCount = Region2size / Sound->BytesPerSample;
		DestSample = static_cast<INT16*>(Region2);
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			Sound->RunningSampleIndex++;
		}

		SecondaryBuffer->Unlock(Region1, Region1size, Region2, Region2size);
	}
}

#if DEBUG
ReadFileResults ReadFile(const char *Filename)
{
	ReadFileResults Result = {};

	HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize))
		{
			UINT32 FileSize32 = SafeTruncateUINT64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents)
			{
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead))
				{
					Result.ContentsSize = FileSize32;
				}
				else
				{
					FreeFileMemory(Result.Contents);
					Result.Contents = 0;
				}
			}
		}
		CloseHandle(FileHandle);
	}

	return(Result);
}

void FreeFileMemory(void *Memory)
{
	if (Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

bool WriteFile(const char *Filename, UINT32 MemorySize, void *Memory)
{
	bool Result = false;

	HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;
		if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
		{
			Result = (BytesWritten == MemorySize);
		}

		CloseHandle(FileHandle);
	}

	return(Result);
}
#endif

inline LARGE_INTEGER GetWallClock()
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return Result;
}

inline float GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	return static_cast<float>(End.QuadPart - Start.QuadPart) / static_cast<float>(PerformanceCountFrequency);
}

static void DebugDrawVertical(WindowsPixelBuffer *DebugBuffer, int X, int Top, int Bottom, UINT32 Color)
{
	UINT8 *Pixel = static_cast<UINT8 *>(DebugBuffer->BitmapMemory) + X * DebugBuffer->BytesPerPixel + Top * DebugBuffer->Pitch;
	for (int Y = 0; Y < Bottom; ++Y)
	{
		*(reinterpret_cast<UINT32 *>(Pixel)) = Color;
		Pixel += DebugBuffer->Pitch;
	}
}

static void DrawSoundBufferMarker(WindowsPixelBuffer *DebugGlobalBuffer, SoundOutput *SBuffer, float C, int PadX, int Top, int Bottom, DWORD Value, UINT32 Color)
{
	int X = PadX + static_cast<int>(C * static_cast<float>(Value));
	DebugDrawVertical(DebugGlobalBuffer, X, Top, Bottom, Color);
}

//TEST
static void DebugSyncDisplay(WindowsPixelBuffer *Debuguffer, int MarkerCount, DebugTimeMarker *Markers, SoundOutput *SBuffer, float TargetSecondsPerFrame)
{
	int PadX = 16;
	int PadY = 16;
	int Top = PadY;
	int Bottom = Debuguffer->BitmapHeight - PadY;

	float C = static_cast<float>(Debuguffer->BitmapWidth - 2 * PadX) / static_cast<float>(SBuffer->SecondaryBufferSize);

	for (int MarkerIndex = 0; MarkerIndex < MarkerCount; ++MarkerIndex)
	{
		DebugTimeMarker *Marker = &Markers[MarkerIndex];
		DrawSoundBufferMarker(Debuguffer, SBuffer, C, PadX, Top, Bottom, Marker->PlayCursor, 0xFFFFFFFF);
		DrawSoundBufferMarker(Debuguffer, SBuffer, C, PadX, Top, Bottom, Marker->WriteCursor, 0xFFFF0000);
	}
}
//TEST

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE , LPSTR, int)
{
	LARGE_INTEGER PerformanceCountFrequencyResult;
	QueryPerformanceFrequency(&PerformanceCountFrequencyResult);
	PerformanceCountFrequency = PerformanceCountFrequencyResult.QuadPart;

	bool bSleepIsGranular = timeBeginPeriod(1) == TIMERR_NOERROR;	//Set the Windows scheduler granularity to 1ms so that Sleep() is more granular.

	WNDCLASSA WindowClass = {};
	ResizeDIBSection(&GlobalBuffer, 1280, 720);
	WindowClass.style = CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "FromZeroWindowClass"; 

#define FramesOfAudioLatency 4
#define MonitorRefreshHz 60
#define	GameUpdateHz (MonitorRefreshHz / 2)

	float TargetSecondsPerFrame = 1.0f / static_cast<float>(GameUpdateHz);

	if (RegisterClass(&WindowClass))
	{
		HWND WindowHandle = CreateWindowEx(0, WindowClass.lpszClassName, "FromZero", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);

		if (WindowHandle)
		{
			SoundConfig.LatencySampleCount = FramesOfAudioLatency * SoundConfig.SamplesPerSecond / GameUpdateHz;
			InitDSound(WindowHandle, SoundConfig.SamplesPerSecond, SoundConfig.SecondaryBufferSize);
			WinClearSoundBuffer(&SoundConfig);
			SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
// 			while (bRunning)
// 			{
// 				DWORD PlayCursor;
// 				DWORD WriteCursor;
// 				SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
// 
// 				char TextBuffer[256];
// 				wsprintf(TextBuffer, "PlayCursor: %u, WriteCursor: %u\n", PlayCursor, WriteCursor);
// 				OutputDebugStringA(TextBuffer);
// 			}

			INT16 *Samples = static_cast<INT16 *>(VirtualAlloc(0, SoundConfig.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));

			GameMemory Memory = {};
			Memory.PermanentStorageSize = (64 * 1024 * 1024);	//64 Megabytes
			Memory.PermanentStorage = VirtualAlloc(0, Memory.PermanentStorageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			Memory.TransientStorageSize = (static_cast<UINT64>(1) * 1024 * 1024 * 1024);	//1 gigabytes
			Memory.TransientStorage = VirtualAlloc(0, Memory.TransientStorageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			if (!Memory.PermanentStorage || !Samples || !Memory.TransientStorage)
				return 0;

			//TEST
			int DebugTimeMarkerIndex = 0;
			DebugTimeMarker DebugTimeMarkers[GameUpdateHz / 2] = {};
			//TEST

			LARGE_INTEGER LastCounter = GetWallClock();

			DWORD LastPlayCursor = 0;
			bool SoundIsValid = false;

			while (bRunning)
			{
				MSG Message;

				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				DWORD BytesToLock  = 0;
				DWORD TargetCursor = 0;
				DWORD BytesToWrite = 0;

				if (SoundIsValid)
				{
					BytesToLock = (SoundConfig.RunningSampleIndex * SoundConfig.BytesPerSample) % SoundConfig.SecondaryBufferSize;
					TargetCursor = (LastPlayCursor + SoundConfig.BytesPerSample * SoundConfig.LatencySampleCount) % SoundConfig.SecondaryBufferSize;
					BytesToWrite = 0;

					if (BytesToLock > TargetCursor)
					{
						BytesToWrite = SoundConfig.SecondaryBufferSize - BytesToLock;
						BytesToWrite += TargetCursor;
					}
					else
					{
						BytesToWrite = TargetCursor - BytesToLock;
					}
				}

				SoundBuffer SBuffer = {};
				SBuffer.SamplesPerSecond = SoundConfig.SamplesPerSecond;
				SBuffer.SampleCountToOutput = BytesToWrite / SoundConfig.BytesPerSample;
				SBuffer.Samples = Samples;

				PixelBuffer Buffer = {};
				Buffer.BitmapMemory = GlobalBuffer.BitmapMemory;
				Buffer.BitmapWidth = GlobalBuffer.BitmapWidth;
				Buffer.BitmapHeight = GlobalBuffer.BitmapHeight;
				Buffer.Pitch = GlobalBuffer.Pitch;

				GameUpdateAndRencer(&Buffer, &SBuffer, &Input, &Memory);

				if (SoundIsValid)
				{
					//TEST
					DWORD PlayCursor;
					DWORD WriteCursor;
					SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
					char TextBuffer[256];
					wsprintf(TextBuffer, "LastPlayCursor: %u BytesToLock: %u, TargetCursor: %u, ByteToWrite: %u - PC: %u, WC: %u\n", LastPlayCursor, BytesToLock, TargetCursor, BytesToWrite, PlayCursor, WriteCursor);
					OutputDebugStringA(TextBuffer);
					//TEST

					FillSoundBuffer(&SoundConfig, BytesToLock, BytesToWrite, &SBuffer);
				}

				LARGE_INTEGER WorkCounter = GetWallClock();
				float SecondsElapsedForWork = GetSecondsElapsed(LastCounter, WorkCounter);
				float SecondsElapsedForFrame = SecondsElapsedForWork;

				if (SecondsElapsedForFrame < TargetSecondsPerFrame)
				{
					if (bSleepIsGranular)
					{
						DWORD SleepMS = static_cast<DWORD>(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
						if (SleepMS > 0) Sleep(SleepMS);
					}
					while (SecondsElapsedForFrame < TargetSecondsPerFrame)
					{
						SecondsElapsedForFrame = GetSecondsElapsed(LastCounter, GetWallClock());
					}
				}
				else
				{

				}

				LARGE_INTEGER EndCounter = GetWallClock();
				LastCounter = EndCounter;

				//TEST
				DebugSyncDisplay(&GlobalBuffer, sizeof(DebugTimeMarkers) / sizeof(*DebugTimeMarkers), DebugTimeMarkers, &SoundConfig, TargetSecondsPerFrame);
				//TEST

				DisplayBufferToWindow(&GlobalBuffer, GetWindowDimention(WindowHandle), GetDC(WindowHandle));

				DWORD PlayCursor;
				DWORD WriteCursor;
				if (SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
				{
					LastPlayCursor = PlayCursor;
					if (!SoundIsValid)
					{
						//at startup
						SoundConfig.RunningSampleIndex = WriteCursor / SoundConfig.BytesPerSample;
						SoundIsValid = true;
					}
				}
				else
				{
					SoundIsValid = false;
				}

				//TEST
				{
					DebugTimeMarker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex++];
					if (DebugTimeMarkerIndex > sizeof(DebugTimeMarkers) / sizeof(*DebugTimeMarkers))
					{
						DebugTimeMarkerIndex = 0;
					}
					Marker->PlayCursor = PlayCursor;
					Marker->WriteCursor = WriteCursor;
				}
				//TEST

				char CBuffer[256];
				wsprintf(CBuffer, "%d FPS\n", static_cast<int>(1 / SecondsElapsedForFrame));
				OutputDebugStringA(CBuffer);
			}
		}
	}

	return(0);
}