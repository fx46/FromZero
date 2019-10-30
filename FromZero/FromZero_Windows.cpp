#include "FromZero_Windows.h"

#include <dsound.h>
#include <malloc.h>

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
	const int XOffSet = 10;
	const int YOffSet = 10;

	//Clear unused buffer pixels to black
	PatBlt(DeviceContext, 0, 0, Dimension.Width, YOffSet, BLACKNESS);
	PatBlt(DeviceContext, 0, YOffSet + Buffer->BitmapHeight, Dimension.Width, Dimension.Height, BLACKNESS);
	PatBlt(DeviceContext, 0, 0, XOffSet, Dimension.Height, BLACKNESS);
	PatBlt(DeviceContext, XOffSet + Buffer->BitmapWidth, 0, Dimension.Width, Dimension.Height, BLACKNESS);

	StretchDIBits(DeviceContext, XOffSet, YOffSet, Buffer->BitmapWidth, Buffer->BitmapHeight, 0, 0, Buffer->BitmapWidth, Buffer->BitmapHeight, Buffer->BitmapMemory, &Buffer->BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
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
				Input.W = IsDown;
			}
			else if (VKCode == 'A')
			{
				OutputDebugStringA("A Pressed!\n");
				Input.A = IsDown;
			}
			else if (VKCode == 'S')
			{
				OutputDebugStringA("S Pressed!\n");
				Input.S = IsDown;
			}
			else if (VKCode == 'D')
			{
				OutputDebugStringA("D Pressed!\n");
				Input.D = IsDown;
			}
			else if (VKCode == 'Q')
			{
				OutputDebugStringA("Q Pressed!\n");
				Input.Q = IsDown;
			}
			else if (VKCode == 'E')
			{
				OutputDebugStringA("E Pressed!\n");
				Input.E = IsDown;
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
			//OutputDebugStringA("Default message received.\n");
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
ReadFileResults ReadFile(/*ThreadContext *Thread,*/ const char *Filename)
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
					FreeFileMemory(/*Thread,*/ Result.Contents);
					Result.Contents = 0;
				}
			}
		}
		CloseHandle(FileHandle);
	}

	return(Result);
}

void FreeFileMemory(/*ThreadContext *Thread,*/ void *Memory)
{
	if (Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

bool WriteFile(/*ThreadContext *Thread,*/ const char *Filename, UINT32 MemorySize, void *Memory)
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

//static void DebugDrawVertical(WindowsPixelBuffer *DebugBuffer, int X, int Top, int Bottom, UINT32 Color)
//{
//	if (Top <= 0)
//	{
//		Top = 0;
//	}
//	if (Bottom > DebugBuffer->BitmapHeight)
//	{
//		Bottom = DebugBuffer->BitmapHeight;
//	}
//
//	if ((X >= 0) && (X < DebugBuffer->BitmapWidth))
//	{
//		UINT8 *Pixel = static_cast<UINT8 *>(DebugBuffer->BitmapMemory) + X * DebugBuffer->BytesPerPixel + Top * DebugBuffer->Pitch;
//		for (int Y = Top; Y < Bottom; ++Y)
//		{
//			*(reinterpret_cast<UINT32 *>(Pixel)) = Color;
//			Pixel += DebugBuffer->Pitch;
//		}
//	}
//}

//static void DrawSoundBufferMarker(WindowsPixelBuffer *DebugBuffer, float C, int PadX, int Top, int Bottom, DWORD Value, UINT32 Color)
//{
//	int X = PadX + static_cast<int>(C * static_cast<float>(Value));
//	DebugDrawVertical(DebugBuffer, X, Top, Bottom, Color);
//}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE , LPSTR, int)
{
	LARGE_INTEGER PerformanceCountFrequencyResult;
	QueryPerformanceFrequency(&PerformanceCountFrequencyResult);
	PerformanceCountFrequency = PerformanceCountFrequencyResult.QuadPart;

	bool bSleepIsGranular = timeBeginPeriod(1) == TIMERR_NOERROR;	//Set the Windows scheduler granularity to 1ms so that Sleep() is more granular.

	WNDCLASSA WindowClass = {};
	ResizeDIBSection(&GlobalBuffer, 960, 540);
	WindowClass.style = CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "FromZeroWindowClass"; 

#define FramesOfAudioLatency 3

	if (RegisterClass(&WindowClass))
	{
		HWND WindowHandle = CreateWindowEx(0, WindowClass.lpszClassName, "FromZero", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);

		if (WindowHandle)
		{
			int MonitorRefreshHz = 60;
			HDC DC = GetDC(WindowHandle);
			int RefreshRate = GetDeviceCaps(DC, VREFRESH);
			ReleaseDC(WindowHandle, DC);
			if (RefreshRate > 1)
			{
				MonitorRefreshHz = RefreshRate;
			}
			float GameUpdateHz = (MonitorRefreshHz / 1.0f);	//60 fps
			float TargetSecondsPerFrame = 1.0f / GameUpdateHz;

			SoundConfig.SafetyBytes = (int)(((float)SoundConfig.SamplesPerSecond * (float)SoundConfig.BytesPerSample / GameUpdateHz) / 3.0f);
			InitDSound(WindowHandle, SoundConfig.SamplesPerSecond, SoundConfig.SecondaryBufferSize);
			WinClearSoundBuffer(&SoundConfig);
			SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			WindowsState State = {};

			INT16 *Samples = static_cast<INT16 *>(VirtualAlloc(0, SoundConfig.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));

#if DEBUG
			LPVOID BaseAddress = (LPVOID)(2LL * 1024LL * 1024LL * 1024LL * 1024LL);	//2 Terra bytes
#else
			LPVOID BaseAddress = 0;
#endif

			GameMemory Memory = {};
			Memory.PermanentStorageSize = (64LL * 1024LL * 1024LL);	//64 Megabytes
			Memory.TransientStorageSize = (1LL * 1024LL * 1024LL * 1024LL);	//1 gigabytes

			State.TotalSize = Memory.PermanentStorageSize + Memory.TransientStorageSize;
			State.GameMemoryBlock = VirtualAlloc(BaseAddress, static_cast<size_t>(State.TotalSize), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			Memory.PermanentStorage = State.GameMemoryBlock;
			Memory.TransientStorage = (static_cast<UINT8 *>(Memory.PermanentStorage) + Memory.PermanentStorageSize);

			if (!Memory.PermanentStorage || !Samples || !Memory.TransientStorage)
			{
				return 0;
			}

			LARGE_INTEGER LastCounter = GetWallClock();
			LARGE_INTEGER FlipWallClock = GetWallClock();

			//DWORD AudioLatencyBytes = 0;
			//float AudioLatencySeconds = 0;
			bool SoundIsValid = false;
			
			while (bRunning)
			{
				Input.TimeElapsingOverFrame = TargetSecondsPerFrame;

				MSG Message;

				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				//ThreadContext Thread = {};

				PixelBuffer Buffer = {};
				Buffer.BitmapMemory = GlobalBuffer.BitmapMemory;
				Buffer.BitmapWidth = GlobalBuffer.BitmapWidth;
				Buffer.BitmapHeight = GlobalBuffer.BitmapHeight;
				Buffer.Pitch = GlobalBuffer.Pitch;
				GameUpdateAndRencer(/*&Thread,*/ &Buffer, &Input, &Memory);

				LARGE_INTEGER AudioWallClock = GetWallClock();
				float FromBeginToAudioSeconds = GetSecondsElapsed(FlipWallClock, AudioWallClock);

				DWORD PlayCursor;
				DWORD WriteCursor;
				if (SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
				{
					if (!SoundIsValid)
					{	//at startup
						SoundConfig.RunningSampleIndex = WriteCursor / SoundConfig.BytesPerSample;
						SoundIsValid = true;
					}

					DWORD BytesToLock = (SoundConfig.RunningSampleIndex * SoundConfig.BytesPerSample) % SoundConfig.SecondaryBufferSize;
					DWORD ExpectedSoundBytesPerFrame = (int)((float)(SoundConfig.SamplesPerSecond * SoundConfig.BytesPerSample) / GameUpdateHz);
					float SecondsLeftUntilFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;
					DWORD ExpectedBytesUntilFlip = static_cast<DWORD>((SecondsLeftUntilFlip / TargetSecondsPerFrame) * static_cast<float>(ExpectedSoundBytesPerFrame));
					DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;

					DWORD SafeWriteCursor = WriteCursor;
					if (SafeWriteCursor < PlayCursor)
					{
						SafeWriteCursor += SoundConfig.SecondaryBufferSize;
					}
					assert(SafeWriteCursor >= PlayCursor)
					SafeWriteCursor += SoundConfig.SafetyBytes;
					bool AudioCardIsLowLatency = SafeWriteCursor < ExpectedFrameBoundaryByte;

					DWORD TargetCursor = 0;
					if (AudioCardIsLowLatency)
					{
						TargetCursor = ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame;
					}
					else
					{
						TargetCursor = WriteCursor + ExpectedSoundBytesPerFrame + SoundConfig.SafetyBytes;
					}

					TargetCursor %= SoundConfig.SecondaryBufferSize;

					DWORD BytesToWrite = 0;
					if (BytesToLock > TargetCursor)
					{
						BytesToWrite = SoundConfig.SecondaryBufferSize - BytesToLock;
						BytesToWrite += TargetCursor;
					}
					else
					{
						BytesToWrite = TargetCursor - BytesToLock;
					}

					SoundBuffer SBuffer = {};
					SBuffer.SamplesPerSecond = SoundConfig.SamplesPerSecond;
					SBuffer.SampleCountToOutput = BytesToWrite / SoundConfig.BytesPerSample;
					SBuffer.Samples = Samples;
					GameGetSoundSamples(/*&Thread,*/ &SBuffer/*, &Memory*/);
					
					FillSoundBuffer(&SoundConfig, BytesToLock, BytesToWrite, &SBuffer);
				}
				else
				{
					SoundIsValid = false;
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

				DisplayBufferToWindow(&GlobalBuffer, GetWindowDimention(WindowHandle), GetDC(WindowHandle));

				FlipWallClock = GetWallClock();

				char CBuffer[256];
				wsprintf(CBuffer, "%d FPS\n", static_cast<int>(1 / SecondsElapsedForFrame));
				OutputDebugStringA(CBuffer);
			}
		}
	}

	return(0);
}