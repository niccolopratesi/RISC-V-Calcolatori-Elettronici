#include "libce.h"
#include "vid.h"

namespace vid{

	void clear_screen(natb attribute) 
	{

		for(int i=0;i<VIDEO_SIZE*4;i+=4){
			video[i] = ' ';
			video[i+1] = attribute;
		}
		attr = attribute;

		writeport(CRTC, 0x0e, 0x00);
		writeport(CRTC, 0x0f, 0x00);
	}
}

