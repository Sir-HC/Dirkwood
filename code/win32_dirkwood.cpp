  /* ==================================================
   Author: Chris Lynch


   ================================================== */

#include <windows.h>
#include <stdint.h>
#include <xinput.h>

#define local_persist static
#define global_variable static
#define internal static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32 bool32;

struct win32_offscreen_buffer{
 BITMAPINFO Info;
 void *Memory;
 int Width;
 int Height;
 int Pitch;
 int BytesPerPixel;
};

struct win32_window_dimension{
  int Width;
  int Height;
};

internal void Win32InitDSound(void){
  // Load Lib
  // Get DirectSound object

}

internal win32_window_dimension Win32GetWindowDimension(HWND Window){
  win32_window_dimension Res;
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Res.Width = ClientRect.right - ClientRect.left;
  Res.Height = ClientRect.bottom - ClientRect.top;
  return Res;
}



#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub){
  return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub){
  return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void Win32LoadXInput(void){
  HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
  if(!XInputLibrary){
    HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
  }
  if(XInputLibrary){
    XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    if(!XInputGetState) {XInputGetState = XInputGetStateStub;}

    XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    if(!XInputSetState) {XInputSetState = XInputSetStateStub;}
  }

}

global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;

global_variable int XOffset = 0;
global_variable int YOffset = 0;

internal void renderGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset) {

  uint8 *row = (uint8 *)Buffer->Memory;

  for(int y = 0; y < Buffer->Height; ++y)
  {
    uint32 *pixel = (uint32 *)row;
    for(int x = 0; x < Buffer->Width; ++x){
      uint8 Blue = (x + XOffset);
      uint8 Green = (y + YOffset);
      uint8 Red = (y );

      *pixel++ = ((Red << 16) | (Green << 8) | Blue);
    }
    row += Buffer->Pitch;
  }
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height) {

  if(Buffer->Memory){
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
  }

  Buffer->Width = Width;
  Buffer->Height = Height;

  Buffer->BytesPerPixel = 4;
  Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;

  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Width;
  Buffer->Info.bmiHeader.biHeight = -Height;
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biCompression = BI_RGB;


  int BitmapMemorySize = Buffer->BytesPerPixel*(Buffer->Width*Buffer->Height);
  Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  int pitch = Buffer->Width*Buffer->BytesPerPixel;

}

internal void Win32DisplayBufferToWindow(win32_offscreen_buffer *Buffer,
                                         HDC DeviceContext, int WindowWidth, int WindowHeight){
  //Fix for aspect ratio
  StretchDIBits(DeviceContext,
    0,0, WindowWidth, WindowHeight,
    0,0, Buffer->Width, Buffer->Height,
    Buffer->Memory,
    &Buffer->Info,
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
    case WM_SIZE:{
      RECT ClientRect;
      win32_window_dimension Dimension = Win32GetWindowDimension(Window);
      Win32ResizeDIBSection(&GlobalBackBuffer, Dimension.Width, Dimension.Height);
      OutputDebugStringA("WM_SIZE\n");
    }break;

    case WM_DESTROY:{
      Running = false;
      OutputDebugStringA("WM_DESTROY\n");
    }break;

    case WM_CLOSE:{
      Running = false;
      OutputDebugStringA("WM_CLOSE\n");
    }break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:{
      uint32 VKCode = WParam;
      bool32 wasDown =  ((LParam & (1 << 30)) != 0);
      bool32 isDown = ((LParam & (1 << 31)) == 0);
      if(wasDown == isDown){
        break;
      }
      if( VKCode == 'W' ){

      }
      else if( VKCode == 'A'){

      }
      else if( VKCode == 'S'){

      }
      else if( VKCode == 'D'){

      }
      else if( VKCode == VK_UP){
        YOffset+=3;
      }
      else if( VKCode == VK_DOWN){
        YOffset-=3;
      }
      else if( VKCode == VK_LEFT){

      }
      else if( VKCode == VK_RIGHT){

      }
      else if( VKCode == VK_ESCAPE){
      }
      else if( VKCode == VK_SPACE){

      }
      else if( (VKCode == VK_F4) && ((LParam & (1 << 29)) != 0) ){
        //Alt & F4
        Running = false;
      }

    }break;

    case WM_ACTIVATEAPP:{
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    }break;

    case WM_PAINT:{
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window, &Paint);
      int X = Paint.rcPaint.left;
      int Y = Paint.rcPaint.top;
      int Width = Paint.rcPaint.right - Paint.rcPaint.left;
      int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;


      win32_window_dimension Dimension = Win32GetWindowDimension(Window);
      Win32DisplayBufferToWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);

      EndPaint(Window, &Paint);
    }

    default:{
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
  Win32LoadXInput();
  WNDCLASSA WindowClass = {};


  WindowClass.style = CS_HREDRAW|CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
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
       // int XOffset = 0;
       // int YOffset = 0;
       XINPUT_VIBRATION vib;

       Win32InitDSound();
       while(Running){

         while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)){
           if(Message.message == WM_QUIT){
             Running = false;
           }

           TranslateMessage(&Message);
           DispatchMessage(&Message);
         }


         for(DWORD index = 0; index < XUSER_MAX_COUNT; index++){
           XINPUT_STATE controllerState;
           if(XInputGetState(index, &controllerState) == ERROR_SUCCESS){
             // controller is plugged in
             // look at contollerState.dwPacketnUmber increment is high?
             XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

             bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
             bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
             bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
             bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
             bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
             bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
             bool lShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
             bool rShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
             bool aButton = (pad->wButtons & XINPUT_GAMEPAD_A);
             bool bButton = (pad->wButtons & XINPUT_GAMEPAD_B);
             bool xButton = (pad->wButtons & XINPUT_GAMEPAD_X);
             bool yButton = (pad->wButtons & XINPUT_GAMEPAD_Y);

             int16 stickX = pad->sThumbLX;
             int16 stickY = pad->sThumbLY;

             if(aButton){
               YOffset += 2;
             }
             if(bButton){
               XOffset -= 1;
             }
             if(yButton){

               vib.wLeftMotorSpeed += 200;
               vib.wRightMotorSpeed += 200;

               XInputSetState(0, &vib);
             }
             if(xButton){
               vib.wLeftMotorSpeed = 0;
               vib.wRightMotorSpeed = 0;
               XInputSetState(0, &vib);
             }

           }
           else{
             //controller not available
           }

         }

         renderGradient(&GlobalBackBuffer, XOffset, YOffset);

         HDC DeviceContext = GetDC(Window);
         win32_window_dimension Dimension = Win32GetWindowDimension(Window);
         Win32DisplayBufferToWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
         ReleaseDC(Window, DeviceContext);
         XOffset++;
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
