#include "vm.h"
#include "tipo.h"
#include "costanti.h"
#include "libce.h"
#include "uart.h"

extern "C" void test_paginazione_c(){
    // iizializziamo la parte M2
	init_frame();

	// creiamo le parti condivise della memoria virtuale di tutti i processi
	// le parti sis/priv e usr/priv verranno create da crea_processo()
	// ogni volta che si attiva un nuovo processo
	paddr root_tab = alloca_tab();
	if (!root_tab)
		flog(LOG_WARN, "Errore allocazione tabella root");
	// finestra di memoria, che corrisponde alla parte sis/cond
	if(!crea_finestra_FM(root_tab))
		flog(LOG_WARN, "Errore creazione finestra FM\n");

	// Attivazione paginazione
	writeSATP(root_tab);

	// Successo!
	printf("\nPaging enabled. Hi from Virtual Memory!\n\r");

	flog(LOG_INFO, "Hello from flog");
} 

