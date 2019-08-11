#include <windows.h>
#include <dsound.h>

struct PixelBuffer
{
	BITMAPINFO BitmapInfo;
	void *BitmapMemory;
	int BitmapWidth;
	int BitmapHeight;
	int Pitch;
	int BytesPerPixel = 4;
};

static bool bRunning = true;
static PixelBuffer Buffer;
static LPDIRECTSOUNDBUFFER SecondaryBuffer;

struct WindowDimension
{
	int Width;
	int Height;
};

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

static void RenderGradient(PixelBuffer *Buffer, int XOffset, int YOffset)
{
	UINT8* Row = (UINT8*)Buffer->BitmapMemory;	//8 bit because when we would do "Row + x", the x will be multiplied by the size of the object (pointer arithmetic)
	for (int Y = 0; Y < Buffer->BitmapHeight; ++Y)
	{
		UINT32 *Pixel = (UINT32*)Row;
		for (int X = 0; X < Buffer->BitmapWidth; ++X)
		{
			UINT8 Blue = (Y - YOffset);
			UINT8 Green = (Y + YOffset);
			UINT8 Red = (Y + 2 * YOffset);

			*Pixel++ = (Red << 16 | Green << 8 | Blue);
		}
		Row += Buffer->Pitch;
	}
}

static void ResizeDIBSection(PixelBuffer *Buffer, int Width, int Height)
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

static void DisplayBufferToWindow(PixelBuffer *Buffer, WindowDimension Dimension, HDC DeviceContext)
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
			DisplayBufferToWindow(&Buffer, GetWindowDimention(WindowHandle), DeviceContext);
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
			}
			else if (VKCode == 'S')
			{
				OutputDebugStringA("S Pressed!\n");
			}
			else if (VKCode == 'D')
			{
				OutputDebugStringA("D Pressed!\n");
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

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int ShowCmd)
{
	WNDCLASSA WindowClass = {};
	ResizeDIBSection(&Buffer, 1280, 720);
	WindowClass.style = CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "FromZeroWindowClass"; 

	if (RegisterClass(&WindowClass))
	{
		HWND WindowHandle = CreateWindowEx(0, WindowClass.lpszClassName, "FromZero", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);

		if (WindowHandle)
		{
			UINT32 RunningSampleIndex = 0;
			INT16 Volume = 500;
			int ToneHz = 256;
			int SamplesPerSecond = 48000;
			int SqareWavePeriod = SamplesPerSecond / ToneHz;
			int BytesPerSample = sizeof(INT16) * 2;
			int SecondaryBufferSize = SamplesPerSecond * BytesPerSample;	//buffer size for 1 second

			InitDSound(WindowHandle, SamplesPerSecond, SecondaryBufferSize);
			bool SoundIsPlaying = false;

			while (bRunning)
			{
				MSG Message;

				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				static int XOffset = 0, YOffset = 0;
				RenderGradient(&Buffer, XOffset++, YOffset++);

				DWORD PlayCursor;
				DWORD WriteCursor;
				if (SUCCEEDED(SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{
					DWORD ByteToLock = RunningSampleIndex * BytesPerSample % SecondaryBufferSize;
					DWORD BytesToWrite;
					
					if (ByteToLock == PlayCursor)
					{
						BytesToWrite = SecondaryBufferSize;
					}
					else if (ByteToLock > PlayCursor)
					{
						BytesToWrite = SecondaryBufferSize - ByteToLock;
						BytesToWrite += PlayCursor;
					}
					else
					{
						BytesToWrite = PlayCursor - ByteToLock;
					}

					void *Region1;
					DWORD Region1size;
					void *Region2;
					DWORD Region2size;

					if (SUCCEEDED(SecondaryBuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1size, &Region2, &Region2size, 0)))
					{
						DWORD Region1SampleCount = Region1size / BytesPerSample;
						INT16 *SampleOut = static_cast<INT16*>(Region1);
						for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++)
						{
							INT16 SampleValue = (RunningSampleIndex++ / (SqareWavePeriod / 2)) % 2 ? Volume : -Volume;
							*SampleOut++ = SampleValue;
							*SampleOut++ = SampleValue;
						}

						DWORD Region2SampleCount = Region2size / BytesPerSample;
						SampleOut = static_cast<INT16*>(Region2);
						for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++)
						{
							INT16 SampleValue = (RunningSampleIndex++ / (SqareWavePeriod / 2)) % 2 ? Volume : -Volume;
							*SampleOut++ = SampleValue;
							*SampleOut++ = SampleValue;
						}

						SecondaryBuffer->Unlock(Region1, Region1size, Region2, Region2size);
					}
				}

				if (!SoundIsPlaying)
				{
					SoundIsPlaying = true;
					SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
				}

				WindowDimension Dimension = GetWindowDimention(WindowHandle);
				HDC DeviceContext = GetDC(WindowHandle);
				DisplayBufferToWindow(&Buffer, Dimension, DeviceContext);
			}
		}
	}

	return(0);
}