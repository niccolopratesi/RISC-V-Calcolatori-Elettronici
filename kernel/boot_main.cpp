#include "libce.h"
#include "uart.h"
#include "plic.h"
#include "pci_risc.h"

__attribute__ ((aligned (16))) char stack0[4096];

void timer_debug(){
  flog(LOG_INFO, "Timer fired");
}

extern "C" int boot_main(){

  flog(LOG_INFO, "Running in S-Mode");

  pci_init();
  flog(LOG_INFO, "PCI initialized");
  flog(LOG_INFO, "VGA inizialized");

  plic_init();
  flog(LOG_INFO, "PLIC Initialized\n");

  /* paginazione */
  return 0;
}

