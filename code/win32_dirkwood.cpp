  /* ==================================================
   Author: Chris Lynch


   ================================================== */
#include <stdint.h>

#define local_persist static
#define global_variable static
#define internal static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#include <math.h>

#include "dirkwood.cpp"

#include <windows.h>
#include <xinput.h>
#include <dsound.h>

#include <stdio.h>

#include "win32_dirkwood.h"

#pragma comment(lib,"user32.lib") 
#pragma comment(lib,"gdi32.lib") 




global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER globalSecondaryBuffer;


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
Win32ClearSoundBuffer(win32_sound_output *soundOutput) {
	VOID* region1;
	DWORD region1Size;
	VOID* region2;
	DWORD region2Size;
	if (SUCCEEDED(globalSecondaryBuffer->Lock(0, soundOutput->secondaryBufferSize, &region1, &region1Size, &region2, &region2Size, 0))) {

		uint8* sampleOut = (uint8*)region1;

		for (DWORD byteIndex = 0; byteIndex < region1Size; ++byteIndex) {
			*sampleOut++ = 0;
		}

		sampleOut = (uint8*)region2;

		for (DWORD byteIndex = 0; byteIndex < region2Size; ++byteIndex) {
			*sampleOut++ = 0;
		}
		globalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);

	}
}

internal void
Win32FillSoundBuffer(win32_sound_output* soundOutput, DWORD byteToLock, DWORD bytesToWrite,
					 game_sound_output_buffer *soundBuffer) {
	VOID* region1;
	DWORD region1Size;
	VOID* region2;
	DWORD region2Size;

	if (SUCCEEDED(globalSecondaryBuffer->Lock(byteToLock, bytesToWrite, &region1, &region1Size, &region2, &region2Size, 0))) {
		
		DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample;

		int16* sampleOut = (int16*)region1;
		int16* sourceSample = soundBuffer->samples;

		for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex) {
			*sampleOut++ = *sourceSample++;
			*sampleOut++ = *sourceSample++;
			soundOutput->runningSampleIndex++;
		}

		sampleOut = (int16*)region2;
		DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
		for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex) {
			*sampleOut++ = *sourceSample++;
			*sampleOut++ = *sourceSample++;
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

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state *oldState, DWORD buttonBit,
										      game_button_state *newState) {
	newState->endedDown = ((XInputButtonState & buttonBit) == buttonBit);
	newState->halfTransitionCount = (oldState->endedDown != newState->endedDown) ? 1 : 0;

}

internal LRESULT CALLBACK Win32MainWindowCallback(HWND   Window,
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
		
			}
			else if( VKCode == VK_DOWN){
		
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
	LARGE_INTEGER perfCounterFrequency;
	QueryPerformanceFrequency(&perfCounterFrequency);
	uint64 countFrequency = perfCounterFrequency.QuadPart;

	Win32LoadXInput();
	WNDCLASSA WindowClass = {};

	WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
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
			
#if DIRKWOOD_INTERNAL
			LPVOID BaseAddress = (LPVOID)Terabytes((uint64)2);
#else
			LPVOID BaseAddress = (LPVOID)0;
#endif

			game_memory gameMemory = {};
			gameMemory.PermanentStorageSize = Megabytes(64);
			gameMemory.TransientStorageSize = Gigabytes(2);

			uint64 TotalSize = gameMemory.PermanentStorageSize + gameMemory.TransientStorageSize;
			gameMemory.PermanentStorage = VirtualAlloc(BaseAddress, TotalSize,
				MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			gameMemory.TransientStorage = ((uint8*)gameMemory.PermanentStorage + gameMemory.PermanentStorageSize);

			win32_sound_output soundOutput = {};
			soundOutput.samplesPerSecond = 48000;
			soundOutput.runningSampleIndex = 0;
			soundOutput.bytesPerSample = sizeof(int16) * 2;
			soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
			soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;
			int16* Samples = (int16*)VirtualAlloc(0, soundOutput.secondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			
			Win32InitDSound(Window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);
			Win32ClearSoundBuffer(&soundOutput);
			globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			
			game_input input[2] = {};
			game_input* newInput = &input[0];
			game_input* oldInput = &input[1];

			XINPUT_VIBRATION vib;
			vib.wLeftMotorSpeed = 0;
			vib.wRightMotorSpeed = 0;

			LARGE_INTEGER lastCounter;
			QueryPerformanceCounter(&lastCounter);
			uint64 lastCycleCount = __rdtsc();

			Running = true;
			while(Running){
				MSG Message;

				while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)){
					if(Message.message == WM_QUIT){
						Running = false;
					}

					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				int maxControllerCount = XUSER_MAX_COUNT;
				if (maxControllerCount > ArrayCount(input->controllers)) {
					maxControllerCount = ArrayCount(input->controllers);
				}
				for(DWORD controllerIndex = 0; controllerIndex < maxControllerCount; controllerIndex++){
					XINPUT_STATE controllerState;
					game_controller_input* oldController = &oldInput->controllers[controllerIndex];
					game_controller_input* newController = &newInput->controllers[controllerIndex];
					if(XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS){
						// controller is plugged in
						// look at contollerState.dwPacketnUmber increment is high?
						XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

						bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						newController->IsAnalog = true;
						newController->StartX = oldController->EndX;
						newController->StartY = oldController->EndY;
						
						real32 X;
						if (pad->sThumbLX < 0) {
							X = (real32)pad->sThumbLX / 32768.0f;
						}
						else {
							X = (real32)pad->sThumbLX / 32767.0f;
						}

						
						newController->MinX = newController->MaxX = newController->EndX = X;
						
						real32 Y;
						if (pad->sThumbLY < 0) {
							Y = (real32)pad->sThumbLY / 32768.0f;
						}
						else {
							Y = (real32)pad->sThumbLY / 32767.0f;
						}

						newController->MinY = newController->MaxY = newController->EndY = Y;

						Win32ProcessXInputDigitalButton(pad->wButtons, &oldController->Down, XINPUT_GAMEPAD_A,
							&newController->Down);
						Win32ProcessXInputDigitalButton(pad->wButtons, &oldController->Right, XINPUT_GAMEPAD_B,
							&newController->Right);
						Win32ProcessXInputDigitalButton(pad->wButtons, &oldController->Left, XINPUT_GAMEPAD_X,
							&newController->Left);
						Win32ProcessXInputDigitalButton(pad->wButtons, &oldController->Up, XINPUT_GAMEPAD_Y,
							&newController->Up);
						Win32ProcessXInputDigitalButton(pad->wButtons, &oldController->LShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
							&newController->LShoulder);
						Win32ProcessXInputDigitalButton(pad->wButtons, &oldController->RShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
							&newController->RShoulder);

						/*
						bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
						bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
						*/

					}
					else{
						//controller not available
					}
				}
				game_offscreen_buffer preScreenBuffer = {};
				preScreenBuffer.Memory = GlobalBackBuffer.Memory;
				preScreenBuffer.Width = GlobalBackBuffer.Width;
				preScreenBuffer.Height = GlobalBackBuffer.Height;
				preScreenBuffer.Pitch = GlobalBackBuffer.Pitch;

				DWORD playCursor;
				DWORD writeCursor;
				DWORD bytesToWrite;
				DWORD byteToLock;
				DWORD targetCursor;

				bool32 soundIsValid = false;

				if (SUCCEEDED(globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor))) {
					
					byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;
					targetCursor = (playCursor + (soundOutput.latencySampleCount * soundOutput.bytesPerSample)) % soundOutput.secondaryBufferSize;
					if (byteToLock > targetCursor) {
						bytesToWrite = soundOutput.secondaryBufferSize - byteToLock;
						bytesToWrite += targetCursor;
					}
					else {
						bytesToWrite = targetCursor - byteToLock;
					}
					soundIsValid = true;
				}

				game_sound_output_buffer soundBuffer = {};
				soundBuffer.samples = Samples;
				soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
				soundBuffer.sampleCount = bytesToWrite/soundOutput.bytesPerSample;

				int tone = GameUpdateAndRender(&gameMemory, newInput, &preScreenBuffer, &soundBuffer);

				// Sound output testing
				
				if(soundIsValid){
					Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuffer);
				}

				HDC DeviceContext = GetDC(Window);

				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferToWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
				
				ReleaseDC(Window, DeviceContext);
				
				//Performance & Debug================================
				uint64 endCycleCount = __rdtsc();
				uint64 cyclesElasped = endCycleCount - lastCycleCount;

				LARGE_INTEGER endCounter;
				QueryPerformanceCounter(&endCounter);

				int64 counterElasped = endCounter.QuadPart - lastCounter.QuadPart;
				real32 msPerFrame = (real32)(1000.0f * (real32)counterElasped / (real32)countFrequency);
				real32 fps = (int32)countFrequency / counterElasped;
				real32 MCPF = (real32)((cyclesElasped) / (1000.0f * 1000.0f));

				char Buffer[256];
				sprintf(Buffer, "%.02fms - %.02fFPS - %.02f Mcycles/f - %d - %lu\n", msPerFrame, fps, MCPF, tone, bytesToWrite);
				//OutputDebugStringA(Buffer);
				//END Performance & Debug=============================END

				lastCycleCount = endCycleCount;
				lastCounter = endCounter;

				game_input* temp = newInput;
				newInput = oldInput;
				oldInput = temp;
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
