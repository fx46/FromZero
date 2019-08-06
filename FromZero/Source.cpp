#include <windows.h>

static bool bRunning = true;
static BITMAPINFO BitmapInfo;
static void *BitmapMemory;
static HBITMAP BitmapHandle;
static HDC BitmapDeviceContext;

static void ResizeDIBSection(int Width, int Height)
{
	if (BitmapHandle)
	{
		DeleteObject(BitmapHandle);
	}
	
	if (BitmapDeviceContext)
	{
		BitmapDeviceContext = CreateCompatibleDC(0);
	}

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = Width;
	BitmapInfo.bmiHeader.biHeight = Height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	BitmapHandle = CreateDIBSection(BitmapDeviceContext, &BitmapInfo, DIB_RGB_COLORS, &BitmapMemory, 0, 0);
}

static void UpdateClientWindow(HDC DeviceContext, int Left, int Top, int Width, int Height)
{
	StretchDIBits(DeviceContext, Left, Top, Width, Height, Left, Top, Width, Height, BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
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
			UpdateClientWindow(DeviceContext, P.rcPaint.left, P.rcPaint.top, P.rcPaint.right - P.rcPaint.left, P.rcPaint.bottom - P.rcPaint.top);
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
			while (bRunning)
			{
				BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
				if (MessageResult > 0)
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}
				else
				{
					break;
				}
			}
		}
	}

	return(0);
}