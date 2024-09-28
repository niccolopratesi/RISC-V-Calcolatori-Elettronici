#include "libce.h"
#include "vm.h"
#include "proc.h"
#include "costanti.h"
#include "uart.h"

////////////////////////////////////////////////////////////////////////////////
//                         MEMORIA DINAMICA					       //
/////////////////////////////////////////////////////////////////////////////////

void *operator new(size_t s)
{
	return alloca(s);
}

void operator delete(void *p, unsigned long)
{
	dealloca(p);
}

void* operator new(size_t s, align_val_t a)
{
	return alloc_aligned(s, a);
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

// ultimo indirizzo del modulo sistema + heap sistema, fornito dal collegatore
extern "C" natq end;

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
	paddr fine_M1 = start_M2;
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

	flog(LOG_INFO, "Numero di frame: %ld (M1) %ld (M2)", N_M1, N_M2);
}

// estrae un frame libero dalla lista, se non vuota
paddr alloca_frame(){

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
void rilascia_frame(paddr f){

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
	vdf[(f-0x80000000L) / DIM_PAGINA].nvalide++;
}

void dec_ref(paddr f)
{
	vdf[(f-0x80000000L) / DIM_PAGINA].nvalide--;
}

natl get_ref(paddr f)
{
	return vdf[(f-0x80000000L) / DIM_PAGINA].nvalide;
}

/////////////////////////////////////////////////////////////////////////////////
//                         PAGINAZIONE                                         //
/////////////////////////////////////////////////////////////////////////////////

// alloca un frame libero destinato a contenere una tabella
paddr alloca_tab()
{
	paddr f = alloca_frame();
	if (f) {
		memset(voidptr_cast(f), 0, DIM_PAGINA);
		vdf[(f-0x80000000L)/ DIM_PAGINA].nvalide = 0;
	}
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

// restituisce true se v appartiene alla parte utente/condivisa
bool in_utn_c(vaddr v)
{
	return v >= ini_utn_c && v < fin_utn_c;
}

/*! @brief Parte C++ della primitiva access()
 *
 *  Primitiva utilizzata dal modulo I/O per controllare che i buffer passati dal
 *  livello utente siano accessibili dal livello utente (problema del Cavallo di
 *  Troia) e non possano causare page fault nel modulo I/O (bit P tutti a 1 e
 *  scrittura permessa quando necessario).
 *
 *  @param begin	base dell'intervallo da controllare
 *  @param dim		dimensione dell'intervallo da controllare
 *  @param writeable	se true, l'intervallo deve essere anche scrivibile
 *  @param shared	se true, l'intevallo deve trovarsi in utente/condivisa
 *  @return		true se i vincoli sono rispettati, false altrimenti
 */
extern "C" bool c_access(vaddr begin, natq dim, bool writeable, bool shared)
{
	esecuzione->contesto[I_A0] = false;

	if (!tab_iter::valid_interval(begin, dim))
		return false;

	if (shared && (!in_utn_c(begin) || (dim > 0 && !in_utn_c(begin + dim - 1))))
		return false;

	// usiamo un tab_iter per percorrere tutto il sottoalbero relativo
	// alla traduzione degli indirizzi nell'intervallo [begin, begin+dim).
	for (tab_iter it(esecuzione->satp << 12, begin, dim); it; it.next()) {
		tab_entry e = it.get_e();

		// interrompiamo il ciclo non appena troviamo qualcosa che non va
		// in RISC-V contano solo i bit dei permessi della foglia
		if (!(e & BIT_V))
			return false;
		
		if (it.is_leaf()) {
			if (!(e & BIT_U) || (writeable && !(e & BIT_W)))
				return false;
		}
	}
	esecuzione->contesto[I_A0] = true;
	return true;
}

/*! @brief Parte C++ della primitiva trasforma()
 *
 *  Traduce _ind_virt_ usando il TRIE del processo puntato
 *  da @ref esecuzione.
 *
 *  @param ind_virt indirizzo virtuale da tradurre
 */
extern "C" void c_trasforma(vaddr ind_virt)
{
	esecuzione->contesto[I_A0] = trasforma(esecuzione->satp << 12, ind_virt);
}
/// @}

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

	// Mappa VIRT TEST per lo shutdown
	if(map(root_tab, VIRT_TEST,  VIRT_TEST+DIM_PAGINA, BIT_X | BIT_W | BIT_R | BIT_G, identity_map) != (VIRT_TEST+DIM_PAGINA)){
		return false;
	}

	// Mappa UART
	if(map(root_tab, UART0, UART0+DIM_PAGINA, BIT_X | BIT_W | BIT_R | BIT_G, identity_map) != (UART0+DIM_PAGINA)){
		return false;
	}

	// Mappa VIRTIO (al momento inutilizzato)
	if(map(root_tab, VIRTIO0, VIRTIO0+DIM_PAGINA, BIT_X | BIT_W | BIT_R | BIT_G, identity_map) != (VIRTIO0+DIM_PAGINA)){
		return false;
	}

	// Mappa PLIC 
	if(map(root_tab, PLIC, PLIC+PLIC_SIZE, BIT_X | BIT_W | BIT_R | BIT_G, identity_map) != (PLIC+PLIC_SIZE)){
		return false;
	}

	//Mappa PCIe-ECAM
	if(map(root_tab, PCI_ECAM, PCI_ECAM+PCI_ECAM_SIZE, BIT_X | BIT_W | BIT_R | BIT_G, identity_map,2) != (PCI_ECAM+PCI_ECAM_SIZE)){
		return false;
	}

	//Mappa PCIe-MMIO
	if(map(root_tab, PCI_MMIO, PCI_MMIO+PCI_MMIO_SIZE, BIT_X | BIT_W | BIT_R | BIT_G, identity_map,2) != (PCI_MMIO+PCI_MMIO_SIZE)){
		return false;
	}

	// Mappa VGA
	/* #define VGA_BASE 0x3000000L
	#define FRAMEBUFFER_VGA (0x50000000 | (0xb8000 - 0xa0000))
	if(map(root_tab, VGA_BASE, VGA_BASE+DIM_PAGINA, BIT_X | BIT_W | BIT_R | BIT_G, identity_map) != (VGA_BASE+DIM_PAGINA)){
		return false;
	}
	if(map(root_tab, FRAMEBUFFER_VGA, FRAMEBUFFER_VGA+DIM_PAGINA, BIT_X | BIT_W | BIT_R | BIT_G, identity_map) != (FRAMEBUFFER_VGA+DIM_PAGINA)){
		return false;
	} */

	// mappiamo il kernel con tabelle di livello 2
	if (map(root_tab, KERNBASE, KERNBASE+MEM_TOT, BIT_X | BIT_W | BIT_R | BIT_G, identity_map, 2) != KERNBASE+MEM_TOT)
		return false;

	flog(LOG_INFO, "Creata finestra sulla memoria centrale:  [%p, %p)", DIM_PAGINA, KERNBASE+MEM_TOT);

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
	tab_entry& e = it.get_e();
	if (!(e & BIT_V))
		return 0;

	// se il percorso è completo calcoliamo la traduzione corrispondente.
	// Si noti che non siamo necessariamente arrivati al livello 1, se
	// c'era un bit PS settato lungo il percorso.
	paddr p = extr_IND_FISICO(e);
	int l = it.get_l();
	natq mask = dim_region(l - 1) - 1;
	return (p & ~mask) | (v & mask);
}
