#include "types.h"

#define N_REG  31

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
extern des_proc *esecuzione;
extern des_proc init;
//struct des_proc *esecuzione_precedente;

extern "C" void test_stato_asm();
extern "C" void boot_printf(char *fmt, ...);

extern "C" void salva_success(){
    boot_printf("Salva_stato done.\n\r");
}

extern "C" void carica_success(){
    boot_printf("Carica_stato done.\n\r");
}

extern "C" void test_stato_c(){
    init.id = 0xFFFF;
    init.precedenza = 0xFFFFFFFF;
	esecuzione = &init;
    test_stato_asm();
}
