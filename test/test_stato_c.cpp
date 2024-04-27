#include "libce.h"
#include "proc.h"
#include "tipo.h"

extern des_proc *esecuzione;
extern des_proc init;
//struct des_proc *esecuzione_precedente;

extern "C" void test_stato_asm();
extern "C" void boot_printf(char *fmt, ...);

extern "C" void salva_success(){
    flog(LOG_INFO, "Salva_stato done.");
}

extern "C" void carica_success(){
    flog(LOG_INFO, "Carica_stato done.");
}

extern "C" int readSATP();

extern "C" void test_stato_c(){
    init.id = 0xFFFF;
    init.precedenza = 0xFFFFFFFF;
	init.satp = readSATP();
	esecuzione = &init;
    test_stato_asm();
}
