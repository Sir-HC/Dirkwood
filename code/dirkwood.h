#if !defined(DIRKWOOD_H)

#if SLOWBUILD
#define Assert(Expression) 
#else
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#endif

#if !INTERNAL
struct debug_read_file_result {
	uint32 ContentSize;
	void* Contents;
};
internal debug_read_file_result DEBUGPlatformReadEntireFile(char* Filename);
internal void DEBUGPlatformFreeFileMemory(void* Memory);

internal bool32 DEBUGPlatformWriteEntireFile(char* Filename, uint32 MemorySize, void* Memory);

#endif

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

inline uint32
SafeTruncateUInt64(uint64 Value) {
	Assert(Value <= 0xFFFFFFFF);
	uint32 Result = (uint32)Value;
	return(Result);
}

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
	real32 StickAverageX;
	real32 StickAverageY;

	bool32 IsAnalog;

	union {
		game_button_state buttons[10];
		struct {
			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveLeft;
			game_button_state MoveRight;
			
			game_button_state ActionUp;
			game_button_state ActionDown;
			game_button_state ActionLeft;
			game_button_state ActionRight;
			
			game_button_state LShoulder;
			game_button_state RShoulder;

			game_button_state Back;
			game_button_state Start;

		};
	};
};

struct game_input {

	game_controller_input controllers[5];
};

// timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
int GameUpdateAndRender(game_memory *memory, 
						game_input *input,
						game_offscreen_buffer *Buffer,
						game_sound_output_buffer *soundBuffer);


#define DIRKWOOD_H
#endif