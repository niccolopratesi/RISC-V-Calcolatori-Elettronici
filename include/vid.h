#ifndef VID_H
#define VID_H
#include "tipo.h"

/*
//standard vga ports
#define AC 0x3c0            //attribute or palette registers                       
#define AC_READ 0x3c1       //attribute read register                            
#define MISC 0x3c2          //miscellaneous register                                       
#define MISC_READ 0x3cc     //miscellaneous read register                      
#define SEQ 0x3c4           //sequencer                                    
#define GC 0x3ce            //graphic address register                     
#define CRTC 0x3d4          //cathode ray tube controller address register
#define PC 0x3c8            //PEL write index                                
#define PD 0x3c9            //PEL write data                                           
#define INPUT_STATUS_REGISTER 0x3da
*/

namespace vid {
	//remapped VGA ports into qemu
	#define AC                    0x400           //attribute or palette registers              
	#define AC_READ               0x401           //attribute read register                     
	#define MISC                  0x402           //miscellaneous register                        
	#define MISC_READ             0x40c           //miscellaneous read register                 
	#define SEQ                   0x404           //sequencer                                     
	#define GC                    0x40e           //graphic address register                      
	#define CRTC                  0x414           //cathode ray tube controller address register  
	#define PC                    0x408           //PEL write index                               
	#define PD                    0x409           //PEL write data                              
	#define INPUT_STATUS_REGISTER 0x41a

	const natl COLS = 80;
	const natl ROWS = 25;
	const natl VIDEO_SIZE = COLS * ROWS;
	const natb CUR_HIGH = 0x0e;
	const natb CUR_LOW = 0x0f;

	extern volatile natb* VGA_BASE;
	extern volatile natb* video;
	extern natb x, y;
	extern natb attr;
	
	void cursore();
	void scroll();
}
#endif
