#include <windows.h>

LRESULT CALLBACK MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;
	if (Message == WM_SIZE)
	{
		OutputDebugStringA("WM_SIZE\n");
	}
	else if (Message == WM_DESTROY)
	{
		OutputDebugStringA("WM_DESTROY\n");
	}
	else if (Message == WM_CLOSE)
	{
		OutputDebugStringA("WM_CLOSE\n");
	}
	else if (Message == WM_ACTIVATEAPP)
	{
		OutputDebugStringA("WM_ACTIVATEAPP\n");
	}
	else if (Message == WM_PAINT)
	{
		PAINTSTRUCT P;
		HDC DeviceContext = BeginPaint(Window, &P);
		PatBlt(DeviceContext, P.rcPaint.left, P.rcPaint.top, P.rcPaint.right - P.rcPaint.left, P.rcPaint.bottom - P.rcPaint.top, WHITENESS);
		EndPaint(Window, &P);
	}
	else
	{
		OutputDebugStringA("Default message received.\n");
		Result = DefWindowProc(Window, Message, WParam, LParam);
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
			for (;;)
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