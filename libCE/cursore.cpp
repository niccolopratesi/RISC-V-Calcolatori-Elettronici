#include "libce.h"
#include "vid.h"

using namespace vid;

void cursore()
{
	video[y * COLS + x] = attr | ' ';
	natb* VGA_BASE = (natb *)0x3000000;
	natl port = 0x3d4;
	natw pos = COLS * y + x;
	VGA_BASE[port] = CUR_HIGH;
    VGA_BASE[port + 1] = (natb)(pos >> 8);
	VGA_BASE[port] = CUR_LOW;
    VGA_BASE[port + 1] = (natb)pos;
}
