#ifndef UART_H
#define UART_H
#ifdef __cplusplus
extern "C" {
#endif

#define UART0 0x10000000L
#define UART0_IRQ 10
#define UART_RHR 0x00
#define UART_THR 0x00               
#define UART_IER 0x01               // Interrupt Enable Register: bitmask of interrupt enables
#define UART_IER_RX_ENABLE 0x01
#define UART_IER_TX_ENABLE 0x02
#define UART_LCR 3                  // Line Control Register
#define UART_LSR 0x05               // Line Status Register: data available when set to 1
#define UART_LCR_BAUD_LATCH (1<<7)
#define UART_LCR_EIGHT_BITS (3<<0)
static char digits[] = "0123456789abcdef";
static void boot_panic(char *s);
void boot_printf(char *fmt, ...);
void uart_init();
void uart_intr();
#define WRITE_UART_REG(reg,c) (*(char *)(UART0 + reg) = c)
#define WRITE_UART(c) (WRITE_UART_REG(UART_THR,c))
#define READ_UART_REG(reg) (*((char *)(UART0 + reg)))
#define READ_UART() (READ_UART_REG(UART_RHR))

#ifdef __cplusplus
}
#endif
#endif