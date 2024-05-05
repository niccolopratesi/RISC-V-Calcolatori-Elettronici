#ifndef VM_H
#define VM_H
#include "libce.h"
#include "costanti.h"

/// numero di livelli del TRIE
static const int MAX_LIV = 4;
// massimo livello supportato per le pagine di grandi dimensioni
#define MAX_PS_LVL		2
static const natq BIT_SEGNO = (1ULL << (12 + 9 * MAX_LIV - 1));
static const natq MASCHERA_MODULO = BIT_SEGNO - 1;
// restituisce la versione normalizzata (16 bit piu' significativi uguali al
// bit 47) dell'indirizzo a
static inline vaddr norm(vaddr a)
{
	return (a & BIT_SEGNO) ? (a | ~MASCHERA_MODULO) : (a & MASCHERA_MODULO);
}

// restituisce la dimensione di una regione di livello liv
static inline constexpr natq dim_region(int liv)
{
	natq v = 1ULL << (liv * 9 + 12);
	return v;
}

// dato un indirizzo 'v', restituisce l'indirizzo della
// regione di livello 'liv' a cui 'v' appartiene
static inline vaddr base(vaddr v, int liv)
{
	natq mask = dim_region(liv) - 1;
	return v & ~mask;
}

// dato l'idirizzo 'e', estremo destro di un intervallo [b, e),
// restituisce l'indirizzo della prima regione di livello 'liv'
// a destra dell'intervallo.
static inline vaddr limit(vaddr e, int liv)
{
	natq dr = dim_region(liv);
	natq mask = dr - 1;
	return (e + dr - 1) & ~mask;
}

// indirizzo virtuale di partenza delle varie zone della memoria
// virtuale dei processi

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


typedef natq tab_entry;

//   ( definiamo alcune costanti utili per la manipolazione dei descrittori
//     di pagina e di tabella. Assegneremo a tali descrittori il tipo "natq"
//     e li manipoleremo tramite maschere e operazioni sui bit.

// Altre informazioni disponibili a: https://riscv.org/wp-content/uploads/2017/05/riscv-privileged-v1.10.pdf (pag.60)

const natq BIT_V   = 1U << 0; // il bit di validita' (simile al Bit P di x86)
const natq BIT_R   = 1U << 1; // il bit di lettura
const natq BIT_W   = 1U << 2; // il bit di scrittura
const natq BIT_X   = 1U << 3; // il bit di esecuzione
const natq BIT_U   = 1U << 4; // il bit di accessibilita' in modalita' utente (simile al Bit U/S di x86)
const natq BIT_G   = 1U << 5; // il bit globale (pagine globali non vengono scaricate/invalidate a cambio contesto)
const natq BIT_A   = 1U << 6; // il bit di accesso
const natq BIT_D   = 1U << 7; // il bit "dirty"

// Lo schema di indirzzamento Sv48 di Risc-V utilizza 48 bit per gli indirizzi virtuali e 56 per quelli fisici.
// Nonostante le pagine siano di 4kiB, l'indirizzo fisico parte dal bit 10.
// Conviene percio' utilizzare due maschere.
const natq ADDR_MASK  = 0xFFFFFFFFFFF000; // maschera per l'indirizzo (44 bit, le pagine devono essere allineate a 4Kib, offset di 12 bit)
const natq PTE_MASK   = 0x3FFFFFFFFFFC00; // maschera per la page table (44 bit, offset di 10 bit)

// )

static inline paddr extr_IND_FISICO(tab_entry descrittore)
{ // (
	return ((descrittore & PTE_MASK) << 2); // )
}
static inline void  set_IND_FISICO(tab_entry& descrittore, paddr ind_fisico) //
{ // (
	descrittore &= ~PTE_MASK;
	descrittore |= ((ind_fisico & ADDR_MASK)>>2); // )
}

// dato un indirizzo virtuale 'ind_virt' ne restituisce
// l'indice del descrittore corrispondente nella tabella di livello 'liv'
static inline int i_tab(vaddr ind_virt, int liv)
{
	int shift = 12 + (liv - 1) * 9;
	natq mask = 0x1ffULL << shift;
	return (ind_virt & mask) >> shift;
}
// dato l'indirizzo di una tabella e un indice, restituisce un
// riferimento alla corrispondente entrata
static inline tab_entry& get_entry(paddr tab, natl index)
{
	tab_entry *pd = reinterpret_cast<tab_entry*>(tab);
	return  pd[index];
}

// Scrive una entrata di una tabella
void set_entry(paddr tab, natl j, tab_entry se);

// Copia descrittori da una tabella ad un'altra
void copy_des(paddr src, paddr dst, natl i, natl n);

// Inizializza (parte dei) descrittori di una tabella
void set_des(paddr dst, natl i, natl n, tab_entry e);

class tab_iter {

	// ogni riga dello stack, partendo dalla 0, traccia la posizione
	// corrente all'interno dell'albero di traduzione.  La riga 0 traccia
	// la posizione nell'albero al livello massimo, la riga 1 nel livello
	// subito inferiore, e così via. Il livello corrente è contenuto
	// nella variabile 'l'.
	//
	// Ogni riga dello stack contiene l'indirizzo fisico (tab) della
	// tabella, del livello corrispondente, attualmente in corso di visita.
	// La riga contiene anche un intervallo [cur, end) di indirizzi ancora
	// da visitare in quella tabella. I campi cur e end della riga MAX_LIV,
	// altrimenti inutilizzati, vengono usati per contenere gli estremi
	// dell'intervallo completo passato al costruttore dell'iteratore.
	//
	// La terminazione della visita si riconosce da p->tab == 0.

	struct stack {
		vaddr cur, end;
		paddr tab;
	} s[MAX_LIV + 1];

	int l;

	stack *sp() { return &s[l - 1]; }
	stack const *sp() const { return &s[l - 1]; }
	stack *sp(int lvl) { return &s[lvl - 1]; }
	stack *pp() { return &s[MAX_LIV]; }
	stack const *pp() const { return &s[MAX_LIV]; }
	bool done() const { return !sp()->tab; }

public:
	static bool valid_interval(vaddr beg, natq dim)
	{
		vaddr end = beg + dim - 1;
		return !dim || (
			// non inizia nel buco
			     norm(beg) == beg
			// non termina nel buco
			&&   norm(end) == end
			// tutto dalla stessa parte rispetto al buco
			&&   (beg & BIT_SEGNO) == (end & BIT_SEGNO));
	}

	tab_iter(paddr tab, vaddr beg, natq dim = 1, int liv = MAX_LIV);

	// conversione a booleano: true sse la visita è terminata
	// (le altre funzioni non vanno chiamate se l'iteratore è in questo stato)
	operator bool() const {
		return !done();
	}

	// restituisce il livello su cui si trova l'iteratore
	int get_l() const {
		return l;
	}

	// restituisce true sse l'iteratore si trova su una foglia
	// In Risc-V, una PTE e' foglia se possiede a 1 almeno un bit RWX
	bool is_leaf() const {
		tab_entry e = get_e();
		return !(e & BIT_V) || ((e & BIT_R) || (e & BIT_W) || (e & BIT_X)) || l == 1;
	}

	// restituisce il più piccolo indirizzo virtuale appartenente
	// all'intevallo e la cui traduzione passa dal descrittore corrente
	vaddr get_v() const {
		return max(pp()->cur, sp()->cur);
	}

	// restituisce un riferimento al descrittore corrente
	tab_entry& get_e() const {
		return get_entry(sp()->tab, i_tab(sp()->cur, l));
	}

	// restituisce l'indirizzo fisico della tabella che contiene il
	// descrittore corrente
	paddr get_tab() const {
		return sp()->tab;
	}

	// prova a spostare l'iteratore di una posizione in basso nell'albero,
	// se possibile, altrimenti non fa niente. Restituisce true sse lo
	// spostamento è avvenuto.
	bool down();
	// prova a spostare l'iteratore di una posizione in alto nell'albero,
	// se possibile, altrimenti non fa niente. Restituisce true sse lo
	// spostamento è avvenuto.
	bool up();
	// prova a spostare l'iteratore di una posizione a destra nell'albero
	// (rimanendo nel livello corrente), se possibile, altrimenti non fa
	// niente. Restituisce true sse lo spostamento è avvenuto.
	bool right();

	// porta l'iteratore alla prossima posizione della visita in ordine
	// anticipato
	void next();

	// inizia una visita in ordine posticipato
	void post();
	// porta l'iteratore alla prossima posizione della visita in ordine
	// posticipato
	void next_post();
};

// Traduzione di un indirizzo virtuale in fisico.
paddr trasforma(paddr root_tab, vaddr v);

//invalida il TLB
extern "C" void invalida_TLB();

// invalida una entrata del TLB
extern "C" void invalida_entrata_TLB(vaddr v);

// inizializza la lista dei frame liberi
extern void init_frame();

// estrae un frame libero dalla lista, se non vuota
extern paddr alloca_frame();

// rende di nuovo libero il frame descritto da df
extern void rilascia_frame(paddr f);

/// invocata quando serve una nuova tabella
extern paddr alloca_tab();
/// invocata quando una tabella non serve più
extern void rilascia_tab(paddr);
/// invocata quando una tabella acquisisce una nuova entrata valida
extern void inc_ref(paddr);
/// invocata quando una tabella perde una entrata precedentemente valida
extern void dec_ref(paddr);
/// invocata per leggere il contatore delle entrate valide di una tabella
extern natl get_ref(paddr);

// crea la finestra di memoria
extern bool crea_finestra_FM(paddr root_tab);

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
#include "type_traits.h"
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
	flog(LOG_WARN, "map: indirizzo %p, livello %d: %s\n", v, l, err_msg);
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

extern natq num_frame_liberi;
// Spazio utente
#define TRAMPOLINE	(MAXVA - DIM_PAGINA)
#define TRAPFRAME	(TRAMPOLINE - DIM_PAGINA)

#endif
