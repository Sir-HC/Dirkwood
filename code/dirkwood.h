#if !defined(DIRKWOOD_H)

struct game_offscreen_buffer {
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};

// timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
void GameUpdateAndRender(game_offscreen_buffer, int XOffset, int YOffset);


#define DIRKWOOD_H
#endif