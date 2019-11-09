#include "dirkwood.h"


internal void 
renderGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset) {

	uint8* row = (uint8*)Buffer->Memory;

	for (int y = 0; y < Buffer->Height; ++y)
	{
		uint32* pixel = (uint32*)row;
		for (int x = 0; x < Buffer->Width; ++x) {
			uint8 Blue = (x + XOffset);
			uint8 Green = (y + YOffset);
			uint8 Red = (y);

			*pixel++ = ((Red << 16) | (Green << 8) | Blue);
		}
		row += Buffer->Pitch;
	}
}

internal void
GameOutputSound(game_sound_output_buffer *soundBuffer, int toneHtz)
{
	local_persist real32 tSine;
	int16 toneVolume = 3000;

	int wavePeriod = soundBuffer->samplesPerSecond / toneHtz;

	int16* sampleOut = soundBuffer->samples;

	for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount; ++sampleIndex) {
		real32 sineValue = sinf(tSine);
		int16 sampleValue = (int16)(sineValue * toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;
		tSine += 2.0f * Pi32 * 1.0f / (real32)wavePeriod;
	}
}

internal int 
GameUpdateAndRender(game_memory* memory, game_input* input, game_offscreen_buffer* Buffer, game_sound_output_buffer* soundBuffer) {
	//TODO: allow sound offset for more functionality
	game_state* gameState = (game_state*)memory->PermanentStorage;
	if (!memory->IsInitialized)
	{
		gameState->toneHtz = 256;

		memory->IsInitialized = true;
	}
	
	for (int controllerIndex = 0; controllerIndex < ArrayCount(input->controllers); ++controllerIndex) {
		game_controller_input* input0 = &input->controllers[controllerIndex];



		if (input0->IsAnalog) {
			//Use analog movement
			gameState->toneHtz = 256 + (int)(128.0f * (input0->StickAverageX));
			gameState->BlueOffset += (int)4.0f * (input0->StickAverageY);
		}
		else
		{
			//Use digital movement

		}

		if (input0->MoveDown.endedDown) {
			gameState->GreenOffset += 5;
		}
	}
	

	GameOutputSound(soundBuffer, gameState->toneHtz);
	renderGradient(Buffer, gameState->BlueOffset, gameState->GreenOffset);

	return gameState->toneHtz;
}