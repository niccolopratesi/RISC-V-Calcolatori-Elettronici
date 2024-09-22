#include "vid.h"

namespace vid{

	void scroll()
	{

		for(unsigned int i = 0; i < VIDEO_SIZE*4 - COLS*4; i+=4){
			video[i] = video[i+COLS*4];
			video[i+1] = video[i+COLS*4+1];
		}

		for(unsigned int i = 0; i < COLS*4; i+=4){
			video[VIDEO_SIZE*4 - COLS*4 + i] = ' ';
			video[VIDEO_SIZE*4 - COLS*4 + i + 1] = attr;
		}

		if(y == 0){
			x = 0;
			cursore();
			return;
		}

		y--;
		cursore();
	}
}

