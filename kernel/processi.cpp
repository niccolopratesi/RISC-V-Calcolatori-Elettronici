#include "tipo.h"
#include "costanti.h"

/// Priorità del processo dummy
const natl DUMMY_PRIORITY = 0;
/// Numero di registri nel campo contesto del descrittore di processo
const int N_REG = 16;

/// @brief Descrittore di processo
struct des_proc {
	/// identificatore numerico del processo
	natw id;
	/// livello di privilegio (LIV_UTENTE o LIV_SISTEMA)
	natw livello;
	/// precedenza nelle code dei processi
	natl precedenza;
	/// indirizzo della base della pila sistema
	vaddr punt_nucleo;
	/// copia dei registri generali del processore
	natq contesto[N_REG];
	/// radice del TRIE del processo
	paddr cr3;

	/// prossimo processo in coda
	des_proc* puntatore;

	/// @name Informazioni utili per il per debugging
	/// @{

	/// parametro `f` passato alla `activate_p`/`_pe` che ha creato questo processo
	void (*corpo)(natq);
	/// parametro `a` passato alla `activate_p`/`_pe` che ha creato questo processo
	natq  parametro;
	/// @}
};

/// @brief Tabella che associa l'id di un processo al corrispondente des_proc.
///
/// I des_proc sono allocati dinamicamente nello heap del sistema (si veda
/// crea_processo).
des_proc* proc_table[MAX_PROC];

/// Numero di processi utente attivi.
natl processi;

/// @cond
// (forward) Distrugge il processo puntato da esecuzione.
extern "C" void c_abort_p(bool selfdump = true);

// (forward) Ferma tutto il sistema (in caso di bug nel sistema stesso)
extern "C" [[noreturn]] void panic(const char* msg);
/// @endcond

/// @brief Indici delle copie dei registri nell'array contesto
enum { I_RAX, I_RCX, I_RDX, I_RBX,
	I_RSP, I_RBP, I_RSI, I_RDI, I_R8, I_R9, I_R10,
	I_R11, I_R12, I_R13, I_R14, I_R15 };

/// Coda esecuzione (contiene sempre un solo elemento)
des_proc* esecuzione;

/// Coda pronti (vuota solo quando dummy è in @ref esecuzione)
des_proc* pronti;

/*! @brief Inserimento in lista ordinato (per priorità)
 *  @param p_lista	lista in cui inserire
 *  @param p_elem	elemento da inserire
 *  @note a parità di priorità favorisce i processi già in coda
 */
void inserimento_lista(des_proc*& p_lista, des_proc* p_elem)
{
// inserimento in una lista semplice ordinata
//   (tecnica dei due puntatori)
	des_proc *pp, *prevp;

	pp = p_lista;
	prevp = nullptr;
	while (pp && pp->precedenza >= p_elem->precedenza) {
		prevp = pp;
		pp = pp->puntatore;
	}

	p_elem->puntatore = pp;

	if (prevp)
		prevp->puntatore = p_elem;
	else
		p_lista = p_elem;

}

/*! @brief Estrazione del processo a maggiore priorità
 *  @param  p_lista lista da cui estrarre
 *  @return processo a più alta priorità
 */
des_proc* rimozione_lista(des_proc*& p_lista)
{
// estrazione dalla testa
	des_proc* p_elem = p_lista;  	// nullptr se la lista è vuota

	if (p_lista)
		p_lista = p_lista->puntatore;

	if (p_elem)
		p_elem->puntatore = nullptr;

	return p_elem;
}

/// @brief Inserisce @ref esecuzione in testa alla lista @ref pronti
extern "C" void inspronti()
{
	esecuzione->puntatore = pronti;
	pronti = esecuzione;
}

/*! @brief Sceglie il prossimo processo da mettere in esecuzione
 *  @note Modifica solo la variabile @ref esecuzione.
 *  Il processo andrà effettivamente in esecuzione solo alla prossima
 *  `call carica_stato; iretq`
 */
extern "C" void schedulatore(void)
{
// poiché la lista è già ordinata in base alla priorità,
// è sufficiente estrarre l'elemento in testa
	esecuzione = rimozione_lista(pronti);
}

/*! @brief Trova il descrittore di processo dato l'id.
 *
 *  Errore fatale se _id_ non corrisponde ad alcun processo.
 *
 *  @param id 	id del processo
 *  @return	descrittore di processo corrispondente
 */
extern "C" des_proc* des_p(natw id)
{
	if (id > MAX_PROC_ID)
		fpanic("id %hu non valido (max %lu)", id, MAX_PROC_ID);

	return proc_table[id];
}

/// @name Funzioni usate dal processo dummy
/// @{

/// @brief Esegue lo shutdown del sistema.
extern "C" [[noreturn]] void end_program();

/*! @brief Esegue l'istruzione `hlt`.
 *
 *  Mette in pausa il processore in attesa di una richiesta di interruzione
 *  esterna
 */
extern "C" void halt();
/// @}

/// @brief Corpo del processo dummy
void dummy(natq)
{
	while (processi)
		halt();
	flog(LOG_INFO, "Shutdown");
	end_program();
}


/// @name Funzioni di supporto alla creazione e distruzione dei processi
/// @{

/*! @brief Alloca un id di processo
 *  @param p	descrittore del processo a cui assegnare l'id
 *  @return	id del processo (0xFFFFFFFF se terminati)
 */
natl alloca_proc_id(des_proc* p)
{
	static natl next = 0;

	// La funzione inizia la ricerca partendo  dall'id successivo
	// all'ultimo restituito (salvato nella variable statica 'next'),
	// saltando quelli che risultano in uso.
	// In questo modo gli id tendono ad essere riutilizzati il più tardi possibile,
	// cosa che aiuta chi deve debuggare i propri programmi multiprocesso.
	natl scan = next, found = 0xFFFFFFFF;
	do {
		if (proc_table[scan] == nullptr) {
			found = scan;
			proc_table[found] = p;
		}
		scan = (scan + 1) % MAX_PROC;
	} while (found == 0xFFFFFFFF && scan != next);
	next = scan;
	return found;
}

/*! @brief Rilascia un id di processo non più utilizzato
 *  @param id	id da rilasciare
 */
void rilascia_proc_id(natw id)
{
	if (id > MAX_PROC_ID)
		fpanic("id %hu non valido (max %lu)", id, MAX_PROC_ID);

	if (proc_table[id] == nullptr)
		fpanic("tentativo di rilasciare id %d non allocato", id);

	proc_table[id] = nullptr;
}


/*! @brief Inizializza la tabella radice di un nuovo processo
 *  @param dest indirizzo fisico della tabella
 */
void init_root_tab(paddr dest)
{
	paddr pdir = esecuzione->cr3;

	// ci limitiamo a copiare dalla tabella radice corrente i puntatori
	// alle tabelle di livello inferiore per tutte le parti condivise
	// (sistema, utente e I/O). Quindi tutti i sottoalberi di traduzione
	// delle parti condivise saranno anch'essi condivisi. Questo, oltre a
	// semplificare l'inizializzazione di un processo, ci permette di
	// risparmiare un po' di memoria.
	copy_des(pdir, dest, I_SIS_C, N_SIS_C);
	copy_des(pdir, dest, I_MIO_C, N_MIO_C);
	copy_des(pdir, dest, I_UTN_C, N_UTN_C);
}

/*! @brief Ripulisce la tabella radice di un processo
 *  @param dest indirizzo fisico della tabella
 */
void clear_root_tab(paddr dest)
{
	// eliminiamo le entrate create da init_root_tab()
	set_des(dest, I_SIS_C, N_SIS_C, 0);
	set_des(dest, I_MIO_C, N_MIO_C, 0);
	set_des(dest, I_UTN_C, N_UTN_C, 0);
}

/*! @brief Crea una pila processo
 *
 *  @param root_tab	indirizzo fisico della radice del TRIE del processo
 *  @param bottom	indirizzo virtuale del bottom della pila
 *  @param size		dimensione della pila (in byte)
 *  @param liv		livello della pila (LIV_UTENTE o LIV_SISTEMA)
 *  @return		true se la creazione ha avuto successo, false altrimenti
 */
bool crea_pila(paddr root_tab, vaddr bottom, natq size, natl liv)
{
	vaddr v = map(root_tab,
		bottom - size,
		bottom,
		BIT_RW | (liv == LIV_UTENTE ? BIT_US : 0),
		[](vaddr) { return alloca_frame(); });
	if (v != bottom) {
		unmap(root_tab, bottom - size, v,
			[](vaddr, paddr p, int) { rilascia_frame(p); });
		return false;
	}
	return true;
}

/*! @brief Distrugge una pila processo
 *
 *  Funziona indifferentemente per pile utente o sistema.
 *
 *  @param root_tab	indirizzo fisico della radice del TRIE del processo
 *  @param bottom	indirizzo virtuale del bottom della pila
 *  @param size		dimensione della pila (in byte)
 */
void distruggi_pila(paddr root_tab, vaddr bottom, natq size)
{
	unmap(
		root_tab,
		bottom - size,
		bottom,
		[](vaddr, paddr p, int) { rilascia_frame(p); });
}

/*! @brief Funzione interna per la creazione di un processo.
 *
 *  Parte comune a activate_p() e activate_pe().  Alloca un id per il processo
 *  e crea e inizializza il descrittore di processo, la pila sistema e, per i
 *  processi di livello utente, la pila utente. Crea l'albero di traduzione
 *  completo per la memoria virtuale del processo.
 *
 *  @param f	corpo del processo
 *  @param a	parametro per il corpo del processo
 *  @param prio	priorità del processo
 *  @param liv	livello del processo (LIV_UTENTE o LIV_SISTEMA)
 *  @return	puntatore al nuovo descrittore di processo
 *  		(nullptr in caso di errore)
 */
des_proc* crea_processo(void f(natq), natq a, int prio, char liv)
{
	des_proc*	p;			// des_proc per il nuovo processo
	paddr		pila_sistema;		// pila_sistema del processo
	natq*		pl;			// pila sistema come array di natq
	natl		id;			// id del nuovo processo

	// allocazione (e azzeramento preventivo) di un des_proc
	p = new des_proc;
	if (!p)
		goto err_out;
	memset(p, 0, sizeof(des_proc));

	// rimpiamo i campi di cui conosciamo già i valori
	p->precedenza = prio;
	p->puntatore = nullptr;
	// il registro RDI deve contenere il parametro da passare alla
	// funzione f
	p->contesto[I_RDI] = a;

	// selezione di un identificatore
	id = alloca_proc_id(p);
	if (id == 0xFFFFFFFF)
		goto err_del_p;
	p->id = id;

	// creazione della tabella radice del processo
	p->cr3 = alloca_tab();
	if (p->cr3 == 0)
		goto err_rel_id;
	init_root_tab(p->cr3);

	// creazione della pila sistema
	if (!crea_pila(p->cr3, fin_sis_p, DIM_SYS_STACK, LIV_SISTEMA))
		goto err_rel_tab;
	// otteniamo un puntatore al fondo della pila appena creata.  Si noti
	// che non possiamo accedervi tramite l'indirizzo virtuale 'fin_sis_p',
	// che verrebbe tradotto seguendo l'albero del processo corrente, e non
	// di quello che stiamo creando.  Per questo motivo usiamo trasforma()
	// per ottenere il corrispondente indirizzo fisico. In questo modo
	// accediamo alla nuova pila tramite la finestra FM.
	pila_sistema = trasforma(p->cr3, fin_sis_p - DIM_PAGINA) + DIM_PAGINA;

	// convertiamo a puntatore a natq, per accedervi più comodamente
	pl = ptr_cast<natq>(pila_sistema);

	if (liv == LIV_UTENTE) {
		// inizializziamo la pila sistema.
		pl[-5] = int_cast<natq>(f);	    // RIP (codice utente)
		pl[-4] = SEL_CODICE_UTENTE;	    // CS (codice utente)
		pl[-3] = BIT_IF;	    	    // RFLAGS
		pl[-2] = fin_utn_p - sizeof(natq);  // RSP
		pl[-1] = SEL_DATI_UTENTE;	    // SS (pila utente)
		// eseguendo una IRET da questa situazione, il processo
		// passerà ad eseguire la prima istruzione della funzione f,
		// usando come pila la pila utente (al suo indirizzo virtuale)

		// creazione della pila utente
		if (!crea_pila(p->cr3, fin_utn_p, DIM_USR_STACK, LIV_UTENTE)) {
			flog(LOG_WARN, "creazione pila utente fallita");
			goto err_del_sstack;
		}

		// inizialmente, il processo si trova a livello sistema, come
		// se avesse eseguito una istruzione INT, con la pila sistema
		// che contiene le 5 parole lunghe preparate precedentemente
		p->contesto[I_RSP] = fin_sis_p - 5 * sizeof(natq);

		p->livello = LIV_UTENTE;

		// dal momento che usiamo traduzioni diverse per le parti sistema/private
		// di tutti i processi, possiamo inizializzare p->punt_nucleo con un
		// indirizzo (virtuale) uguale per tutti i processi
		p->punt_nucleo = fin_sis_p;

		//   tutti gli altri campi valgono 0
	} else {
		// processo di livello sistema
		// inizializzazione della pila sistema
		pl[-6] = int_cast<natq>(f);		// RIP (codice sistema)
		pl[-5] = SEL_CODICE_SISTEMA;            // CS (codice sistema)
		pl[-4] = BIT_IF;  	        	// RFLAGS
		pl[-3] = fin_sis_p - sizeof(natq);      // RSP
		pl[-2] = 0;			        // SS
		pl[-1] = 0;			        // ind. rit.
							//(non significativo)
		// i processi esterni lavorano esclusivamente a livello
		// sistema. Per questo motivo, prepariamo una sola pila (la
		// pila sistema)

		// inizializziamo il descrittore di processo
		p->contesto[I_RSP] = fin_sis_p - 6 * sizeof(natq);

		p->livello = LIV_SISTEMA;

		// tutti gli altri campi valgono 0
	}

	// informazioni di debug
	p->corpo = f;
	p->parametro = a;

	return p;

err_del_sstack:	distruggi_pila(p->cr3, fin_sis_p, DIM_SYS_STACK);
err_rel_tab:	clear_root_tab(p->cr3);
		rilascia_tab(p->cr3);
err_rel_id:	rilascia_proc_id(p->id);
err_del_p:	delete p;
err_out:	return nullptr;
}

/// @cond
// usate da distruggi processo e definite più avanti
extern paddr ultimo_terminato;
extern des_proc* esecuzione_precedente;
extern "C" void distruggi_pila_precedente();
/// @endcond

/*! @brief Dealloca tutte le risorse allocate da crea_processo()
 *
 *  Dealloca l'id, il descrittore di processo, l'eventuale pila utente,
 *  comprese le tabelle che la mappavano nella memoria virtuale del processo.
 *  Per la pila sistema si veda sopra @ref esecuzione_precedente.
 */
void distruggi_processo(des_proc* p)
{
	paddr root_tab = p->cr3;
	// la pila utente può essere distrutta subito, se presente
	if (p->livello == LIV_UTENTE)
		distruggi_pila(root_tab, fin_utn_p, DIM_USR_STACK);
	// se p == esecuzione_precedente rimandiamo la distruzione alla
	// carica_stato, altrimenti distruggiamo subito
	ultimo_terminato = root_tab;
	if (p != esecuzione_precedente) {
		// riporta anche ultimo_terminato a zero
		distruggi_pila_precedente();
	}
	rilascia_proc_id(p->id);
	delete p;
}

/*! @brief Carica un handler nella IDT.
 *
 *  Fa in modo che l'handler _irq_ -esimo sia associato alle richieste
 *  di interruzione provenienti dal piedino _irq_ dell'APIC. L'assozione
 *  avviene tramite l'entrata _tipo_ della IDT.
 *
 *  @param tipo 	tipo dell'interruzione a cui associare l'handler
 *  @param irq		piedino dell'APIC associato alla richiesta di interruzione
 */
extern "C" bool load_handler(natq tipo, natq irq);
/// @}

/// @name Distruzione della pila sistema corrente
///
/// Quando dobbiamo eliminare una pila sistema dobbiamo stare attenti a non
/// eliminare proprio quella che stiamo usando.  Questo succede durante una
/// terminate_p() o abort_p(), quando si tenta di distrugguere proprio il
/// processo che ha invocato la primitiva.
///
/// Per fortuna, se stiamo terminando il processo corrente, vuol dire anche che
/// stiamo per metterne in esecuzione un altro e possiamo dunque usare la pila
/// sistema di quest'ultimo. Operiamo dunque nel seguente modo:
///
/// - all'ingresso nel sistema (in salva_stato), salviamo il valore di @ref esecuzione
///   in @ref esecuzione_precedente; questo è il processo a cui appartiene la
///   pila sistema che stiamo usando;
/// - in @ref distruggi_processo(), se @ref esecuzione è uguale a @ref esecuzione_precedente
///   (stiamo distruggendo proprio il processo a cui appartiene la pila corrente),
///   _non_ distruggiamo la pila sistema e settiamo la variabile @ref ultimo_terminato;
/// - in carica_stato, dopo aver cambiato pila, se @ref ultimo_terminato è settato,
///   distruggiamo la pila sistema di @ref esecuzione_precedente.
///
/// @{

/// @brief Processo che era in esecuzione all'entrata nel modulo sistema
///
/// La salva_stato ricorda quale era il processo in esecuzione al momento
/// dell'entrata nel sistema e lo scrive in questa variabile.
des_proc* esecuzione_precedente;

/// @brief Se diverso da zero, indirizzo fisico della root_tab dell'ultimo processo
///        terminato o abortito.
///
/// La carica_stato legge questo indirizzo per sapere se deve distruggere la
/// pila del processo uscente, dopo aver effettuato il passaggio alla pila del
/// processo entrante.
paddr ultimo_terminato;

/*! @brief Distrugge la pila sistema del processo uscente e rilascia la sua tabella radice.
 *
 *  Chiamata da distruggi_processo() oppure da carica_stato se
 *  distruggi_processo() aveva rimandato la distruzione della pila.  Dealloca la
 *  pila sistema e le traduzioni corrispondenti nel TRIE di radice
 *  @ref ultimo_terminato. La distruggi_processo() aveva già eliminato tutte le
 *  altre traduzioni, quindi la funzione può anche deallocare la radice del
 *  TRIE.
 */
extern "C" void distruggi_pila_precedente() {
	distruggi_pila(ultimo_terminato, fin_sis_p, DIM_SYS_STACK);
	// ripuliamo la tabella radice (azione inversa di init_root_tab())
	// in modo da azzerare il contatore delle entrate valide e passare
	// il controllo in rilascia_tab()
	clear_root_tab(ultimo_terminato);
	rilascia_tab(ultimo_terminato);
	ultimo_terminato = 0;
}
/// @}

/// @name Primitive per la creazione e distruzione dei processi (parte C++)
/// @{

/*! @brief Parte C++ della primitiva activate_p()
 *  @param f	corpo del processo
 *  @param a	parametro per il corpo del processo
 *  @param prio	priorità del processo
 *  @param liv	livello del processo (LIV_UTENTE o LIV_SISTEMA)
 */
extern "C" void c_activate_p(void f(natq), natq a, natl prio, natl liv)
{
	des_proc* p;			// des_proc per il nuovo processo
	natl id = 0xFFFFFFFF;		// id da restituire in caso di fallimento

	// non possiamo accettare una priorità minore di quella di dummy
	// o maggiore di quella del processo chiamante
	if (prio < MIN_PRIORITY || prio > esecuzione->precedenza) {
		flog(LOG_WARN, "priorita' non valida: %u", prio);
		c_abort_p();
		return;
	}

	// controlliamo che 'liv' contenga un valore ammesso
	// [segnalazione di E. D'Urso]
	if (liv != LIV_UTENTE && liv != LIV_SISTEMA) {
		flog(LOG_WARN, "livello non valido: %u", liv);
		c_abort_p();
		return;
	}

	// non possiamo creare un processo di livello sistema mentre
	// siamo a livello utente
	if (liv == LIV_SISTEMA && liv_chiamante() == LIV_UTENTE) {
		flog(LOG_WARN, "errore di protezione");
		c_abort_p();
		return;
	}

	// accorpiamo le parti comuni tra c_activate_p e c_activate_pe
	// nella funzione ausiliare crea_processo
	p = crea_processo(f, a, prio, liv);

	if (p != nullptr) {
		inserimento_lista(pronti, p);
		processi++;
		id = p->id;			// id del processo creato
						// (allocato da crea_processo)
		flog(LOG_INFO, "proc=%u entry=%p(%lu) prio=%u liv=%u", id, f, a, prio, liv);
	}

	esecuzione->contesto[I_RAX] = id;
}

/// @brief Parte C++ della pritimiva terminate_p()
/// @param logmsg	set true invia un messaggio sul log
extern "C" void c_terminate_p(bool logmsg)
{
	des_proc* p = esecuzione;

	if (logmsg)
		flog(LOG_INFO, "Processo %u terminato", p->id);
	distruggi_processo(p);
	processi--;
	schedulatore();
}

/// @cond
/// funzioni usate da c_abort_p() per mostrare il backtrace del processo abortito.
/// (definite in sistema.s)
extern "C" void setup_self_dump();
extern "C" void cleanup_self_dump();
/// @endcond

/// @brief Parte C++ della primitiva abort_p()
///
/// Fuziona come la terminate_p(), ma invia anche un warning al log.  La
/// funzione va invocata quando si vuole terminare un processo segnalando che
/// c'è stato un errore.
///
/// @param selfdump	se true mostra sul log lo stato del processo
extern "C" void c_abort_p(bool selfdump)
{
	des_proc* p = esecuzione;

	if (selfdump) {
		setup_self_dump();
		process_dump(esecuzione, LOG_WARN);
		cleanup_self_dump();
	}
	flog(LOG_WARN, "Processo %u abortito", p->id);
	c_terminate_p(/* logmsg= */ false);
}

/*! @brief Parte C++ della primitiva activate_pe().
 *  @param f	corpo del processo
 *  @param a	parametro per il corpo del processo
 *  @param prio	priorità del processo
 *  @param liv	livello del processo (LIV_UTENTE o LIV_SISTEMA)
 *  @param irq	IRQ gestito dal processo
 */
extern "C" void c_activate_pe(void f(natq), natq a, natl prio, natl liv, natb irq)
{
	des_proc*	p;			// des_proc per il nuovo processo
	natw		tipo;			// entrata nella IDT

	esecuzione->contesto[I_RAX] = 0xFFFFFFFF;

	if (prio < MIN_EXT_PRIO || prio > MAX_EXT_PRIO) {
		flog(LOG_WARN, "priorita' non valida: %u", prio);
		return;
	}
	// controlliamo che 'liv' contenga un valore ammesso
	// [segnalazione di F. De Lucchini]
	if (liv != LIV_UTENTE && liv != LIV_SISTEMA) {
		flog(LOG_WARN, "livello non valido: %u", liv);
		return;
	}
	// controlliamo che 'irq' sia valido prima di usarlo per indicizzare
	// 'a_p'
	if (irq >= apic::MAX_IRQ) {
		flog(LOG_WARN, "irq %hhu non valido (max %u)", irq, apic::MAX_IRQ);
		return;
	}
	// se a_p è non-nullo, l'irq è già gestito da un altro processo
	// esterno o da un driver
	if (a_p[irq]) {
		flog(LOG_WARN, "irq %hhu occupato", irq);
		return;
	}
	// anche il tipo non deve essere già usato da qualcos'altro.
	// Controlliamo quindi che il gate corrispondente non sia marcato
	// come presente (bit P==1)
	tipo = prio - MIN_EXT_PRIO;
	if (gate_present(tipo)) {
		flog(LOG_WARN, "tipo %hx occupato", tipo);
		return;
	}

	p = crea_processo(f, a, prio, liv);
	if (p == 0)
		return;

	// creiamo il collegamento irq->tipo->handler->processo esterno
	// irq->tipo (tramite l'APIC)
	apic::set_VECT(irq, tipo);
	// associazione tipo->handler (tramite la IDT)
	// Nota: in sistema.s abbiamo creato un handler diverso per ogni
	// possibile irq. L'irq che passiamo a load_handler serve ad
	// identificare l'handler che ci serve.
	load_handler(tipo, irq);
	// associazione handler->processo esterno (tramite 'a_p')
	a_p[irq] = p;

	// ora che tutti i collegamenti sono stati creati possiamo
	// iniziare a ricevere interruzioni da irq. Smascheriamo
	// dunque le richieste irq nell'APIC
	apic::set_MIRQ(irq, false);

	flog(LOG_INFO, "estern=%u entry=%p(%lu) prio=%u (tipo=%2x) liv=%u irq=%hhu",
			p->id, f, a, prio, tipo, liv, irq);

	esecuzione->contesto[I_RAX] = p->id;
	return;
}