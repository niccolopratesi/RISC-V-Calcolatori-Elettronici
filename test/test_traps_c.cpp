#include "libce.h"
#include "uart.h"
#include "def.h"
#include "plic.h"
#include "proc.h"

extern des_proc *esecuzione;
extern des_proc init;
extern des_proc *esecuzione_precedente;

extern "C" void s_trap();
extern "C" void k_trap();
extern "C" void writeSTVEC(void*);
extern "C" void writeSEPC(void*);
extern "C" natq readSEPC();
extern "C" int readSCAUSE();
extern "C" int readSTVAL();
extern "C" int readSTVEC();
extern "C" int readSSTATUS();
extern "C" void disableSInterrupts();
extern "C" void enableSInterrupts();
extern "C" void sInterruptReturn();
extern "C" void clearSPreviousPrivilege();
extern "C" void clearSSIP();
extern "C" int readSSIP();
extern "C" int readSATP();
extern "C" void setSPreviousInterruptEnable();

void timer_debug(){
  flog(LOG_INFO, "Timer fired");
}

int dev_int() {
    
    if (readSCAUSE() == 0x8000000000000009L) {
        // PLIC
        int irq = plic_claim();

        if (irq == UART0_IRQ) 
            uart_intr();
        else 
            flog(LOG_WARN, "Unexpected interrupt: %d", irq);

        if (irq)
            plic_complete(irq);

        return 1;
    }

    if (readSCAUSE() == 0x8000000000000001L) {
        // software int
        clearSSIP();

        return 2;
    }

    else
        return 0;
}

extern "C" void sInterruptHandler(){

    if ((readSSTATUS() & SSTATUS_SPP) != 0) 
        fpanic("usertrap: not from user mode");

    //Imposta il trap vector all'handler 
    //per il kernel
    writeSTVEC((void*)k_trap);

    //Salva l'indirizzo a cui si deve tornare
    //al termine della gestione dell'int
    esecuzione->epc = readSEPC();

    if (readSCAUSE() == 8) {
        // syscall
        esecuzione->epc += 4;
        enableSInterrupts();

        // system_call();
    }
    else if (dev_int() != 0) {
        // OK
    }
    else {
        flog(LOG_WARN, "unexpected scause=%p, pid=%p", readSCAUSE(), esecuzione->id);
        flog(LOG_WARN, "sepc=%p, stval=%p", readSEPC(), readSTVAL());
        // TODO: distruggi il processo
    }

    //Potremmo venire da codice che aveva precedentemente eseguito
    //in modalita' supervisor con le interruzioni abilitate.
    //Prima di fare qualsiasi cosa disattiviamole.
    disableSInterrupts();

    //Ripristiniamo l'interrupt handler da usare al di fuori del kernel
    writeSTVEC((void*)s_trap);
}

// extern "C" void sInterruptReturn(){
//     // Prepariamo i valori di cui uservec avrà bisogno alla
//     // prossima trap nel kernel
//     //esecuzione->trapframe->kernel_satp = readSATP();
//     //esecuzione->trapframe->kernel_trap = (natq)sInterruptHandler;

//     //Indichiamo il livello utente come privilegio a cui tornare
//     //clearSPreviousPrivilege();

//     //Abilitiamo interruzioni per l'utente al ritorno
//     //setSPreviousInterruptEnable();

//     //writeSEPC((void*)esecuzione->trapframe->epc);

//     // Informiamo trampoline.s su quale tabella delle pagine utente usare
//     // natq user_page = writeSATP(esecuzione->satp);
// }

extern "C" void kInterruptHandler(){
    int epc = readSEPC();
    int status = readSSTATUS();
    int cause = readSCAUSE();

    if ((status & SSTATUS_SPP) == 0) 
        fpanic("kerneltrap: not from supervisor mode");
    if ((status & SSTATUS_SIE) != 0)
        fpanic("kerneltrap: interrupts enabled");
    
    if (dev_int() == 0) {
        // Eccezione
        flog(LOG_WARN, "scause=%p", readSCAUSE());
        flog(LOG_WARN, "sepc=%p, stval=%p", readSEPC(), readSTVAL());
        fpanic("kerneltrap");
    }

    // TEST: timer interrupts
    // if (dev_int() == 2) 
    //   timer_debug();      

        
}