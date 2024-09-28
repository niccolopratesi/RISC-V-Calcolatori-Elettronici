#include "internal.h"
#include "kbd.h"
#include "costanti.h"

using namespace kbd;

void reboot()
{
	for(;;);
	(*(volatile natl *)VIRT_TEST) = FINISHER_PASS;
}
