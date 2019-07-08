  /* ==================================================
   Author: Chris Lynch


   ================================================== */

#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>

#pragma comment(lib,"user32.lib") 
#pragma comment(lib,"gdi32.lib") 


#define local_persist static
#define global_variable static
#define internal static
#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32 bool32;

typedef float real32;
typedef double real64;

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

struct win32_sound_output {
	int samplesPerSecond;
	int ToneHtz;
	uint32 runningSampleIndex;
	int wavePeriod;
	int bytesPerSample;
	int secondaryBufferSize;
	uint16 volume;
	real32 tSine;
	int latencySampleCount;
};

global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER globalSecondaryBuffer;
global_variable int XOffset = 0;
global_variable int YOffset = 0;


/***************************
Get XInput (game controller) Library
***************************/
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
	if (!XInputLibrary) {
		XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
	}

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

/***************************
Get Direct Sound Library
***************************/
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);
internal void Win32InitDSound(HWND Window, int32 samplesPerSec, int32 bufferSize){
	// Load Lib
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	if (DSoundLibrary) {
		// Get DirectSound object
		direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
		LPDIRECTSOUND directSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0))) {
			WAVEFORMATEX waveFormat = {};
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nChannels = 2;
			waveFormat.nSamplesPerSec = samplesPerSec;
			waveFormat.wBitsPerSample = 16;
			waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
			waveFormat.nAvgBytesPerSec = waveFormat.nBlockAlign * waveFormat.nSamplesPerSec;
			waveFormat.cbSize = 0;
			if (SUCCEEDED(directSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {
				DSBUFFERDESC bufferDescription = {};
				bufferDescription.dwSize = sizeof(bufferDescription);
				bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
				LPDIRECTSOUNDBUFFER primaryBuffer;
				if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0))) {
					HRESULT error = primaryBuffer->SetFormat(&waveFormat);
					if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat))) {
						OutputDebugStringA("Primary Buffer format was set.\n");
						// Format has been set
					}
					else {
						// Log Buffer formatting failed
					}
				}
				else {
					// Log Sound buffer creation failed
				}
			}
			else {
				// Log link with window program failed
			}		//END SET COOPERATIVE LEVEL

			//Create secondary bufferSize
			DSBUFFERDESC bufferDescription = {};
			bufferDescription.dwSize = sizeof(bufferDescription);
			bufferDescription.dwFlags = 0;
			bufferDescription.dwBufferBytes = bufferSize;
			bufferDescription.lpwfxFormat = &waveFormat;
			HRESULT error = directSound->CreateSoundBuffer(&bufferDescription, &globalSecondaryBuffer, 0);


			if (SUCCEEDED(error)) {
				OutputDebugStringA("Secondary buffer created\n");
			}
			else {
				// Log Sound buffer creation failed
			}
		}
		else {
			//Log Direct sound init failed
		}
	}
}

internal void
Win32FillSoundBuffer(win32_sound_output* soundOutput, DWORD byteToLock, DWORD bytesToWrite) {
	VOID* region1;
	DWORD region1Size;
	VOID* region2;
	DWORD region2Size;

	if (SUCCEEDED(globalSecondaryBuffer->Lock(byteToLock, bytesToWrite, &region1, &region1Size, &region2, &region2Size, 0))) {
		int16* sampleOut = (int16*)region1;
		DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample;

		for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex) {
			//real32 t = 2.0f * Pi32 * (real32)soundOutput->runningSampleIndex / soundOutput->wavePeriod;
			real32 sineValue = sinf(soundOutput->tSine);
			int16 sampleValue = (int16)(sineValue * soundOutput->volume);
			//int16 sampleValue = (runningSampleIndex++ / halfWavePeriod) % 2 ? volume : -volume;
			*sampleOut++ = sampleValue;
			*sampleOut++ = sampleValue;
			soundOutput->tSine += 2.0f * Pi32 / (real32)soundOutput->wavePeriod;
			soundOutput->runningSampleIndex++;
		}
		sampleOut = (int16*)region2;
		DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
		for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex) {
			//real32 t = 2.0f * Pi32 * (real32)soundOutput->runningSampleIndex / soundOutput->wavePeriod;
			real32 sineValue = sinf(soundOutput->tSine);
			int16 sampleValue = (int16)(sineValue * soundOutput->volume);
			//int16 sampleValue = (runningSampleIndex++ / halfWavePeriod) % 2 ? volume : -volume;
			*sampleOut++ = sampleValue;
			*sampleOut++ = sampleValue;
			soundOutput->tSine += 2.0f * Pi32 / (real32)soundOutput->wavePeriod;
			soundOutput->runningSampleIndex++;
		}
		// Unlock
		globalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
	}
}

internal win32_window_dimension
Win32GetWindowDimension(HWND Window) {
	win32_window_dimension Res;
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Res.Width = ClientRect.right - ClientRect.left;
	Res.Height = ClientRect.bottom - ClientRect.top;
	return Res;
}

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
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

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
		Result = DefWindowProcA(Window, Message, WParam, LParam);
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
	   
	   
		win32_sound_output soundOutput = {};
		soundOutput.samplesPerSecond = 48000;
		soundOutput.ToneHtz = 256;
		soundOutput.runningSampleIndex = 0;
		soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.ToneHtz;
		soundOutput.bytesPerSample = sizeof(int16) * 2;
		soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
		soundOutput.volume = 1000;
		soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;
		Win32InitDSound(Window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);
		Win32FillSoundBuffer(&soundOutput, 0, soundOutput.latencySampleCount * soundOutput.bytesPerSample);
		globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

		XINPUT_VIBRATION vib;
		vib.wLeftMotorSpeed = 0;
		vib.wRightMotorSpeed = 0;
	   
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
					soundOutput.ToneHtz = 512 + (int)(256.0f * ((real32)stickY / 30000.0f));
					soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.ToneHtz;
				}
				else{
					//controller not available
				}
			}

			renderGradient(&GlobalBackBuffer, XOffset, YOffset);

			// Sound output testing
			DWORD playCursor;
			DWORD writeCursor;
			if(SUCCEEDED(globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor))){
				DWORD bytesToWrite;
				DWORD byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;
				DWORD targetCursor = (playCursor + (soundOutput.latencySampleCount * soundOutput.bytesPerSample)) % soundOutput.secondaryBufferSize;
				if (byteToLock > targetCursor) {
					bytesToWrite = soundOutput.secondaryBufferSize - byteToLock;
					bytesToWrite += targetCursor;
				}
				else {
						bytesToWrite = targetCursor - byteToLock;
				}
				Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite);
			}

			HDC DeviceContext = GetDC(Window);
			win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			Win32DisplayBufferToWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
			ReleaseDC(Window, DeviceContext);
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
