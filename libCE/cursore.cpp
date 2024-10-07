#include "vid.h"

namespace vid{

	void cursore()
	{
		natl port = CRTC;
		natw pos = COLS * y + x;
		VGA_BASE[port] = CUR_HIGH;
		VGA_BASE[port + 1] = (natb)(pos >> 8);
		VGA_BASE[port] = CUR_LOW;
		VGA_BASE[port + 1] = (natb)pos;
	}
}

