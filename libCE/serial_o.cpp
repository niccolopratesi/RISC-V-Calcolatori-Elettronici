#include "libce.h"
#include "com1.h"
#include "uart.h"

using namespace com1;

void serial_o(natb c)
{
	natb s;
	do {
		s = READ_UART_REG(UART_LSR);
	} while (! (s & 0x20));
	WRITE_UART(c);
}
