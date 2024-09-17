#include "libce.h"
#include "vm.h"
#include "proc.h"
#include "costanti.h"
#include "sys.h"
#include "uart.h"
#include "plic.h"
#include "pci_risc.h"

__attribute__ ((aligned (16))) char stack0[4096];

/// Indirizzo base dello heap di sistema, fornito dal collegatore.
extern "C" natq __heap_start;

/// Dimensione dello heap di sistema.
const natq HEAP_SIZE = 1*MiB;

/// Ultimo indirizzo del modulo sistema + heap sistema, fornito dal collegatore
extern "C" natq end;

/// Un primo des_proc, allocato staticamente, da usare durante l'inizializzazione.
des_proc init;

/// Indirizzo base dell'ELF del modulo IO
natq start_io;

/// Indirizzo base dell'ELF del modulo utente
natq start_user;

/// Indirizzo base della memoria libera M2
natq start_M2;

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


/// @brief Sposta i file ELF del modulo I/O ed utente nel primo frame libero dopo la fine 
///        del modulo sistema
///
/// @note Setta le variabili @ref start_io e @ref start_user e @ref start_M2
void sposta_ELF_moduli();

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

	// pci_init();
	// flog(LOG_INFO, "PCI initialized");

	plic_init();
	flog(LOG_INFO, "PLIC Initialized");

	// Sposto i file ELF dei moduli I/O ed utente per allinearli 
	// e liberare la memoria da essi occupata per i frame liberi di M2
	sposta_ELF_moduli();
	
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
	//printf("\nPaging enabled. Hi from Virtual Memory!\n\r");
    flog(LOG_INFO, "Attivata paginazione");

	// anche se il primo processo non è completamente inizializzato,
	// gli diamo un identificatore, in modo che compaia nei log
	init.id = 0xFFFF;
	init.precedenza = MAX_PRIORITY;
	init.satp = readSATP();
	esecuzione = &init;
	esecuzione_precedente = esecuzione;

	flog(LOG_INFO, "Nucleo di Calcolatori Elettronici - RISC-V");

	// Usiamo come heap la parte di memoria compresa tra __heap_start e __heap_start + HEAP_SIZE + (start_M2 - start_io)
  heap_start = allinea(reinterpret_cast<void*>(&__heap_start), DIM_PAGINA);
	heap_init(0, 0, (natq) heap_start, (natq) heap_start + HEAP_SIZE + (start_M2 - start_io));
	flog(LOG_INFO, "Heap del modulo sistema: [%p, %p)", heap_start, heap_start + HEAP_SIZE + (start_M2 - start_io));

	flog(LOG_INFO, "Suddivisione della memoria virtuale:");
	flog(LOG_INFO, "- sis/cond [%p, %p)", ini_sis_c, fin_sis_c);
	flog(LOG_INFO, "- sis/priv [%p, %p)", ini_sis_p, fin_sis_p);
	flog(LOG_INFO, "- io /cond [%p, %p)", ini_mio_c, fin_mio_c);
	flog(LOG_INFO, "- usr/cond [%p, %p)", ini_utn_c, fin_utn_c);
	flog(LOG_INFO, "- usr/priv [%p, %p)", ini_utn_p, fin_utn_p);

	pci_init();
	flog(LOG_INFO, "PCI initialized");

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
	// in processi_asm.s, contiene solo 'call carica_stato; iretq'.
	salta_a_main();

  
  return 0;

error:
  fpanic("Errore di inizializzazione");
}

/// @brief Entry point del modulo IO
///
/// @note Inizializzato da crea_spazio_condiviso().
void (*io_entry)(natq);

/// @brief Entry point del modulo utente
///
/// @note Inizializzato da crea_spazio_condiviso().
void (*user_entry)(natq);

extern void test_keyboard_c();

/// @brief Corpo del processo main_sistema
void main_sistema(natq)
{
  	natl id;
	// inizializzazione del modulo I/O
	// creiamo un processo che esegua la procedura start del modulo I/O
	// Usiamo un semaforo di sincronizzazione per sapere quando
	// l'inizializzazione è terminata

	flog(LOG_INFO,"Creo il processo main I/O");
	natl sync_io = sem_ini(0);
	if (sync_io == 0xFFFFFFFF) {
		flog(LOG_ERR, "Impossibile allocare il semaforo di sincr per I/O");
		goto error;
	}
	id = activate_p(io_entry, sync_io, MAX_EXT_PRIO, LIV_SISTEMA);
	if (id == 0xFFFFFFFF) {
		flog(LOG_ERR, "Impossibile creare il processo main I/O");
		goto error;
	}
	flog(LOG_INFO,"Attendo inizializzazione modulo I/O");
	sem_wait(sync_io);

	// creazione del processo main utente
	flog(LOG_INFO, "Creo il processo main utente");
	id = activate_p(user_entry, 0, MAX_PRIORITY, LIV_UTENTE);
	if (id == 0xFFFFFFFF) {
		flog(LOG_ERR, "Impossibile creare il processo main utente");
		goto error;
	}

	// // TEST
	// test_keyboard_c();

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

#include "elf64.h"

/// @brief Oggetto da usare con map() per caricare un segmento ELF in memoria virtuale.
struct copy_segment {
	// Il segmento si trova in memoria agli indirizzi (fisici) [mod_beg, mod_end)
	// e deve essere visibile in memoria virtuale a partire dall'indirizzo
	// virt_beg. Il segmento verrà copiato (una pagina alla volta) in
	// frame liberi di M2. La memoria precedentemente occupata dal modulo
	// sarà poi riutilizzata per lo heap di sistema.

	/// base del segmento in memoria fisica
	paddr mod_beg;
	/// limite del segmento in memoria fisica
	paddr mod_end;
	/// indirizzo virtuale della base del segmento
	vaddr virt_beg;

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
	// allochiamo un frame libero in cui copiare la pagina
	paddr dst = alloca_frame();
	if (dst == 0)
		return 0;

	// offset della pagina all'interno del segmento
	natq offset = v - virt_beg;
	// indirizzo della pagina all'interno del modulo
	paddr src = mod_beg + offset;

	// il segmento in memoria può essere più grande di quello nel modulo.
	// La parte eccedente deve essere azzerata.
	natq tocopy = DIM_PAGINA;
	if (src > mod_end)
		tocopy = 0;
	else if (mod_end - src < DIM_PAGINA)
		tocopy =  mod_end - src;
	if (tocopy > 0)
		memcpy(voidptr_cast(dst), voidptr_cast(src), tocopy);
	if (tocopy < DIM_PAGINA)
		memset(voidptr_cast(dst + tocopy), 0, DIM_PAGINA - tocopy);
	return dst;
}

/*! @brief Carica un modulo in M2.
 *
 *  Copia il modulo in M2, lo mappa al suo indirizzo virtuale e
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
vaddr carica_modulo(natq mod_start, paddr root_tab, natq flags, natq heap_size)
{
	// puntatore all'intestazione ELF
	Elf64_Ehdr* elf_h  = ptr_cast<Elf64_Ehdr>(mod_start);
	// indirizzo fisico della tabella dei segmenti
	paddr ph_addr = mod_start + elf_h->e_phoff;
	// ultimo indirizzo virtuale usato
	vaddr last_vaddr = 0;
	// flag con cui eseguire la map
	natq map_flags;
	// esaminiamo tutta la tabella dei segmenti
	for (int i = 0; i < elf_h->e_phnum; i++) {
		Elf64_Phdr* elf_ph = ptr_cast<Elf64_Phdr>(ph_addr);

		// ci interessano solo i segmenti di tipo PT_LOAD
		if (elf_ph->p_type != PT_LOAD) {
			ph_addr += elf_h->e_phentsize;
			continue;
		}

		// i byte che si trovano ora in memoria agli indirizzi (fisici)
		// [mod_beg, mod_end) devono diventare visibili nell'intervallo
		// di indirizzi virtuali [virt_beg, virt_end).
		// il loader lascia una pagina per gli header, quindi si inizia a copiare da 0x1000 più avanti
		vaddr	virt_beg = elf_ph->p_vaddr,
			virt_end = virt_beg + elf_ph->p_memsz;
		paddr	mod_beg  = mod_start + elf_ph->p_offset,
			mod_end  = mod_beg + elf_ph->p_filesz;

		// se necessario, allineiamo alla pagina gli indirizzi di
		// partenza e di fine
		natq page_offset = virt_beg & (DIM_PAGINA - 1);
		virt_beg -= page_offset;
		mod_beg  -= page_offset;
		virt_end = allinea(virt_end, DIM_PAGINA);

		// aggiorniamo l'ultimo indirizzo virtuale usato
		if (virt_end > last_vaddr)
			last_vaddr = virt_end;

		// settiamo BIT nella traduzione
		map_flags = flags | BIT_G; // BIT_U e BIT_G;
		if (elf_ph->p_flags & PF_W)
			map_flags |= BIT_W;
		if (elf_ph->p_flags & PF_X)
			map_flags |= BIT_X;
		if (elf_ph->p_flags & PF_R)
			map_flags |= BIT_R;

		// mappiamo il segmento
		if (map(root_tab,
			virt_beg,
			virt_end,
			map_flags,
			copy_segment{mod_beg, mod_end, virt_beg}) != virt_end)
			return 0;

		flog(LOG_INFO, " - segmento %s %s mappato a [%lx, %lx)",
				(map_flags & BIT_U) ? "utente " : "sistema",
				(map_flags & BIT_W) ? "read/write" : "read-only ",
				virt_beg, virt_end);

		// passiamo alla prossima entrata della tabella dei segmenti
		ph_addr += elf_h->e_phentsize;
	}
	// dopo aver mappato tutti i segmenti, mappiamo lo spazio destinato
	// allo heap del modulo. I frame corrispondenti verranno allocati da
	// alloca_frame()
	if (map(root_tab,
		last_vaddr,
		last_vaddr + heap_size,
		flags | BIT_R | BIT_W | BIT_G,
		[](vaddr) { return alloca_frame(); }) != last_vaddr + heap_size)
		return 0;
	flog(LOG_INFO, " - heap:                                 [%lx, %lx)",
				last_vaddr, last_vaddr + heap_size);
	flog(LOG_INFO, " - entry point: 0x%lx", elf_h->e_entry);
	return elf_h->e_entry;
}

/*! @brief Mappa il modulo I/O.
 *  @param mod informazioni sul modulo caricato dal boot loader
 *  @param root_tab indirizzo fisico della radice del TRIE
 *  @return indirrizzo virtuale dell'entry point del modulo I/O,
 *  	   o zero in caso di errore
 */
 vaddr carica_IO(paddr root_tab)
 {
	flog(LOG_INFO, "mappo il modulo I/O:");
 	return carica_modulo(start_io, root_tab, 0, DIM_IO_HEAP);
 }

/*! @brief Mappa il modulo utente.
 *  @param mod informazioni sul modulo caricato dal boot loader
 *  @param root_tab indirizzo fisico della radice del TRIE
 *  @return indirrizzo virtuale dell'entry point del modulo utente,
 *  	   o zero in caso di errore
 */
vaddr carica_utente(paddr root_tab)
{
	flog(LOG_INFO, "mappo il modulo utente:");
	return carica_modulo(start_user, root_tab, BIT_U, DIM_USR_HEAP);
}

bool crea_spazio_condiviso(paddr root_tab)
{
	io_entry = ptr_cast<void(natq)>(carica_IO(root_tab));
	if (!io_entry)
		return false;
	user_entry = ptr_cast<void(natq)>(carica_utente(root_tab));
	if (!user_entry)
		return false;

	return true;
}


void sposta_ELF_moduli(){
	flog(LOG_INFO, "Spostamento ed allineamento dei file ELF modulo I/O e utente");

	char* puntatore_moduli = (char*) MOD_START;
	natq lunghezza_header=0;
	natq IO_size = 0;
	natq user_size = 0;
	natq exp = 1;
	natq i=0;

	//conto quanti  byte è lungo il size_file del modulo I/O
	for(; *(puntatore_moduli+lunghezza_header) != '\n';lunghezza_header++){
		exp *= 10;
	}
	exp/=10;

	//converto la stringa in numero decimale
	for(;i<lunghezza_header;i++){
		//48 è ASCII code del carattere 0
		natq conversione = *(puntatore_moduli+i)-48;
		IO_size += conversione*exp;
		exp/=10;
	}
	flog(LOG_DEBUG, "IO_size: %d byte", IO_size);

	exp=1;
	lunghezza_header++;
	i=lunghezza_header;
	//conto quanti  byte è lungo il size_file del modulo utente
	for(; *(puntatore_moduli+lunghezza_header) != '\n';lunghezza_header++){
		exp *= 10;
	}
	exp/=10;
	//converto la stringa in numero decimale
	for(;i<lunghezza_header;i++){
		//48 è ASCII code del carattere 0
		natq conversione = *(puntatore_moduli+i)-48;
		user_size += conversione*exp;
		exp/=10;
	}
	lunghezza_header++;

	flog(LOG_DEBUG, "user_size: %d byte", user_size);

	// Primo frame libero dopo il modulo sistema
	natq moduli_base = allinea(reinterpret_cast<paddr>(&end),DIM_PAGINA);

		
	start_io = moduli_base;
	flog(LOG_DEBUG, "new start io %p",start_io);

	start_user = allinea(reinterpret_cast<paddr>(moduli_base + IO_size),DIM_PAGINA);
	flog(LOG_DEBUG, "new start user %p",start_user);

	start_M2 = allinea(reinterpret_cast<paddr>(start_user + user_size),DIM_PAGINA);
	flog(LOG_DEBUG, "new start m2 %p",start_M2);

	// Allineo entrambi gli elf alla pagina durante la copia
	memcpy((void*) start_io,(void*)(puntatore_moduli+lunghezza_header),IO_size);
	memcpy((void*) start_user,(void*)(puntatore_moduli+lunghezza_header+IO_size),user_size);
	return;
}

