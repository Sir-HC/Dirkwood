/* ==================================================
   Author: Chris Lynch


   ================================================== */

#include <windows.h>
#include <stdint.h>

#define local_persist static
#define global_variable static
#define internal static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;

internal void renderGradient(int XOffset, int YOffset) {
  int BytesPerPixel = 4;
  int pitch = BitmapWidth*BytesPerPixel;
  uint8 *row = (uint8 *)BitmapMemory;

  for(int y = 0; y < BitmapHeight; ++y)
  {
    uint8 *pixel = (uint8 *)row;
    for(int x = 0; x < BitmapWidth; ++x){
      *pixel = (uint8)(x + XOffset);//blue
      ++pixel;

      *pixel = (uint8)(x + YOffset);
      ++pixel;

      *pixel = (uint8)(y + YOffset);
      ++pixel;

      *pixel = 0;
      ++pixel;
    }
    row += pitch;
  }
}

internal void Win32ResizeDIBSection(int Width, int Height) {

  if(BitmapMemory){
    VirtualFree(BitmapMemory, 0, MEM_RELEASE);
  }

  BitmapWidth = Width;
  BitmapHeight = Height;

  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = Width;
  BitmapInfo.bmiHeader.biHeight = -Height;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  int BytesPerPixel = 4;
  int BitmapMemorySize = BytesPerPixel*(BitmapWidth*BitmapHeight);
  BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

}

internal void Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect, int X, int Y, int Width, int Height){

  int WindowWidth = ClientRect->right - ClientRect->left;
  int WindowHeight = ClientRect->bottom - ClientRect->top;

  StretchDIBits(DeviceContext,
    0,0, BitmapWidth, BitmapHeight,
    0,0, WindowWidth, WindowHeight,
    BitmapMemory,
    &BitmapInfo,
    DIB_RGB_COLORS,
    SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND   Window,
                                    UINT   Message,
                                    WPARAM WParam,
                                    LPARAM LParam)
{
  LRESULT Result = 0;
  switch(Message){
    case WM_SIZE:
    {
      RECT ClientRect;
      GetClientRect(Window, &ClientRect);
      int Width = ClientRect.right - ClientRect.left;
      int Height = ClientRect.bottom - ClientRect.top;
      Win32ResizeDIBSection(Width, Height);
      OutputDebugStringA("WM_SIZE\n");
    }break;

    case WM_DESTROY:
    {
      Running = false;
      OutputDebugStringA("WM_DESTROY\n");
    }break;

    case WM_CLOSE:
    {
      Running = false;
      OutputDebugStringA("WM_CLOSE\n");
    }break;

    case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    }break;

    case WM_PAINT:
    {
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window, &Paint);
      int X = Paint.rcPaint.left;
      int Y = Paint.rcPaint.top;
      int Width = Paint.rcPaint.right - Paint.rcPaint.left;
      int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

      RECT ClientRect;
      GetClientRect(Window, &ClientRect);

      Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);

      EndPaint(Window, &Paint);
    }

    default:
    {
      //OutputDebugStringA("default\n");
      Result = DefWindowProc(Window, Message, WParam, LParam);
    }break;
  }
  return(Result);
}


int CALLBACK WinMain(HINSTANCE Instance,
		                 HINSTANCE PrevInstance,
		                 LPSTR CommandLine,
		                 int ShowCode)
{
	WNDCLASS WindowClass = {};


  WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  //WindowClass.cbClsExtra;
  //WindowClass.cbWndExtra;
  WindowClass.hInstance = Instance;
  //WindowClass.hIcon;
  //WindowClass.hCursor;
  //WindowClass.hbrBackground;
  //WindowClass.lpszMenuName;
  WindowClass.lpszClassName = "DirkwoodWindowClass";

  if(RegisterClass(&WindowClass)){
    HWND Window= CreateWindowEx(
       0,
       WindowClass.lpszClassName,
       "Test",
       WS_OVERLAPPEDWINDOW | WS_VISIBLE,
       CW_USEDEFAULT,
       CW_USEDEFAULT,
       CW_USEDEFAULT,
       CW_USEDEFAULT,
       0,
       0,
       Instance,
       0);
     if(Window){
       MSG Message;
       Running = true;
       int XOffset = 0;
       int YOffset = 0;
       while(Running){

         while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)){
           if(Message.message == WM_QUIT){
             Running = false;
           }
           TranslateMessage(&Message);
           DispatchMessage(&Message);
         }

         renderGradient(XOffset, YOffset);

         HDC DeviceContext = GetDC(Window);
         RECT ClientRect;
         GetClientRect(Window, &ClientRect);
         int WindowWidth = ClientRect.right - ClientRect.left;
         int WindowHeight = ClientRect.bottom - ClientRect.top;
         Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
         ReleaseDC(Window, DeviceContext);
         XOffset++;
         YOffset++;
       }
     }
     else{
       //Log error
     }
  }
  else{
    //Log error
  }

  return(0);
}
