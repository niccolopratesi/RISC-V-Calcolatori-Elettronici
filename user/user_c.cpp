#include "all.h"
/*
    Semplice progrmma di test:

    main crea 2 processi main_body e cede il controllo al primo di essi.
    i processi main_body si mettono in attesa per wait cicli di timer e poi terminano.

    (e' consigliato attivare la funzione timer_debug all'interno del file traps_c.cpp
    e modificare la frequenza dei cicli del timer all'interno del file traps_asm.s)
*/

void main_body(natq wait) {
    flog(LOG_DEBUG, "Mi metto in attesa di %d cicli di timer", wait);
    delay(wait);
    flog(LOG_DEBUG, "Attesa terminata");
    terminate_p();
}

extern "C" void main() {
    flog(LOG_DEBUG, ">>>INIZIO<<<");

    flog(LOG_DEBUG, "Creo il primo processo utente");
    activate_p(main_body, 5, MIN_PRIORITY+1, LIV_UTENTE);
    flog(LOG_DEBUG, "Creo il secondo processo utente");
    activate_p(main_body, 10, MIN_PRIORITY+1, LIV_UTENTE);

    flog(LOG_DEBUG, "Cedo il controllo al primo processo utente");
    terminate_p();
}

