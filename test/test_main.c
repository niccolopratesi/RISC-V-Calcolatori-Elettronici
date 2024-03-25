#include "types.h"
#include "uart.h"
#include "plic.h"

#define N_REG  31

__attribute__ ((aligned (16))) char stack0[4096];

// descrittore di processo
struct des_proc {
	uint16 id;
	uint16 livello;
	uint32 precedenza;
	uint64 punt_nucleo;
	uint64 contesto[N_REG];
	uint64 epc;
	uint64 satp;
	// paddr cr3; TODO: Insert pagination info

	struct des_proc *puntatore;
};
struct des_proc *esecuzione;
struct des_proc init;
struct des_proc *esecuzione_precedente;

int boot_main();
void test_stato_c();
void test_ctors_asm();
void test_keyboard_c();
void test_paginazione_c();

int boot_main(){
  
  pci_init();

  boot_printf("\n\nStarting tests...\n\r");

  plic_init();
  
  boot_printf("Starting salva/carica_stato test.\n\r");
  test_stato_c();
  boot_printf("Salva/carica_stato test done.\n\r");
  
  /*
  boot_printf("Starting ctors test.\n\r");
  test_ctors_asm();
  boot_printf("ctors test done.\n\r");
  */
  
  boot_printf("Starting keyboard test\n\r");
  test_keyboard_c();
  boot_printf("Keyboard test done\n\r");
  
  boot_printf("Starting paging test\n\r");
  test_paginazione_c();
  boot_printf("Paging test done\n\r");
  
  boot_printf("All tests done.\n\r");
  return 0;

}