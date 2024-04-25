#include "vm.h"
#include "tipo.h"
#include "costanti.h"
#include "libce.h"
#include "uart.h"

extern "C" void print_VGA(char *message, uint8 fg, uint8 bg);

extern "C" natq end;	// ultimo indirizzo del codice sistema (fornito dal collegatore)

extern "C" natq __user_start; // primo indirizzo del codice utente (fornito dal collegatore)
extern "C" natq __user_etext; // primo indirizzo dei dati utente (fornito dal collegatore)
extern "C" natq __user_end; // ultimo indirizzo dei dati utente (fornito dal collegatore)

void creaRootTab(){
    paddr root_tab = (paddr)end;
	//Align the end address to the pagesize
	root_tab = root_tab + (DIM_PAGINA - (root_tab % DIM_PAGINA));
	paddr tab = root_tab;
	for (tab_iter it(root_tab, 0, 4 * GiB); it; it.next())
	{
		tab_entry &e = it.get_e();

		if (it.get_l() > 2)
		{
			memset((void *)tab, 0, DIM_PAGINA);
			set_IND_FISICO(e, tab);
			e |= BIT_V;
			tab += DIM_PAGINA;
		}
		else
		{
			vaddr v = it.get_v();
			set_IND_FISICO(e, v);
			e |= BIT_V | BIT_R | BIT_W | BIT_X;
		}
	}

	writeSATP(root_tab);
}

////////////////////////////////////////////////////////////////////////////////
//                         FRAME					       //
/////////////////////////////////////////////////////////////////////////////////

struct des_frame {
	union {
		// numero di entrate valide (se il frame contiene una tabella)
		natw nvalide;
		// lista di frame liberi (se il frame è libero)
		natl prossimo_libero;
	};
};

// numero totale di frame (M1 + M2)
natq const N_FRAME = MEM_TOT / DIM_PAGINA;
// numero totale di frame in M1 e in M2
natq N_M1;
natq N_M2;

// array dei descrittori di frame
des_frame vdf[N_FRAME];
// testa della lista dei frame liberi
natq primo_frame_libero;
// numero di frame nella lista dei frame liberi
natq num_frame_liberi;

// init_des_frame viene chiamata in fase di inizializzazione.  Tutta la memoria
// non ancora occupata viene usata per i frame.  La funzione si preoccupa anche
// di inizializzare i descrittori dei frame in modo da creare la lista dei
// frame liberi.
// &end è l'indirizzo del primo byte non occupato dal modulo sistema (è
// calcolato dal collegatore). La parte M2 della memoria fisica inizia al
// primo frame dopo &end.
void init_frame()
{
	// primo frame di M2
	paddr fine_M1 = allinea(reinterpret_cast<paddr>(&end), DIM_PAGINA);
	// numero di frame in M1 e indice di f in vdf
	N_M1 = (fine_M1-0x80000000) / DIM_PAGINA;
	// numero di frame in M2
	N_M2 = N_FRAME - N_M1;

	if (!N_M2)
		return;

	// creiamo la lista dei frame liberi, che inizialmente contiene tutti i
	// frame di M2
	primo_frame_libero = N_M1;
#ifndef N_STEP
	// alcuni esercizi definiscono N_STEP == 2 per creare mapping non
	// contigui in memoria virtale e controllare meglio alcuni possibili
	// bug
#define N_STEP 1
#endif
	natq last;
	for (natq j = 0; j < N_STEP; j++) {
		for (natq i = j; i < N_M2; i += N_STEP) {
			vdf[primo_frame_libero + i].prossimo_libero =
				primo_frame_libero + i + N_STEP;
			num_frame_liberi++;
			last = i;
		}
		vdf[primo_frame_libero + last].prossimo_libero =
			primo_frame_libero + j + 1;
	}
	vdf[primo_frame_libero + last].prossimo_libero = 0;
}

// estrae un frame libero dalla lista, se non vuota
paddr alloca_frame()
{
	if (!num_frame_liberi) {
		flog(LOG_ERR, "out of memory");
		return 0;
	}
	natq j = primo_frame_libero;
	primo_frame_libero = vdf[primo_frame_libero].prossimo_libero;
	vdf[j].prossimo_libero = 0;
	num_frame_liberi--;
	return j * DIM_PAGINA + 0x80000000;
}

// rende di nuovo libero il frame descritto da df
void rilascia_frame(paddr f)
{
	natq j = (f-0x80000000) / DIM_PAGINA;
	if (j < N_M1) {
		fpanic("tentativo di rilasciare il frame %p di M1", f);
	}
	vdf[j].prossimo_libero = primo_frame_libero;
	primo_frame_libero = j;
	num_frame_liberi++;
}

// gestione del contatore di entrate valide (per i frame che contengono
// tabelle)
void inc_ref(paddr f)
{
	vdf[f / DIM_PAGINA].nvalide++;
}

void dec_ref(paddr f)
{
	vdf[f / DIM_PAGINA].nvalide--;
}

natl get_ref(paddr f)
{
	return vdf[f / DIM_PAGINA].nvalide;
}

/////////////////////////////////////////////////////////////////////////////////
//                         PAGINAZIONE                                         //
/////////////////////////////////////////////////////////////////////////////////

// indirizzo virtuale di partenza delle varie zone della memoria
// virtuale dei proceii

const vaddr ini_sis_c = norm(I_SIS_C * dim_region(MAX_LIV - 1)); // sistema condivisa
const vaddr ini_sis_p = norm(I_SIS_P * dim_region(MAX_LIV - 1)); // sistema privata
const vaddr ini_mio_c = norm(I_MIO_C * dim_region(MAX_LIV - 1)); // modulo IO
const vaddr ini_utn_c = norm(I_UTN_C * dim_region(MAX_LIV - 1)); // utente condivisa
const vaddr ini_utn_p = norm(I_UTN_P * dim_region(MAX_LIV - 1)); // utente privata

// indirizzo del primo byte che non appartiene alla zona specificata
const vaddr fin_sis_c = ini_sis_c + dim_region(MAX_LIV - 1) * N_SIS_C;
const vaddr fin_sis_p = ini_sis_p + dim_region(MAX_LIV - 1) * N_SIS_P;
const vaddr fin_mio_c = ini_mio_c + dim_region(MAX_LIV - 1) * N_MIO_C;
const vaddr fin_utn_c = ini_utn_c + dim_region(MAX_LIV - 1) * N_UTN_C;
const vaddr fin_utn_p = ini_utn_p + dim_region(MAX_LIV - 1) * N_UTN_P;

// alloca un frame libero destinato a contenere una tabella
paddr alloca_tab()
{
	paddr f = alloca_frame();
	if (f) {
		memset(reinterpret_cast<void*>(f), 0, DIM_PAGINA);
	}
	vdf[f / DIM_PAGINA].nvalide = 0;
	return f;
}

// dealloca un frame che contiene una tabella, controllando che non contenga
// entrate valide
void rilascia_tab(paddr f)
{
	if (int n = get_ref(f)) {
		fpanic("tentativo di deallocare la tabella %x con %d entrate valide", f, n);
	}
	rilascia_frame(f);
}

// setta l'entrata j-esima della tabella 'tab' con il valore 'se'.
// Si preoccupa di aggiustare apportunamente il contatore delle
// entrate valide.
void set_entry(paddr tab, natl j, tab_entry se)
{
	tab_entry& de = get_entry(tab, j);
	if ((se & BIT_V) && !(de & BIT_V)) {
		inc_ref(tab);
	} else if (!(se & BIT_V) && (de & BIT_V)) {
		dec_ref(tab);
	}
	de = se;
}

// copia 'n' descrittori a partire da quello di indice 'i' dalla
// tabella di indirizzo 'src' in quella di indirizzo 'dst'
void copy_des(paddr src, paddr dst, natl i, natl n)
{
	for (natl j = i; j < i + n && j < 512; j++) {
		tab_entry se = get_entry(src, j);
		set_entry(dst, j, se);
	}
}

// setta il valore 'e' su 'n' descrittori a partire da quello di indice 'i' nella
// tabella di indirizzo 'dst'
void set_des(paddr dst, natl i, natl n, tab_entry e)
{
	for (natl j = i; j < i + n && j < 512; j++) {
		set_entry(dst, j, e);
	}
}

// crea tutto il sottoalbero, con radice tab, necessario a tradurre tutti gli
// indirizzi dell'intervallo [begin, end). L'intero intervallo non deve
// contenere traduzioni pre-esistenti.
//
// I bit RW e US che sono a 1 nel parametro flags saranno settati anche in
// tutti i descrittori rilevanti del sottoalbero. Se flags contiene i bit PCD e/o PWT,
// questi saranno settati solo sui descrittori foglia.
//
// Il tipo getpaddr deve poter essere invocato come 'getpaddr(v)', dove 'v' è
// un indirizzo virtuale. L'invocazione deve restituire l'indirizzo fisico che
// si vuole far corrispondere a 'v'.
//
// La funzione, per default, crea traduzioni con pagine di livello 1. Se si
// vogliono usare pagine di livello superiore (da 2 a MAX_PS_LVL) occorre
// passare anche il parametro ps_lvl.
//
// La funzione restituisce il primo indirizzo non mappato, che in caso di
// sucesso è end. Un valore diverso da end segnala che si è verificato
// un problema durante l'operazione (per esempio: memoria esaurita, indirizzo
// già mappato).
//
// Lo schema generale della funzione, escludendo i controlli sugli errori e
// la gestione dei flag, è il seguente:
//
// new_f = 0;
// for (/* tutto il sotto-albero visitato in pre-ordine */) {
// 	tab_entry& e = 	/* riferimento all'entrata corrente */
// 	vaddr v = 	/* indirizzo virtuale corrente */
//	if (/* livello non foglia */) {
//		if (/* tabella assente */) {
//			new_f = alloca_tab();
//		}
//	} else {
//		new_f = getpaddr(v);
//	}
//	if (new_f) {
//		/* inizializza 'e' in modo che punti a 'new_f' */
//	}
// }
#include "type_traits"
template<typename T, typename = std::enable_if_t<!std::is_same_v<T, paddr(vaddr)>>>
vaddr map(paddr tab, vaddr begin, vaddr end, natl flags, const T& getpaddr, int ps_lvl = 1)
{
	// adattatore nel caso in cui getpaddr non sia un lvalue
	T tmp = getpaddr;
	return map(tab, begin, end, flags, tmp, ps_lvl);
}
template<typename T>
vaddr map(paddr tab, vaddr begin, vaddr end, natl flags, T& getpaddr, int ps_lvl = 1)
{
	vaddr v;	/* indirizzo virtuale corrente */
	int l;		/* livello (del TRIE) corrente */
	natq dr;	/* dimensione delle regioni di livello ps_lvl */
	const char *err_msg = "";
	natq allowed_flags = BIT_G | BIT_U | BIT_X | BIT_W | BIT_R;

	//boot_printf("Lancio map con tab begin end flags | %p %p %p %d\n",tab,begin,end,flags);
	int count = 0;

	// controlliamo che ps_lvl abbia uno dei valori consentiti
	if (ps_lvl < 1 || ps_lvl > MAX_PS_LVL) {
		fpanic("map: ps_lvl %d non ammesso (deve essere compreso tra 2 e %d)",
				ps_lvl, MAX_PS_LVL);
	}
	// il mapping coinvolge sempre almeno pagine, quindi consideriamo come
	// erronei dei parametri beg, end che non sono allineati alle pagine
	// (di livello ps_lvl)
	dr = dim_region(ps_lvl - 1);
	if (begin & (dr - 1)) {
		fpanic("map: begin=%p non allineato alle pagine di livello %d",
				begin, ps_lvl);
	}
	if (end & (dr - 1)) {
		fpanic("map: end=%p non allineato alle pagine di livello %d",
				end, ps_lvl);
	}
	// controlliamo che flags contenga solo i flag che possiamo gestire
	if (flags & ~allowed_flags) {
		fpanic("map: flags contiene bit non validi: %x", flags & ~allowed_flags);
	}

	// usiamo un tab_iter per percorrere tutto il sottoalbero.  Si noti che
	// il sottoalbero verrà costruito man mano che lo visitiamo.
	//
	// Si noti che tab_iter fa ulteriori controlli sulla validità
	// dell'intervallo (si veda tab_iter::valid_interval in libce)
	//
	// mentre percorriamo l'albero controlliamo che non esistano già
	// traduzioni per qualche indirizzo in [begin, end). In caso contrario
	// ci fermiamo restituendo errore.
	tab_iter it(tab, begin, end - begin);
	for ( /* niente */ ; it; it.next()) {
		tab_entry& e = it.get_e();
		l = it.get_l();
		v = it.get_v();
		// new_f diventerà diverso da 0 se dobbiamo settare a 1 il bit
		// P di 'e'
		paddr new_f = 0;

		if (l > ps_lvl) {
			// per tutti i livelli non "foglia" allochiamo la
			// tabella di livello inferiore e facciamo in modo che
			// il descrittore corrente la punti (Si noti che la
			// tabella potrebbe esistere già, e in quel caso non
			// facciamo niente)
			if (!(e & BIT_V)) {
				new_f = alloca_tab();
				if (!new_f) {
					err_msg = "impossibile allocare tabella";
					goto error;
				}
			} else if ((e & BIT_R) || (e & BIT_W) || (e & BIT_X)) {
				err_msg = "gia' mappato";
				goto error;
			}
		} else {
			// arrivati ai descrittori di livello ps_lvl creiamo la
			// traduzione vera e propria.
			if (e & BIT_V) {
				boot_printf("Map error: e ind l v new_f count | %p %p %d %p %p %d\n",e,&e,l,v,new_f,count);
				err_msg = "gia' mappato";
				goto error;
			}
			// otteniamo il corrispondente indirizzo fisico
			// chiedendolo all'oggetto getpaddr
			new_f = getpaddr(v);
			if (!new_f) {
				err_msg = "getpaddr() ha restituito 0";
				goto error;
			}

			// settiamo i bit dei flag richiesti
			e |= flags;

			count++;
		}
		if (new_f) {
			// 'e' non puntava a niente e ora deve puntare a new_f
			set_IND_FISICO(e, new_f);
			e |= BIT_V;

			// ricordiamoci di incrementare il contatore delle entrate
			// valide della tabella a cui 'e' appartiene
			inc_ref(it.get_tab());

		}

	}
	return end;

error:
	boot_printf("map: indirizzo %p, livello %d: %s\n", v, l, err_msg);
	// it punta all'ultima entrata visitata. Se si tratta di una tabella
	// dobbiamo provare a deallocare tutto il percorso che abbiamo creato
	// fino a quel punto. Una chiamata ad unmap() non è sufficiente, in
	// quanto queste tabelle potrebbero non contenere traduzioni complete.
	paddr p = it.get_tab();
	for (;;) {
		if (get_ref(p))
			break;
		rilascia_tab(p);
		if (!it.up())
			break;
		it.get_e() = 0;
		p = it.get_tab();
		dec_ref(p);
	}
	return (v > begin ? v : begin);
}

// elimina tutte le traduzioni nell'intervallo [begin, end).  Rilascia
// automaticamente tutte le sottotabelle che diventano vuote dopo aver
// eliminato le traduzioni. Per liberare le pagine vere e proprie, invece,
// chiama la funzione putpaddr() passandole l'indirizzo virtuale, fisico e il
// livello della pagina da elinimare.
template<typename T, typename = std::enable_if_t<!std::is_same_v<T, void(vaddr, paddr, int)>>>
void unmap(paddr tab, vaddr begin, vaddr end, const T& putpaddr)
{
	// adattatore nel caso in cui putpaddr non sia un lvalue
	T tmp = putpaddr;
	unmap(tab, begin, end, tmp);
}
template<typename T>
void unmap(paddr tab, vaddr begin, vaddr end, T& putpaddr)
{
	tab_iter it(tab, begin, end - begin);
	// eseguiamo una visita in ordine posticipato
	for (it.post(); it; it.next_post()) {
		tab_entry& e = it.get_e();

		if (!(e & BIT_V))
			continue;

		paddr p = extr_IND_FISICO(e);
		if (!it.is_leaf()) {
			// l'entrata punta a una tabella.
			if (!get_ref(p)) {
				// Se la tabella non contiene più entrate
				// valide la deallochiamo
				rilascia_tab(p);
			} else {
				// altrimenti non facciamo niente
				// (la tabella serve per traduzioni esterne
				// all'intervallo da eliminare)
				continue;
			}
		} else {
			// l'entrata punta ad una pagina (di livello it.get_l())
			// lasciamo al chiamante decidere cosa fare
			// con l'indirizzo fisico puntato da 'e'
			putpaddr(it.get_v(), p, it.get_l());
		}

		// per tutte le pagine, e per le tabelle che abbiamo
		// deallocato, azzeriamo l'entrata 'e' e decrementiamo il
		// contatore delle entrate valide nella tabella che la contiene
		e = 0;
		dec_ref(it.get_tab());
	}
}

// restituisce true sse v appartiene alla parte utente/condivisa
bool in_utn_c(vaddr v)
{
	return v >= ini_utn_c && v < fin_utn_c;
}

// mappa la memoria fisica in memoria virtuale, inclusa l'area PCI
bool crea_finestra_FM(paddr root_tab)
{
	auto identity_map = [] (vaddr v) -> paddr { return v; };
	// mappiamo tutta la memoria fisica:
	// - a livello sistema (bit U/S non settato)
	// - scrivibile (bit R/W settato)
	// - con pagine di grandi dimensioni (bit PS)
	//   (usiamo pagine di livello 2 che sono sicuramente disponibili)

	// vogliamo saltare la prima pagina per intercettare *nullptr, e inoltre
	// vogliamo settare il bit PWT per la pagina che contiene la memoria
	// video.  Per farlo dobbiamo rinunciare a settare PS per la prima regione

	// Mappa UART
	if(map(root_tab, UART0, UART0+DIM_PAGINA, BIT_X | BIT_W | BIT_R, identity_map) != (UART0+DIM_PAGINA)){
		return false;
	}

	// Mappa VIRTIO (al momento inutilizzato)
	#define VIRTIO0 0x10001000L
	if(map(root_tab, VIRTIO0, VIRTIO0+DIM_PAGINA, BIT_X | BIT_W | BIT_R, identity_map) != (VIRTIO0+DIM_PAGINA)){
		return false;
	}

	// Mappa PLIC 
	#define PLIC 0x0c000000L
	if(map(root_tab, PLIC, PLIC+0x400000, BIT_X | BIT_W | BIT_R, identity_map) != (PLIC+0x400000)){
		return false;
	}

	// Mappa VGA
	#define VGA_BASE 0x3000000L
	#define FRAMEBUFFER_VGA (0x50000000 | (0xb8000 - 0xa0000))
	if(map(root_tab, VGA_BASE, VGA_BASE+DIM_PAGINA, BIT_X | BIT_W | BIT_R, identity_map) != (VGA_BASE+DIM_PAGINA)){
		return false;
	}
	if(map(root_tab, FRAMEBUFFER_VGA, FRAMEBUFFER_VGA+DIM_PAGINA, BIT_X | BIT_W | BIT_R, identity_map) != (FRAMEBUFFER_VGA+DIM_PAGINA)){
		return false;
	}

	// mappiamo il kernel con tabelle di livello 2
	if (map(root_tab, KERNBASE, KERNBASE+MEM_TOT, BIT_X | BIT_W | BIT_R, identity_map, 2) != KERNBASE+MEM_TOT)
		return false;

	boot_printf("Creata finestra sulla memoria centrale:  [%p, %p)\n", DIM_PAGINA, KERNBASE+MEM_TOT);

	return true;
}

// restituisce l'indirizzo fisico che corrisponde a ind_virt nell'albero
// di traduzione con radice root_tab.
paddr trasforma(paddr root_tab, vaddr v)
{
	// usiamo un tab_iter sul solo indirizzo 'v', fermandoci
	// sul descrittore foglia lungo il percorso di traduzione
	tab_iter it(root_tab, v);
	while (it.down())
		;

	// si noti che il percorso potrebbe essere incompleto.
	// Ce ne accorgiamo perché il descrittore foglia ha il bit P a
	// zero. In quel caso restituiamo 0, che per noi non è un
	// indirizzo fisico valido.
	tab_entry e = it.get_e();
	if (!(e & BIT_V))
		return 0;

	// se il percorso è completo calcoliamo la traduzione corrispondente.
	// Si noti che non siamo necessariamente arrivati al livello 1, se
	// c'era un bit PS settato lungo il percorso.
	int l = it.get_l();
	natq mask = dim_region(l - 1) - 1;
	return (e & ~mask) | (v & mask);
}

///////////////////////////////////

extern "C" void test_paginazione_c(){
    // iizializziamo la parte M2
	init_frame();

	// creiamo le parti condivise della memoria virtuale di tutti i processi
	// le parti sis/priv e usr/priv verranno create da crea_processo()
	// ogni volta che si attiva un nuovo processo
	paddr root_tab = alloca_tab();
	if (!root_tab)
		boot_printf("Errore allocazione tabella root");
	// finestra di memoria, che corrisponde alla parte sis/cond
	if(!crea_finestra_FM(root_tab))
		boot_printf("Errore creazione finestra FM\n");

	// Attivazione paginazione
	writeSATP(root_tab);

	// Successo!
	printf("\nPaging enabled. Hi from Virtual Memory!\n\r");

	flog(LOG_INFO, "Hello from flog");
} 

