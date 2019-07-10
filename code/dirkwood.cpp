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
	int16 toneVolume = 300;

	int wavePeriod = soundBuffer->samplesPerSecond / toneHtz;

	int16* sampleOut = soundBuffer->samples;

	for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount; ++sampleIndex) {
		real32 sineValue = sinf(tSine);
		int16 sampleValue = (int16)(sineValue * toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;
		tSine += 2.0f * Pi32 / (real32)wavePeriod;
	}
}

internal void 
GameUpdateAndRender(game_offscreen_buffer* Buffer, int XOffset, int YOffset, game_sound_output_buffer* soundBuffer, int toneHtz) {
	//TODO: allow sound offset for more functionality
	GameOutputSound(soundBuffer, toneHtz);
	renderGradient(Buffer, XOffset, YOffset);

}