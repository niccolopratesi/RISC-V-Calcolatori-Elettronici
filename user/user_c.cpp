#include "all.h"
/*
    Semplice progrmma di test:

    main crea 2 processi main_body e cede il controllo al primo di essi.
    i processi main_body si mettono in attesa per wait cicli di timer e poi terminano.

    (e' consigliato attivare la funzione timer_debug all'interno del file traps_c.cpp
    e modificare la frequenza dei cicli del timer all'interno del file traps_asm.s)
*/

extern "C" void main() {
    char hw[] = "Hello World!";
    natq hw_len = strlen(hw);

    char buf[11];
    natq buf_len = 10;

    flog(LOG_DEBUG, ">>>INIZIO<<<");

    flog(LOG_DEBUG, "Inizializzo la console (iniconsole) bianco su nero (0x0f)");
    iniconsole(0x0f);

    flog(LOG_DEBUG, "Scrivo sulla console (writeconsole): Hello World!");
    writeconsole(hw, hw_len);

    flog(LOG_DEBUG, "Leggo dalla console (readconsole) 10 caratteri");
    natq letti = readconsole(buf, buf_len);
    flog(LOG_DEBUG, "Caratteri effettivamente letti: %d", letti);

    flog(LOG_DEBUG, "Cedo il controllo al primo processo utente");
    terminate_p();
}

