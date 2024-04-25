#include "tipo.h"
#include "uart.h"
#include <stdarg.h>
// uart is mapped in UART0
// for further informations regarding the memory mapping see qemu memory tree
// Reg info at: https://github.com/safinsingh/ns16550a/blob/master/src/ns16550a.s
// and at:      https://github.com/michaeljclark/riscv-probe/blob/master/libfemto/drivers/ns16550a.c


static void print_int(int xx, int base, int sign)
{
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    WRITE_UART(buf[i]);
}

static void print_ptr(uint64 x)
{
  int i;
  WRITE_UART('0');
  WRITE_UART('x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    WRITE_UART(digits[x >> (sizeof(uint64) * 8 - 4)]);
}



void boot_printf(char *fmt, ...)
{
  va_list ap;
  int i, c;
  char *s;

  if (fmt == 0)
    boot_panic("null fmt");

  va_start(ap, fmt);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      WRITE_UART(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'c':
      WRITE_UART((char)va_arg(ap, int));
      break;
    case 'd':
      print_int(va_arg(ap, int), 10, 1);
      break;
    case 'x':
      print_int(va_arg(ap, int), 16, 1);
      break;
    case 'p':
      print_ptr(va_arg(ap, uint64));
      break;
    case 's':
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s; s++)
        WRITE_UART(*s);
      break;
    case '%':
      WRITE_UART('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      WRITE_UART('%');
      WRITE_UART(c);
      break;
    }
  }
}

static void boot_panic(char *s)
{
  boot_printf("panic: ");
  boot_printf(s);
  boot_printf("\n");
  for(;;)
    ;
}

