#include "types.h"
#include "uart.h"

__attribute__ ((aligned (16))) char stack0[4096];

int boot_main();
void print_VGA(char *message, uint8 fg, uint8 bg);

void timer_debug(){
  boot_printf("Timer fired\n\r");
}

int boot_main(){

  char* message = "Running in S-Mode\n\r";
  boot_printf(message);
  pci_init();
  message = "PCI initialized\n\r";
  boot_printf(message);
  message = "VGA inizialized\n\r";
  boot_printf(message);
  //Since we are still in a C environment, we should
  //call the ctors function before switching to sistema,
  //which uses a C++.
  //ctors();
  return 0;
}

