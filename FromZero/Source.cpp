#include <windows.h>

struct PixelBuffer
{
	BITMAPINFO BitmapInfo;
	void *BitmapMemory;
	int BitmapWidth;
	int BitmapHeight;
	int Pitch;
	int BytesPerPixel;
};

static bool bRunning = true;
static PixelBuffer Buffer;

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

	Buffer->BytesPerPixel = 4;
	Buffer->BitmapWidth = Width;
	Buffer->BitmapHeight = Height;
	Buffer->BitmapInfo.bmiHeader.biSize = sizeof(Buffer->BitmapInfo.bmiHeader);
	Buffer->BitmapInfo.bmiHeader.biWidth = Buffer->BitmapWidth;
	Buffer->BitmapInfo.bmiHeader.biHeight = -Buffer->BitmapHeight; //negative so the bitmap is a top-down DIB with the origin at the upper left corner.
	Buffer->BitmapInfo.bmiHeader.biPlanes = 1;
	Buffer->BitmapInfo.bmiHeader.biBitCount = 32;
	Buffer->BitmapInfo.bmiHeader.biCompression = BI_RGB;
	Buffer->BitmapMemory = VirtualAlloc(0, Buffer->BytesPerPixel * Buffer->BitmapWidth * Buffer->BitmapHeight, MEM_COMMIT, PAGE_READWRITE);

	Buffer->Pitch = Buffer->BitmapWidth * Buffer->BytesPerPixel;
}

static void CopyBufferToWindow(PixelBuffer *Buffer, RECT WindowRect, HDC DeviceContext, int Left, int Top, int Width, int Height)
{
	int WindowWidth = WindowRect.right - WindowRect.left;
	int WindowHeight = WindowRect.bottom - WindowRect.top;
	StretchDIBits(DeviceContext, 0, 0, Buffer->BitmapWidth, Buffer->BitmapHeight, 0, 0, WindowWidth, WindowHeight, Buffer->BitmapMemory, &Buffer->BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;

	switch (Message)
	{
		case WM_SIZE:
		{
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			ResizeDIBSection(&Buffer, ClientRect.right - ClientRect.left, ClientRect.bottom - ClientRect.top);
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
			HDC DeviceContext = BeginPaint(Window, &P);
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			CopyBufferToWindow(&Buffer, ClientRect, DeviceContext, P.rcPaint.left, P.rcPaint.top, P.rcPaint.right - P.rcPaint.left, P.rcPaint.bottom - P.rcPaint.top);
			EndPaint(Window, &P);
		} break;

		default:
		{
			OutputDebugStringA("Default message received.\n");
			Result = DefWindowProc(Window, Message, WParam, LParam);
		} break;
	}
	return Result;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int ShowCmd)
{
	WNDCLASS WindowClass = {};
	WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "FromZeroWindowClass"; 

	if (RegisterClass(&WindowClass))
	{
		HWND WindowHandle = CreateWindowEx
		(
			0,
			WindowClass.lpszClassName, 
			"FromZero", 
			WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
			CW_USEDEFAULT, 
			CW_USEDEFAULT, 
			CW_USEDEFAULT, 
			CW_USEDEFAULT, 
			0, 
			0, 
			Instance, 
			0
		);

		if (WindowHandle)
		{
			int XOffset = 0, YOffset = 0;

			while (bRunning)
			{
				MSG Message;

				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				RenderGradient(&Buffer, XOffset++, YOffset++);

				RECT ClientRect;
				GetClientRect(WindowHandle, &ClientRect);
				HDC DeviceContext = GetDC(WindowHandle);
				CopyBufferToWindow(&Buffer, ClientRect, DeviceContext, ClientRect.left, ClientRect.top, ClientRect.right - ClientRect.left, ClientRect.bottom - ClientRect.top);
			}
		}
	}

	return(0);
}