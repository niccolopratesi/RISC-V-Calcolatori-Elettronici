#include "libce.h"
#include "proc.h"
#include "plic.h"
#include "pci_risc.h"

#define N_REG  31

__attribute__ ((aligned (16))) char stack0[4096];

// // descrittore di processo
// struct des_proc {
// 	natw id;
// 	natw livello;
// 	natl precedenza;
// 	natq punt_nucleo;
// 	natq contesto[N_REG];
// 	natq epc;
// 	natq satp;
// 	// paddr cr3; TODO: Insert pagination info

// 	struct des_proc *puntatore;
// };
extern des_proc *esecuzione;
extern des_proc init;
extern des_proc *esecuzione_precedente;


extern "C" void test_stato_c();
extern "C" void test_keyboard_c();
extern "C" void test_paginazione_c();

extern "C" int boot_main(){
  flog(LOG_INFO, "Running in S-Mode");
  
  pci_init();

  flog(LOG_INFO, "Starting tests...");

  plic_init();
  
  flog(LOG_INFO, "Starting paging test");
  test_paginazione_c();
  flog(LOG_INFO, "Paging test done");
  
  flog(LOG_INFO, "Starting salva/carica_stato test.");
  test_stato_c();
  flog(LOG_INFO, "Salva/carica_stato test done.");  
  
  flog(LOG_INFO, "Starting keyboard test");
  test_keyboard_c();
  flog(LOG_INFO, "Keyboard test done");
  
  flog(LOG_INFO, "All tests done.");
  return 0;

}