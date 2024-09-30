#include "libce.h"
#include "costanti.h"
#include "uart.h"
#include "traps.h"
#include "plic.h"
#include "proc.h"
#include "semafori.h"
#include "timer.h"
#include "sys.h"
#include "vm.h"

void timer_debug() {
  flog(LOG_INFO, "Timer fired");
}

/// @brief Gestore delle interruzioni (flag in sscause = 1)
/// @return 1 se interruzione esterna, 2 se software, 3 se timer, 0 altrimenti
int dev_int() {
    natq scause = readSCAUSE();
    
    // Interruzione esterna
    if (scause == 0x8000000000000009L) {
        flog(LOG_INFO, "interruzione esterna");
        // PLIC
        int irq = plic_claim();

        //handler interruzione esterna
        if(a_p[irq]){
            inspronti();
            inserimento_lista(pronti,a_p[irq]);
            schedulatore();
        }
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

bool syscall_supervisor(void)
{
  des_proc *p = esecuzione;

  switch (p->contesto[I_A7]) {
  case TIPO_WFI:
    plic_complete(p->contesto[I_A0]);
    schedulatore();
    break;
  case TIPO_TRA :
    c_trasforma(p->contesto[I_A0]);
    break;
  case TIPO_ACC :
    c_access(p->contesto[I_A0], p->contesto[I_A1], p->contesto[I_A2], p->contesto[I_A3]);
    break;
   case TIPO_APE :
    c_activate_pe((void(*)(natq))p->contesto[I_A0], p->contesto[I_A1], p->contesto[I_A2], p->contesto[I_A3],p->contesto[I_A4]);
    break;
  case TIPO_AB :
    c_abort_p(p->contesto[I_A0]);
    break;
  default:
    return false;
  }

  return true;
}

/// Legge il numero della syscall in a7 e chiama la funzione corrispondente
void syscall_user(void) {
  des_proc *p = esecuzione;

  switch (p->contesto[I_A7]) {
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
    flog(LOG_WARN, "unknown sys call %d\n", p->contesto[I_A7]);
    p->contesto[I_A0] = -1;
    break;
  }
}

// Parte C++ del gestore delle interruzioni in modalità supervisor
extern "C" void supervisor_handler()
{
    natq epc = readSEPC();
    natq status = readSSTATUS();
    natq cause = readSCAUSE();

    if ((status & SSTATUS_SIE) != 0)
        fpanic("trap: interrupts enabled");

    if (cause == 9 && syscall_supervisor()) {
        esecuzione->epc += 4;
        return;
    }

    // ecall da u-mode (8) o s-mode (9)
    if (cause == 8 || cause == 9) {
        // Salviamo l'indirizzo dell'istruzione successiva, a cui si deve tornare
        esecuzione->epc += 4;
        syscall_user();
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
