#ifndef COLOUR_H
#define COLOUR_H

#include "util/int.h"

#define darken(c) ((c >> 1) & 0x7bef)
#define lighten(c) (darken(c) + 0x7bef)
#define mix(c1, c2) (darken(c1) + darken(c2))

// Fast RGB565 pixel blending
// Found in a pull request for the Adafruit framebuffer library. Clever!
// https://github.com/tricorderproject/arducordermini/pull/1/files#diff-d22a481ade4dbb4e41acc4d7c77f683d
inline u16 alphaBlendRGB565(u32 fg, u32 bg, u8 alpha) {
	// Converts  0000000000000000rrrrrggggggbbbbb
	//     into  00000gggggg00000rrrrr000000bbbbb
	// with mask 00000111111000001111100000011111
	// This is useful because it makes space for a parallel fixed-point multiply
	bg = (bg | (bg << 16)) & 0b00000111111000001111100000011111;
	fg = (fg | (fg << 16)) & 0b00000111111000001111100000011111;

	// This implements the linear interpolation formula: result = bg * (1.0 - alpha) + fg * alpha
	// This can be factorized into: result = bg + (fg - bg) * alpha
	// alpha is in Q1.5 format, so 0.0 is represented by 0, and 1.0 is represented by 32
	u32 result = (fg - bg) * alpha; // parallel fixed-point multiply of all components
	result >>= 5;
	result += bg;
	result &= 0b00000111111000001111100000011111; // mask out fractional parts
	return (u16)((result >> 16) | result); // contract result
}

#endif // COLOUR_H
