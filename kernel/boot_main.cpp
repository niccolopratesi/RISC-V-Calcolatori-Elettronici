#include "tipo.h"
#include "uart.h"
#include "pci_risc.h"

__attribute__ ((aligned (16))) char stack0[4096];

void print_VGA(char *message, natb fg, natb bg);

void timer_debug(){
  boot_printf("Timer fired\n\r");
}

extern "C" int boot_main(){

  char* message = "Running in S-Mode\n\r";
  boot_printf(message);
  pci_init();
  message = "PCI initialized\n\r";
  boot_printf(message);
  message = "VGA inizialized\n\r";
  boot_printf(message);
  return 0;
}

