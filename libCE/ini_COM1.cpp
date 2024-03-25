#include "libce.h"
#include "com1.h"
#include "uart.h"

using namespace com1;

void ini_COM1()
{
	natw CBITR = 0x000C;		// 9600 bit/sec.
	natb dummy;
	WRITE_UART_REG(UART_LCR,0x80);		// DLAB 1: Set bitrate in regs 0 and 1 while in this mode
	WRITE_UART_REG(0x00,CBITR);
	WRITE_UART_REG(0x01,CBITR >> 8);
	WRITE_UART_REG(UART_LCR,0x80);		// 1 bit STOP, 8 bit/car, parita dis, DLAB 0
	WRITE_UART_REG(UART_IER,0x00);		// richieste di interruzione disabilitate
}
