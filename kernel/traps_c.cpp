#include "libce.h"
#include "costanti.h"
#include "uart.h"
#include "traps.h"
#include "plic.h"
#include "proc.h"
#include "semafori.h"
#include "timer.h"
#include "sys.h"

void timer_debug() {
  flog(LOG_INFO, "Timer fired");
}

/// @brief Gestore delle interruzioni (flag in sscause = 1)
/// @return 1 se interruzione esterna, 2 se software, 3 se timer, 0 altrimenti
int dev_int() {
    natq scause = readSCAUSE();
    
    // Interruzione esterna
    if (scause == 0x8000000000000009L) {
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

    // Interruzione software
    if (scause == 0x8000000000000001L) {
        clearSSIP();

        return 2;
    }

    // Interruzione timer
    if (scause == 0x8000000000000005L) {
        // TEST: timer interrupts
        // timer_debug();

        schedule_next_timer_interrupt();
        c_driver_td();

        return 3;
    }

    return 0;
}

/// Legge il numero della syscall in a7 e chiama la funzione corrispondente
void syscall(void) {
  natq num;
  des_proc *p = esecuzione;

  num = p->contesto[I_A7];

  switch (num)
  {
    case TIPO_A:
        c_activate_p((void(*)(natq))p->contesto[I_A0], p->contesto[I_A1], p->contesto[I_A2], p->contesto[I_A3]);
        break;
    case TIPO_T:
        c_terminate_p();
        break;
    case TIPO_SI:
        c_sem_ini(p->contesto[I_A0]);
        break;
    case TIPO_W:
        c_sem_wait(p->contesto[I_A0]);
        break;
    case TIPO_S:
        c_sem_signal(p->contesto[I_A0]);
        break;
    case TIPO_D:
        c_delay(p->contesto[I_A0]);
        break;
    case TIPO_L:
        c_do_log((log_sev)p->contesto[I_A0], (const char*)p->contesto[I_A1], p->contesto[I_A2]);
        break;
    // case TIPO_GMI:
    //     c_getmeminfo();
    //     break;
    default:    
        flog(LOG_WARN, "unknown sys call %d\n", num);
        p->contesto[I_A0] = -1;
        break;
  }
}

// Parte C++ del gestore delle interruzioni in modalità supervisor
extern "C" void kInterruptHandler(){
    natq epc = readSEPC();
    natq status = readSSTATUS();
    natq cause = readSCAUSE();

    if ((status & SSTATUS_SIE) != 0)
        fpanic("trap: interrupts enabled");

    // ecall da u-mode (8) o s-mode (9)
    if (cause == 8 || cause == 9) {
        // Salviamo l'indirizzo dell'istruzione successiva, a cui si deve tornare
        esecuzione->epc += 4;
        syscall();
        return;
    }

    int dev = dev_int();
    if (dev == 0) {
        // Eccezione
        flog(LOG_WARN, "unexpected scause=%p, id=%p", readSCAUSE(), esecuzione->id);
        flog(LOG_WARN, "sepc=%p, stval=%p", readSEPC(), readSTVAL());
        // Terminiamo e distruggiamo il processo
        c_terminate_p();
    }
}
