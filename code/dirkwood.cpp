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
GameUpdateAndRender(game_offscreen_buffer* Buffer, int XOffset, int YOffset) {

	renderGradient(Buffer, XOffset, YOffset);

}