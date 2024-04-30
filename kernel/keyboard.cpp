#include "libce.h"
#include "uart.h"
#include "keyboard.h"

extern "C" int readSSIP();

const natl MAX_CODE = 42;
natb tab[MAX_CODE] = {
	0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
	0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
	0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x39, 0x1C, 0x0e, 0x01
};
char tabmin[MAX_CODE] = {
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
	'z', 'x', 'c', 'v', 'b', 'n', 'm', ' ', '\n', '\b', 0x1b
};
char tabmai[MAX_CODE] = {
	'!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
	'Z', 'X', 'C', 'V', 'B', 'N', 'M', ' ', '\r', '\b', 0x1b
};
bool shift = false;

char read_byte(long memAddr){
    return *((char *)memAddr);
}

char conv(natb c) {
	char cc;
	natl pos = 0;
	while (pos < MAX_CODE && tab[pos] != c)
		pos++;
	if (pos == MAX_CODE)
		return 0;
	if (shift)
		cc = tabmai[pos];
	else
		cc = tabmin[pos];
	return cc;
}

void uart_init() {
	
	// disable interrupts
	WRITE_UART_REG(UART_IER, 0x00);

	// special mode to set baud rate
	WRITE_UART_REG(UART_LCR, UART_LCR_BAUD_LATCH);

	// LSB for baud rate of 38.4K
	WRITE_UART_REG(0, 0x03);

	// MSB for baud rate of 38.4K
	WRITE_UART_REG(1, 0x00);

	// leave set-baud modde,
	// and set word lenght to 8 bits, no parity
	WRITE_UART_REG(UART_LCR, UART_LCR_EIGHT_BITS);

	// Enable receive interrupts
 	WRITE_UART_REG(UART_IER, UART_IER_RX_ENABLE);

}

char testChar;
bool read = false;
int string = 0;

#define BUF_SIZE	32

char uart_buf[BUF_SIZE];
natq uart_tx_w;

void uart_intr() {

	if (string == 1) {

		testChar = (char)READ_UART();

		if (READ_UART() == 32)
			boot_printf(" ");

		else
			boot_printf("%c", testChar);

		uart_buf[uart_tx_w % BUF_SIZE] = testChar;
		uart_tx_w += 1;

	}

	else {
		
		flog(LOG_INFO,"Interruzione UART");

		testChar = (char)READ_UART();

		// Segnalo che il carattere è arrivato
		read = true;
	}

}

void read_char() {
	natb c;

	do {

		string = 0;
		c = READ_UART_REG(UART_LSR);

	} while ((!(c & 0x01)) && !read);
	
}

void read_string() {
	natb c;

	do {

		string = 1;
		c = READ_UART_REG(UART_LSR);

	} while ((!(c & 0x01)) && !(READ_UART() == 13));

}

void test_keyboard_c(){

	uart_init();

    boot_printf("Press a character key: \n\r");
	read_char();
	boot_printf("The read character was: %c\n\r",testChar);

	boot_printf("Write something: ");
	read_string();
	boot_printf("\nYou have written: %s\n\r", uart_buf);

} 




