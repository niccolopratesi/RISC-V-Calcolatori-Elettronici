#include "internal.h"
#include "kbd.h"

using namespace kbd;

void reboot()
{
	for(;;);
	//outputb(0xFE, iCMR);
}
