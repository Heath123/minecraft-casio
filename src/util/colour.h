#ifndef COLOUR_H
#define COLOUR_H

#define darken(c) ((c >> 1) & 0x7bef)
#define lighten(c) (darken(c) + 0x7bef)
#define mix(c1, c2) (darken(c1) + darken(c2))

#endif // COLOUR_H
