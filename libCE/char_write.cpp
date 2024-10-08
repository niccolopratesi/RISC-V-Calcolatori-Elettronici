#include "libce.h"
#include "vid.h"

namespace vid{

	void char_write(natb c) {
		switch (c) {
		case 0:
			break;
		case '\r':
			x = 0;
			break;
		case '\n':
			x = 0;
			y++;
			if (y >= ROWS)
				scroll();
			break;
		case '\b':
			if (x > 0 || y > 0) {
				if (x == 0) {
					x = COLS - 1;
					y--;
				} else {
					x--;
				}
			}
			break;
		default:
			//character byte
			video[y * COLS * 4 + x * 4] = c;
			//attribute byte
			video[y * COLS * 4 + x * 4 + 1] = attr;
			x++;
			if (x >= COLS) {
				x = 0;
				y++;
			}
			if (y >= ROWS)
				scroll();
			break;
		}
		cursore();
	}
}

