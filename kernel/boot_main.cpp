#include "libce.h"
#include "vm.h"
#include "proc.h"
#include "costanti.h"
#include "uart.h"
#include "plic.h"
#include "pci_risc.h"

__attribute__ ((aligned (16))) char stack0[4096];

/// Indirizzo base dello heap di sistema, fornito dal collegatore.
extern "C" natq __heap_start;

/// Dimensione dello heap di sistema.
const natq HEAP_SIZE = 1*MiB;

/// Un primo des_proc, allocato staticamente, da usare durante l'inizializzazione.
des_proc init;

/*! @brief Corpo del processo main_sistema.
 *
 *  Si occupa della seconda parte dell'inizializzazione.
 */
void main_sistema(natq);

/*! @brief Crea il processo main_sistema
 *  @return id del processo
 */
natl crea_main_sistema()
{
	des_proc* m = crea_processo(main_sistema, 0, MAX_EXT_PRIO, LIV_SISTEMA);
	if (m == 0) {
		flog(LOG_ERR, "Impossibile creare il processo main_sistema");
		return 0xFFFFFFFF;
	}
	inserimento_lista(pronti, m);
	processi++;
	return m->id;
}

/// @brief Crea le parti utente/condivisa e io/condivisa
///
/// @note Setta le variabili @ref user_entry e @ref io_entry.
/// @param root_tab indirizzo fisico della tabella radice
/// @param mod informazioni sui moduli caricati dal boot loader
/// @return true in caso di successo, false altrimenti
bool crea_spazio_condiviso(paddr root_tab);

/// @brief Funzione di supporto pr avviare il primo processo (definita in processi_asm.s)
extern "C" void salta_a_main();

/// il file start.s salta a boot_main
extern "C" int boot_main(){

  natl mid, dummy_id;
  void *heap_start;

  flog(LOG_INFO, "Running in S-Mode");

  pci_init();
  flog(LOG_INFO, "PCI initialized");
  flog(LOG_INFO, "VGA inizialized");

  plic_init();
  flog(LOG_INFO, "PLIC Initialized");

  // Inizializzazione dei frame della parte M2
  init_frame();

  // creiamo le parti condivise della memoria virtuale di tutti i processi
	// le parti sis/priv e usr/priv verranno create da crea_processo()
	// ogni volta che si attiva un nuovo processo
	paddr root_tab = alloca_tab();
	if (!root_tab)
    goto error;
	flog(LOG_INFO, "Allocata tabella root");
	// finestra di memoria, che corrisponde alla parte sis/cond
	if(!crea_finestra_FM(root_tab))
    goto error;

	// Attivazione paginazione
	writeSATP(root_tab);

	// Successo!
	printf("\nPaging enabled. Hi from Virtual Memory!\n\r");
  flog(LOG_INFO, "Attivata paginazione");

	// anche se il primo processo non è completamente inizializzato,
	// gli diamo un identificatore, in modo che compaia nei log
	init.id = 0xFFFF;
	init.precedenza = MAX_PRIORITY;
	init.satp = readSATP();
	esecuzione = &init;
	esecuzione_precedente = esecuzione;

	flog(LOG_INFO, "Nucleo di Calcolatori Elettronici - Risc-V");

	// Usiamo come heap la parte di memoria comresa tra __heap_start e __heap_start + HEAP_SIZE
  heap_start = allinea(reinterpret_cast<void*>(&__heap_start), DIM_PAGINA);
	heap_init(heap_start, HEAP_SIZE);
	flog(LOG_INFO, "Heap del modulo sistema: [%p, %p)", heap_start, heap_start + HEAP_SIZE);

	flog(LOG_INFO, "Suddivisione della memoria virtuale:");
	flog(LOG_INFO, "- sis/cond [%p, %p)", ini_sis_c, fin_sis_c);
	flog(LOG_INFO, "- sis/priv [%p, %p)", ini_sis_p, fin_sis_p);
	flog(LOG_INFO, "- io /cond [%p, %p)", ini_mio_c, fin_mio_c);
	flog(LOG_INFO, "- usr/cond [%p, %p)", ini_utn_c, fin_utn_c);
	flog(LOG_INFO, "- usr/priv [%p, %p)", ini_utn_p, fin_utn_p);

	// Le parti sis/priv e usr/priv verranno create da crea_processo() ogni
	// volta che si attiva un nuovo processo.  La parte sis/cond contiene
	// la finestra FM.  Le parti io/cond e usr/cond
	// devono contenere i segmenti ELF dei moduli I/O e utente,
	// rispettivamente. In questo momento le copie di questi due file ELF
	// si trovano in memoria non mappate ma ad indirizzi forniti dal collegatore.
	// Mappiamo i loro segmenti codice e dati creando le necessarie traduzioni.
	// Creiamo queste traduzioni una sola volta all'avvio (adesso) e poi le
	// condividiamo tra tutti i processi.
	if (!crea_spazio_condiviso(init.satp << 12))
		goto error;
	flog(LOG_INFO, "Frame liberi: %d (M2)", num_frame_liberi);

	// creazione del processo dummy
	dummy_id = crea_dummy();
	if (dummy_id == 0xFFFFFFFF)
		goto error;
	flog(LOG_INFO, "Creato il processo dummy (id = %d)", dummy_id);

	// creazione del processo main_sistema
	mid = crea_main_sistema();
	if (mid == 0xFFFFFFFF)
		goto error;
	flog(LOG_INFO, "Creato il processo main_sistema (id = %d)", mid);

	flog(LOG_INFO, "Cedo il controllo al processo main sistema...");
	// ora possiamo passare a main_sistema(), che in questo momento è
	// in testa alla coda pronti.
	// Lo selezioniamo:
	schedulatore();
	// e poi carichiamo il suo stato. La funzione salta_a_main(), definita
	// in sistema.s, contiene solo 'call carica_stato; iretq'.
	salta_a_main();

  
  return 0;

error:
  fpanic("Errore di inizializzazione");
}

/// @brief Entry point del modulo IO
///
/// @note Inizializzato da crea_spazio_condiviso().
// void (*io_entry)(natq);

/// @brief Entry point del modulo utente
///
/// @note Inizializzato da crea_spazio_condiviso().
void (*user_entry)(natq);

/// @brief Corpo del processo main_sistema
void main_sistema(natq)
{
  natl id;
	// creazione del processo main utente
	flog(LOG_INFO, "Creo il processo main utente");
	id = activate_p(user_entry, 0, MAX_PRIORITY, LIV_UTENTE);
	if (id == 0xFFFFFFFF) {
		flog(LOG_ERR, "impossibile creare il processo main utente");
		goto error;
	}

	// terminazione
	flog(LOG_INFO, "Cedo il controllo al processo main utente...");
	terminate_p();

error:
	fpanic("Errore di inizializzazione");
}

///////////////////////////////////////////////////////////////////////////////////
/// @defgroup mod	Caricamento dei moduli I/O e utente
///
/// All'avvio troviamo il contenuto dei file `sistema`, `io` e `utente` già
/// copiati in memoria agli indirizzi indicati nel linker script kernel.ld.
/// Ora il modulo sistema deve rendere operativi anche i moduli 
/// IO e utente, creando le necessarie traduzioni nella memoria virtuale.
/// Per farlo dobbiamo mappare le sezioni testo e dati, i cui indirizzi
/// sono forniti dal collegatore, con i permessi corretti.
///
/// @{
///////////////////////////////////////////////////////////////////////////////////
// indirizzi di partenza e fine delle sezioni .text, .data e .bss del modulo utente, forniti dal collegatore
extern "C" natq __user_start;
extern "C" natq __user_etext;
extern "C" natq __user_data_start;
extern "C" natq __user_data_end;
extern "C" natq __user_bss_start;
extern "C" natq __user_end;

struct mod_info {
  /// indirizzi di partenza  e fine della sezione .text
  paddr text_start;
  paddr text_end;
  /// indirizzi di partenza e fine della sezione .data
  paddr data_start;
  paddr data_end;
  /// indirizzi di partenza e fine della sezione .bss
  paddr bss_start;
  paddr bss_end;

  /// indirizzo virtuale di partenza del modulo
  vaddr virt_beg;
};


/// @brief Oggetto da usare con map() per caricare un segmento ELF in memoria virtuale.
struct copy_segment {
	// Il segmento si trova in memoria agli indirizzi (fisici) [mod_beg, mod_end)
	// e deve essere visibile in memoria virtuale a partire dall'indirizzo
	// virt_beg.

	/// base del segmento in memoria fisica
	paddr mod_beg;
	/// limite del segmento in memoria fisica
	paddr mod_end;
	/// indirizzo virtuale della base del segmento
	vaddr virt_beg;
  /// se bss devo azzerare tutto
  bool bss;

	paddr operator()(vaddr);
};

/*! @brief Funzione chiamata da map().
 *
 *  Copia il prossimo frame di un segmento in un frame di M2.
 *  @param v indirizzo virtuale da mappare
 *  @return indirizzo fisico del frame di M2
 */
paddr copy_segment::operator()(vaddr v)
{

	// offset della pagina all'interno del segmento
	natq offset = v - virt_beg;
	// indirizzo della pagina all'interno del modulo
	paddr src = mod_beg + offset;
  // indirizzo fisico associato a v
  paddr dst = src;

	// il segmento in memoria può essere più grande di quello nel modulo.
	// La parte eccedente deve essere azzerata.
	natq tocopy = DIM_PAGINA;
	if (src > mod_end) {
    // memoria fisica terminata, devo allocare un nuovo frame e azzerarlo
    dst = alloca_frame();
    if (dst == 0)
      return 0;
		tocopy = 0;
  }
	else if (mod_end - src < DIM_PAGINA)
    // devo azzerare la parte eccedente
		tocopy =  mod_end - src;

  // se è bss devo azzerare tutto
  if (bss)
    tocopy = 0;

  // se c'è una parte da azzerare o se è un segmento bss 
	if (tocopy < DIM_PAGINA)
		memset(reinterpret_cast<void*>(dst + tocopy), 0, DIM_PAGINA - tocopy);
	return dst;
}

/*! @brief Mappa un modulo caricato in M2.
 *
 *  Mappa il modulo al suo indirizzo virtuale e
 *  aggiunge lo heap dopo l'ultimo indirizzo virtuale usato.
 *
 *  @param mod	informazioni sul modulo caricato dal boot loader
 *  @param root_tab indirizzo fisico della radice del TRIE
 *  @param flags	BIT_U per rendere il modulo accessibile da livello utente,
 *  		altrimenti 0
 *  @param heap_size dimensione dello heap (in byte)
 *  @return indirizzo virtuale dell'entry point del modulo, o zero
 *  	   in caso di errore
 */
vaddr carica_modulo(mod_info mod, paddr root_tab, natq flags, natq heap_size)
{
  vaddr last_vaddr = 0; // base dello heap
  vaddr virt_beg, virt_end;
  paddr mod_beg, mod_end;

  // Per ogni sezione, i byte che si trovano ora in memoria agli indirizzi (fisici)
  // [mod_beg, mod_end) devono diventare visibili nell'intervallo
  // di indirizzi virtuali [virt_beg, virt_end).

  // mappo la sezione .text del modulo con permessi di lettura ed esecuzione
  // l'inizio dei segmenti e dei moduli è sempre allineato alla pagina
  virt_beg = mod.virt_beg;
  virt_end = allinea(virt_beg + mod.text_end - mod.text_start, DIM_PAGINA);
  if (virt_end > last_vaddr) // aggiorniamo last_vaddr
    last_vaddr = virt_end;
  copy_segment text{mod.text_start, mod.text_end, virt_beg, false};
  if (map(root_tab, virt_beg, virt_end, flags | BIT_R | BIT_X, text) != virt_end)
    return 0;

  flog(LOG_INFO, " - segmento .text %s r-x mappato a [%p, %p)",
      (flags & BIT_U) ? "utente " : "sistema",
      virt_beg, virt_end);

  // mappo la sezione .data del modulo con permessi di lettura e scrittura
  virt_beg = virt_end;
  virt_end = allinea(virt_beg + mod.data_end - mod.data_start, DIM_PAGINA);
  if (virt_end > last_vaddr) // aggiorniamo last_vaddr
    last_vaddr = virt_end;
  copy_segment data{mod.data_start, mod.data_end, virt_beg, false};
  if (map(root_tab, virt_beg, virt_end, flags | BIT_R | BIT_W, data) != virt_end)
    return 0;

  flog(LOG_INFO, " - segmento .data %s rw- mappato a [%p, %p)",
      (flags & BIT_U) ? "utente " : "sistema",
      virt_beg, virt_end);

  // mappo la sezione .bss del modulo con permessi di lettura e scrittura
  virt_beg = virt_end;
  virt_end = allinea(virt_beg + mod.bss_end - mod.bss_start, DIM_PAGINA);
  if (virt_end > last_vaddr) // aggiorniamo last_vaddr
    last_vaddr = virt_end;
  copy_segment bss{mod.bss_start, mod.bss_end, virt_beg, true};
  if (map(root_tab, virt_beg, virt_end, flags | BIT_R | BIT_W, bss) != virt_end)
    return 0;

  flog(LOG_INFO, " - segmento .bss %s rw- mappato a [%p, %p)",
      (flags & BIT_U) ? "utente " : "sistema",
      virt_beg, virt_end);


	// dopo aver mappato tutti i segmenti, mappiamo lo spazio destinato
	// allo heap del modulo. I frame corrispondenti verranno allocati da
	// alloca_frame()
	if (map(root_tab,
		last_vaddr,
		last_vaddr + heap_size,
		flags | BIT_R | BIT_W,
		[](vaddr) { return alloca_frame(); }) != last_vaddr + heap_size)
		return 0;
	flog(LOG_INFO, " - heap:                                 [%p, %p)",
				last_vaddr, last_vaddr + heap_size);
	flog(LOG_INFO, " - entry point: 0x%lx", mod.text_start);
	return mod.virt_beg;
}

/*! @brief Mappa il modulo I/O.
 *  @param mod informazioni sul modulo caricato dal boot loader
 *  @param root_tab indirizzo fisico della radice del TRIE
 *  @return indirrizzo virtuale dell'entry point del modulo I/O,
 *  	   o zero in caso di errore
 */
// vaddr carica_IO(boot64_modinfo* mod, paddr root_tab)
// {
// 	flog(LOG_INFO, "mappo il modulo I/O:");
// 	return carica_modulo(mod, root_tab, 0, DIM_IO_HEAP);
// }

/*! @brief Mappa il modulo utente.
 *  @param mod informazioni sul modulo caricato dal boot loader
 *  @param root_tab indirizzo fisico della radice del TRIE
 *  @return indirrizzo virtuale dell'entry point del modulo utente,
 *  	   o zero in caso di errore
 */
vaddr carica_utente(paddr root_tab)
{
  mod_info mod;

  mod.text_start = reinterpret_cast<paddr>(&__user_start);
  mod.text_end = reinterpret_cast<paddr>(&__user_etext);
  mod.data_start = reinterpret_cast<paddr>(&__user_data_start);
  mod.data_end = reinterpret_cast<paddr>(&__user_data_end);
  mod.bss_start = reinterpret_cast<paddr>(&__user_bss_start);
  mod.bss_end = reinterpret_cast<paddr>(&__user_end);
  mod.virt_beg = ini_utn_c;

	flog(LOG_INFO, "mappo il modulo utente:");
	return carica_modulo(mod, root_tab, BIT_U, DIM_USR_HEAP);
}

bool crea_spazio_condiviso(paddr root_tab)
{
	// io_entry = ptr_cast<void(natq)>(carica_IO(&mod[1], root_tab));
	// if (!io_entry)
	// 	return false;
	user_entry = ptr_cast<void(natq)>(carica_utente(root_tab));
	if (!user_entry)
		return false;

	return true;
}

