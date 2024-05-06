#include "all.h"
/*
    Semplice progrmma di test:

    main crea un processo main_body che deve contare fino a tot.
    main_body inizializza il semaforo di sincronizzazione end_count,
    dopodiché crea due processi conta che, in due step, portano count a tot.
*/
natl mainb_proc, conta_proc;
int count = 0;
natl end_count;

void conta(natq quanti) {
    for (int i = 0; i < quanti; i++) {
        count++;
    }

    flog(LOG_DEBUG, "Fine conta: count = %d", count);
    sem_signal(end_count);
    terminate_p();
}

void main_body(natq tot) {
    natq quanti1 = tot/2;
    
    end_count = sem_ini(0);

    flog(LOG_DEBUG, "Creo il processo conta1");
    conta_proc = activate_p(conta, quanti1, MIN_PRIORITY, LIV_UTENTE);
    flog(LOG_DEBUG, "Aspetto che il processo conta1 conti fino a %d", quanti1);
    sem_wait(end_count);

    flog(LOG_DEBUG, "Creo il processo conta2");
    conta_proc = activate_p(conta, tot-quanti1, MIN_PRIORITY, LIV_UTENTE);
    flog(LOG_DEBUG, "Aspetto che il processo conta2 conti fino a %d", tot-quanti1);
    sem_wait(end_count);

    if (count == (int)tot) {
        flog(LOG_DEBUG, "Test completato con successo", count);
        flog(LOG_DEBUG, ">>>FINE<<<");
    }

    terminate_p();
}

extern "C" void main() {
    flog(LOG_DEBUG, ">>>INIZIO<<<");

    flog(LOG_DEBUG, "Creo il processo main_body utente");
    natl mainb;
    mainb = activate_p(main_body, 1000, MIN_PRIORITY+1, LIV_UTENTE);

    flog(LOG_DEBUG, "Cedo il controllo al processo main_body utente...");
    terminate_p();
}

