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
	}
}

