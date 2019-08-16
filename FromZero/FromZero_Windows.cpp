#include "FromZero_Windows.h"

#include <dsound.h>
#include <malloc.h>
#include <math.h>

#include "FromZero.h"

static bool bRunning = true;
static WindowsPixelBuffer GlobalBuffer;
static LPDIRECTSOUNDBUFFER SecondaryBuffer;
static SoundOutput Sound;
static GameInput Input = {};

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
			BufferDescription.dwFlags = 0;
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
			BOOL WasDown = ((LParam & (1 << 30)) != 0);
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

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int ShowCmd)
{
	LARGE_INTEGER PerformanceCountFrequencyResult;
	QueryPerformanceFrequency(&PerformanceCountFrequencyResult);
	INT64 PercormanceCountFrequency = PerformanceCountFrequencyResult.QuadPart;

	WNDCLASSA WindowClass = {};
	ResizeDIBSection(&GlobalBuffer, 1280, 720);
	WindowClass.style = CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "FromZeroWindowClass"; 


	if (RegisterClass(&WindowClass))
	{
		HWND WindowHandle = CreateWindowEx(0, WindowClass.lpszClassName, "FromZero", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);

		if (WindowHandle)
		{
			InitDSound(WindowHandle, Sound.SamplesPerSecond, Sound.SecondaryBufferSize);
			WinClearSoundBuffer(&Sound);
			SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			INT16 *Samples = static_cast<INT16 *>(VirtualAlloc(0, Sound.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));

			GameMemory Memory = {};
			Memory.PermanentStorageSize = (64 * 1024 * 1024);	//64 Megabytes
			Memory.PermanentStorage = VirtualAlloc(0, Memory.PermanentStorageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			Memory.TransientStorageSize = (static_cast<UINT64>(4) * 1024 * 1024 * 1024);	//4 gigabytes
			Memory.TransientStorage = VirtualAlloc(0, Memory.TransientStorageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			if (!Memory.PermanentStorage || !Samples || !Memory.TransientStorage)
				return 0;

			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);

			while (bRunning)
			{
				MSG Message;

				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				DWORD BytesToLock;
				DWORD TargetCursor;
				DWORD BytesToWrite;
				DWORD PlayCursor;
				DWORD WriteCursor;
				bool SoundIsValid = false;
				if (SUCCEEDED(SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{
					BytesToLock = (Sound.RunningSampleIndex * Sound.BytesPerSample) % Sound.SecondaryBufferSize;
					TargetCursor = (PlayCursor + Sound.BytesPerSample * Sound.LatencySampleCount) % Sound.SecondaryBufferSize;
					BytesToWrite = 0;

					if (BytesToLock > TargetCursor)
					{
						BytesToWrite = Sound.SecondaryBufferSize - BytesToLock;
						BytesToWrite += TargetCursor;
					}
					else
					{
						BytesToWrite = TargetCursor - BytesToLock;
					}

					SoundIsValid = true;
				}

				SoundBuffer SBuffer = {};
				SBuffer.SamplesPerSecond = Sound.SamplesPerSecond;
				SBuffer.SampleCountToOutput = BytesToWrite / Sound.BytesPerSample;
				SBuffer.Samples = Samples;

				PixelBuffer Buffer = {};
				Buffer.BitmapMemory = GlobalBuffer.BitmapMemory;
				Buffer.BitmapWidth = GlobalBuffer.BitmapWidth;
				Buffer.BitmapHeight = GlobalBuffer.BitmapHeight;
				Buffer.Pitch = GlobalBuffer.Pitch;

				GameUpdateAndRencer(&Buffer, &SBuffer, &Input, &Memory);

				if (SoundIsValid)
				{
					FillSoundBuffer(&Sound, BytesToLock, BytesToWrite, &SBuffer);
				}

				WindowDimension Dimension = GetWindowDimention(WindowHandle);
				HDC DeviceContext = GetDC(WindowHandle);
				DisplayBufferToWindow(&GlobalBuffer, Dimension, DeviceContext);

				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter);
				INT64 CounterElaped = EndCounter.QuadPart - LastCounter.QuadPart;
				char CBuffer[256];
				wsprintf(CBuffer, "%d FPS\n", PercormanceCountFrequency / CounterElaped);
				OutputDebugStringA(CBuffer);

				LastCounter = EndCounter;
			}
		}
	}

	return(0);
}