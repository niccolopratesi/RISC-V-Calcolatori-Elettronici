#include "libce.h"
#include "kbd.h"
#include "uart.h"

using namespace kbd;

void enable_intr_kbd()
{
	natb intMask = READ_UART_REG(UART_IER);
	WRITE_UART_REG(UART_IER,intMask|UART_IER_TX_ENABLE);
}
