#if !defined(DIRKWOOD_H)

struct game_offscreen_buffer {
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};

struct game_sound_output_buffer {
	int16* samples;
	int samplesPerSecond;
	int sampleCount;

};

// timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset,
						 game_sound_output_buffer *soundBuffer);


#define DIRKWOOD_H
#endif