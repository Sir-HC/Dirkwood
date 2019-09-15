#if !defined(DIRKWOOD_H)

#if SLOWBUILD
#define Assert(Expression) 
#else
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#endif

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

struct game_memory
{
	bool32 IsInitialized;
	uint64 PermanentStorageSize;
	void* PermanentStorage;

	uint64 TransientStorageSize;
	void* TransientStorage;
};

struct game_state
{
	int BlueOffset;
	int GreenOffset;
	int toneHtz;
};

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

struct game_button_state {
	int halfTransitionCount;
	bool32 endedDown;
};

struct game_controller_input {
	real32 StartX;
	real32 StartY;

	real32 MinX;
	real32 MinY;

	real32 MaxX;
	real32 MaxY;

	real32 EndX;
	real32 EndY;

	bool32 IsAnalog;

	union {
		game_button_state buttons[6];
		struct {
			game_button_state Up;
			game_button_state Down;
			game_button_state Left;
			game_button_state Right;
			game_button_state LShoulder;
			game_button_state RShoulder;

		};
	};
};

struct game_input {

	game_controller_input controllers[4];
};

// timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
int GameUpdateAndRender(game_memory *memory, 
						game_input *input,
						game_offscreen_buffer *Buffer,
						game_sound_output_buffer *soundBuffer);


#define DIRKWOOD_H
#endif