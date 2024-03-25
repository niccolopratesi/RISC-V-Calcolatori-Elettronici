#include "internal.h"
#include "kbd.h"
#include "uart.h"

using namespace kbd;

char char_read()
{	
	natb c;

	do {
		c = READ_UART_REG(UART_LSR);
	} while (!(c & 0x01));
	c = READ_UART();

	return c;
}
