#include "keyboard.h"
#include "uart.h"

void test_keyboard_c(){

	uart_init();

    boot_printf("Press a character key: \n\r");
	read_char();
	boot_printf("The read character was: %c\n\r",testChar);

	boot_printf("Write something: ");
	read_string();
	boot_printf("\nYou have written: %s\n\r", uart_buf);

} 




