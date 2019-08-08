#include <windows.h>

static bool bRunning = true;
static BITMAPINFO BitmapInfo;
static void *BitmapMemory;
static int BitmapWidth;
static int BitmapHeight;
static int BytesPerPixel = 4;

static void RenderGradient(int XOffset, int YOffset)
{
	int Pitch = BitmapWidth * BytesPerPixel;
	UINT8* Row = (UINT8*)BitmapMemory;	//8 bit because when we would do "Row + x", the x will be multiplied by the size of the object (pointer arithmetic)
	for (int Y = 0; Y < BitmapHeight; ++Y)
	{
		UINT8 *Pixel = (UINT8*)Row;
		for (int X = 0; X < BitmapWidth; ++X)
		{
			*Pixel = 0;
			++Pixel;

			*Pixel = (UINT8)(Y + YOffset);
			++Pixel;

			*Pixel = (UINT8)(X + XOffset);
			++Pixel;

			*Pixel = 0;
			++Pixel;
		}
		Row += Pitch;
	}
}

static void ResizeDIBSection(int Width, int Height)
{
	if (BitmapMemory)
	{
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);
	}

	BitmapWidth = Width;
	BitmapHeight = Height;
	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = BitmapWidth;
	BitmapInfo.bmiHeader.biHeight = -BitmapHeight; //negative so the bitmap is a top-down DIB with the origin at the upper left corner.
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;
	BitmapMemory = VirtualAlloc(0, BytesPerPixel * BitmapWidth * BitmapHeight, MEM_COMMIT, PAGE_READWRITE);
}

static void UpdateClientWindow(RECT *WindowRect, HDC DeviceContext, int Left, int Top, int Width, int Height)
{
	int WindowWidth = WindowRect->right - WindowRect->left;
	int WindowHeight = WindowRect->bottom - WindowRect->top;
	StretchDIBits(DeviceContext, 0, 0, BitmapWidth, BitmapHeight, 0, 0, WindowWidth, WindowHeight, BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
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
			ResizeDIBSection(ClientRect.right - ClientRect.left, ClientRect.bottom - ClientRect.top);
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

			UpdateClientWindow(&ClientRect, DeviceContext, P.rcPaint.left, P.rcPaint.top, P.rcPaint.right - P.rcPaint.left, P.rcPaint.bottom - P.rcPaint.top);
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
			MSG Message;
			int XOffset = 0, YOffset = 0;

			while (bRunning)
			{
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				RenderGradient(XOffset++, YOffset++);

				RECT ClientRect;
				GetClientRect(WindowHandle, &ClientRect);
				HDC DeviceContext = GetDC(WindowHandle);

				UpdateClientWindow(&ClientRect, DeviceContext, ClientRect.left, ClientRect.top, ClientRect.right - ClientRect.left, ClientRect.bottom - ClientRect.top);
			}
		}
	}

	return(0);
}