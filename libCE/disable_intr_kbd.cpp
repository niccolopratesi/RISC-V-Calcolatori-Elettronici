#include "libce.h"
#include "kbd.h"
#include "uart.h"

using namespace kbd;

void disable_intr_kbd()
{
	WRITE_UART_REG(UART_IER,0x00);
}
