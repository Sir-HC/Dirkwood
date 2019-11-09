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



internal debug_read_file_result DEBUGPlatformReadEntireFile(char* Filename) {
	debug_read_file_result Result = {};
	HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ,
		FILE_SHARE_READ, 0,
		OPEN_EXISTING, 0, 0);
	
	if (FileHandle != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize)) {
			uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents) {
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize.QuadPart, &BytesRead, 0) && (FileSize32 == BytesRead)) {
					// File Read Successful
					Result.ContentSize = FileSize32;
				}
				else {
					DEBUGPlatformFreeFileMemory(Result.Contents);
					Result.Contents = 0;
				}
			}
		}
		CloseHandle(FileHandle);
	}
	return Result;
}

internal void DEBUGPlatformFreeFileMemory(void* Memory) {
	if (Memory) {
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

internal bool32 DEBUGPlatformWriteEntireFile(char* Filename, uint32 MemorySize, void* Memory) {
	bool32 Result = false;
	HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE,
		0, 0,
		CREATE_ALWAYS, 0, 0);

	if (FileHandle != INVALID_HANDLE_VALUE) {
		DWORD BytesWritten;
		if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0)) {
			Result = (BytesWritten == MemorySize);
		}
		CloseHandle(FileHandle);
	}
	return Result;
}


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

internal void Win32ProcessKeyboardMessage(game_button_state *newState, bool32 isDown) {
	Assert(newState->endedDown != isDown)
	newState->endedDown = isDown;
	++newState->halfTransitionCount;
}

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state* oldState, DWORD buttonBit,
	game_button_state* newState) {
	newState->endedDown = ((XInputButtonState & buttonBit) == buttonBit);
	newState->halfTransitionCount = (oldState->endedDown != newState->endedDown) ? 1 : 0;

}

internal real32 Win32ProcessInputStickValue(SHORT thumbVal, SHORT deadzone) {
	real32 result = 0;
	if (thumbVal < -deadzone) {
		result = (real32)thumbVal / 32768.0f;
	}
	else if (thumbVal > deadzone) {
		result = (real32)thumbVal / 32767.0f;
	}
	return(result);
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
			Assert("Key came in through other path")
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

void Win32ProcessPendingMessages(game_controller_input* KeyboardController) {
	MSG Message;
	while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
		switch (Message.message) {
			case WM_QUIT: {
				Running = false;
				break;
			}
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP: {
				uint32 VKCode = (uint32)Message.wParam;
				bool32 wasDown = ((Message.lParam & (1 << 30)) != 0);
				bool32 isDown = ((Message.lParam & (1 << 31)) == 0);
				if (wasDown == isDown) {
					break;
				}
				if (VKCode == 'W') {

				}
				else if (VKCode == 'A') {

				}
				else if (VKCode == 'S') {

				}
				else if (VKCode == 'D') {

				}
				else if (VKCode == VK_UP) {
					Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, isDown);
				}
				else if (VKCode == VK_DOWN) {
					Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, isDown);
				}
				else if (VKCode == VK_LEFT) {
					Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, isDown);
				}
				else if (VKCode == VK_RIGHT) {
					Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, isDown);
				}
				else if (VKCode == VK_ESCAPE) {
				}
				else if (VKCode == VK_SPACE) {

				}
				else if ((VKCode == VK_F4) && ((Message.lParam & (1 << 29)) != 0)) {
					//Alt & F4
					Running = false;
				}

			}break;

			default: {
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			}break;
		}
	}
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

	if(RegisterClassA(&WindowClass)){
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
				
				game_controller_input* oldKeyboardController = &oldInput->controllers[0];
				game_controller_input* newKeyboardController = &newInput->controllers[0];
				game_controller_input zeroController = {};
				*newKeyboardController = zeroController;
				for (int ButtonIndex = 0; ButtonIndex < ArrayCount(newKeyboardController->buttons); ++ButtonIndex) {
					newKeyboardController->buttons[ButtonIndex].endedDown =
						oldKeyboardController->buttons[ButtonIndex].endedDown;
				}

				Win32ProcessPendingMessages(newKeyboardController);

				int maxControllerCount = XUSER_MAX_COUNT + 1;
				if (maxControllerCount > ArrayCount(input->controllers)) {
					maxControllerCount = ArrayCount(input->controllers);
				}
				//char xBuff[256];
				//sprintf(xBuff, "Entering contoller loop");
				for(DWORD controllerIndex = 0; controllerIndex < maxControllerCount - 1 ; controllerIndex++){
					XINPUT_STATE controllerState;
					DWORD padControllerIndex = controllerIndex + 1;
					game_controller_input* oldController = &oldInput->controllers[padControllerIndex];
					game_controller_input* newController = &newInput->controllers[padControllerIndex];
					if(XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS){
						// controller is plugged in
						// look at contollerState.dwPacketnUmber increment is high?
						XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

						bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						
						newController->IsAnalog = true;
						newController->StickAverageX = Win32ProcessInputStickValue(pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
						newController->StickAverageY = Win32ProcessInputStickValue(pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
						 
						//char Buffer[256];
						//sprintf(Buffer, "x %.02f - y %.02f \n", X, Y);
						//OutputDebugStringA(Buffer);

						if (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) {
							newController->StickAverageY = 1.0f;
							newController->IsAnalog = false;
						}
						if (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) {
							newController->StickAverageY = -1.0f;
							newController->IsAnalog = false;
						}
						if (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
							newController->StickAverageX = -1.0f;
							newController->IsAnalog = false;
						}
						if (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
							newController->StickAverageX = 1.0f;
							newController->IsAnalog = false;
						}

						real32 threshold = 0.5f;

						Win32ProcessXInputDigitalButton(
							(newController->StickAverageX < -threshold) ? 1 : 0, 
							&oldController->MoveLeft, 1, &newController->MoveLeft);
						Win32ProcessXInputDigitalButton(
							(newController->StickAverageX > threshold) ? 1 : 0, 
							&oldController->MoveLeft, 1, &newController->MoveLeft);
						Win32ProcessXInputDigitalButton(
							(newController->StickAverageX < -threshold) ? 1 : 0,
							&oldController->MoveLeft, 1, &newController->MoveLeft);
						Win32ProcessXInputDigitalButton(
							(newController->StickAverageX > threshold) ? 1 : 0,
							&oldController->MoveLeft, 1, &newController->MoveLeft);


						Win32ProcessXInputDigitalButton(pad->wButtons, &oldController->ActionDown, XINPUT_GAMEPAD_A,
							&newController->ActionDown);
						Win32ProcessXInputDigitalButton(pad->wButtons, &oldController->ActionRight, XINPUT_GAMEPAD_B,
							&newController->ActionRight);
						Win32ProcessXInputDigitalButton(pad->wButtons, &oldController->ActionLeft, XINPUT_GAMEPAD_X,
							&newController->ActionLeft);
						Win32ProcessXInputDigitalButton(pad->wButtons, &oldController->ActionUp, XINPUT_GAMEPAD_Y,
							&newController->ActionUp);
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

				//char Buffer[256];
				//sprintf(Buffer, "%.02fms - %.02fFPS - %.02f Mcycles/f - %d - %lu\n", msPerFrame, fps, MCPF, tone, bytesToWrite);
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
