#include "libce.h"
#include "costanti.h"
#include "sys.h"
#include "sysio.h"
#include "io.h"
#include "kbd.h"
#include "vid.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// @defgroup ioheap Memoria Dinamica
//
// Dal momento che le funzioni del modulo I/O sono eseguite coon le interruzioni esterne
// mascherabili abilitate, dobbbiamo proteggere lo heap I/O con uun semaforo 
// di mutuaa esclusione
//
// @{
/////////////////////////////////////////////////////////////////////////////////////////////////////

natl ioheap_mutex;

// indirizzo di partenza per l'heap del modulo I/O, fornito dal collegatore
extern "C" natb end[];

/*! @brief Alloca un oggetto nello heap I/O
 *  @param s dimensione dell'oggetto
 *  @return puntatore all'oggetto (nullptr se heap esaurito)
 */
void* operator new(size_t s)
{
	void* p;

	sem_wait(ioheap_mutex);
	p = alloca(s);
	sem_signal(ioheap_mutex);

	return p;
}

/*! @brief Alloca un oggetto nello heap I/O, con vincoli di allineamento
 *  @param s dimensione dell'oggetto
 *  @param a allineamento richiesto 
 *  @return puntatore all'oggetto (nullptr se heap esaurito)
 */
void* operator new(size_t s, align_val_t a)
{
	void* p;

	sem_wait(ioheap_mutex);
	p = alloc_aligned(s,a);
	sem_signal(ioheap_mutex);

	return p;
}

/*! @brief Dealloca un oggetto restituendolo all'heap I/O
 * @param p puntatore all'oggetto
 */
void operator delete(void* p)
{
	sem_wait(ioheap_mutex);
	dealloca(p);
	sem_signal(ioheap_mutex);
}

/// @}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// @defgroup console Console
//
// Per console intendiamo l'unione della tastiera e del video(modalità testo)
//
// @{
/////////////////////////////////////////////////////////////////////////////////////////////////////

// Descrittore della Console
struct des_console {
    /// semaforo di mutua esclusione per l'accesso alla console
    natl mutex;
    /// semaforo di sincronizzazione per le letture da tastiera
    natl sincr;
    /// Dove scrivere il prossimo carattere letto
    char* punt;
    /// Quanti caratteri resta da leggere
    natq cont;
    /// Dimensione del buffer passato a @ref readconsole()
    natq dim;
};

// Unica istanza di des_console
des_console console;

/*! @brief Parte C++ della primitiva writeconsole()
 *  @param buff buffer da cui leggere i caratteri
 *  @param quanti numero di caratteri da scrivere
 */
extern "C" void c_writeconsole(const char* buff, natq quanti)
{
    des_console *p_des = &console;

    if (!access(buff, quanti, false, false)) {
        flog(LOG_WARN,"writeconsole: parametri non validi: %p, %ld:", buff, quanti);
        abort_p();
    }

    sem_wait(p_des->mutex);
#ifndef AUTOCORR
    for (natq i = 0; i < quanti; i++)
        vid::char_write(buff[i]);
#else /*AUTOCORR*/
    if (quanti > 0 && buff[quanti - 1] == '\n')
        quanti--;
    if (quanti > 0)
        flog(LOG_USR, "%.*s", static_cast<int>(quanti), buff);
#endif /*AUTOCORR*/

    sem_signal(p_des->mutex);
}

/*! @brief Avvua una operazione di lettura dalla tastiera
 *  @param d    descrittore della console
 *  @param buff buffer che deve ricevere i caratteri
 *  @param dim  dimensione di _buff_
 */
void startkbd_in(des_console *d, char *buff, natq dim)
{
    d->punt = buff;
    d->cont = dim;
    d->dim = dim;
    kbd::enable_intr();
}

/*! @brief Parte C++ della primitiva readconsole()
 *  @param buff buffer che deve ricevere i caratteri letti
 *  @param quanti dimensione di _buff_
 *  @return numero di caratteri effettivamente letti
 */
extern "C" natq c_readconsole(char* buff, natq quanti)
{
    des_console *d = &console;
    natq rv;

    if (!access(buff, quanti, true)) {
        flog(LOG_WARN, "readconsole: parametri non validi: %p, %ld:", buff, quanti);
        abort_p();
    }

#ifdef AUTOCORR
    return 0;
#endif

    if (!quanti)
        return 0;

    sem_wait(d->mutex);
    startkbd_in(d, buff, quanti);
    sem_wait(d->sincr);
    rv = d->dim - d->cont;
    sem_signal(d->mutex);
    return rv;
}

/// @brief Processo esterno associato alla tastiera
void estern_kbd(int)
{
  des_console *d = &console;
  char a;
  bool fine;

  for (;;) {
    kbd::disable_intr();

    while (kbd::more_to_read()) {
      a = kbd::char_read_intr();

      if (a == 0)
        continue;

      fine = false;
      switch (a) {
      case '\b':
        if (d->cont < d->dim) {
          d->punt--;
          d->cont++;
          vid::str_write("\b \b");
        }
        break;
      case '\r':
      case '\n':
        fine = true;
        *d->punt = '\0';
        vid::str_write("\r\n");
        break;
      default:
        *d->punt = a;
        d->punt++;
        d->cont--;
        vid::char_write(a);
        if (d->cont == 0)
          fine = true;
        break;
      }
    }

    kbd::add_max_buf();
    if (fine)
      sem_signal(d->sincr);
    else
      kbd::enable_intr();
    wfi();
  }
}

/*! @brief Parte C++ della primitiva iniconsole()
 *  @param cc Attributo colore per il video
 */
extern "C" void c_iniconsole(natb cc)
{
    vid::clear_screen(cc);
}

/*! @brief Inizializza la tastiera
 *  @return true in caso di successo, false altrimenti
 */
bool kbd_init()
{
    // inizializzazione delle strutture dati della tastiera
    kbd::eventq = (virtq *) trasforma(alloca(sizeof(*kbd::eventq)));
    kbd::statusq = (virtq *) trasforma(alloca(sizeof(*kbd::statusq)));
    kbd::buf = (virtio_input_event *) trasforma(alloca(sizeof(*kbd::buf) * kbd::QUEUE_SIZE));
    create_virtq(*kbd::eventq, kbd::QUEUE_SIZE, 0);
    create_virtq(*kbd::statusq, kbd::QUEUE_SIZE, 0);
    kbd::eventq->desc = (virtq_desc *) trasforma(kbd::eventq->desc);
    kbd::eventq->avail = (virtq_avail *) trasforma(kbd::eventq->avail);
    kbd::eventq->used = (virtq_used *) trasforma(kbd::eventq->used);
    kbd::statusq->desc = (virtq_desc *) trasforma(kbd::statusq->desc);
    kbd::statusq->avail = (virtq_avail *) trasforma(kbd::statusq->avail);
    kbd::statusq->used = (virtq_used *) trasforma(kbd::statusq->used);
    // le interruzioni sono disabilitate di default dalla init()
    if (!kbd::init()) {
      flog(LOG_ERR, "kbd: impossibile configurare la tastiera");
      return false;
    }

    // prepariamo la virtqueue per ricevere i dati
    kbd::add_max_buf();

    if (activate_pe(estern_kbd, 0, MAX_EXT_PRIO - KBD_IRQ, LIV_SISTEMA, KBD_IRQ) == 0xFFFFFFFF) {
        flog(LOG_ERR, "kbd: imposssibile creare estern_kbd");
        return false;
    }
    flog(LOG_INFO, "kbd: tastiera inizializzata");
    return true;
}

/*! @brief Inizializza il video (modalità testo)
 *  @return true in caso di successo, false altrimenti
 */
bool vid_init()
{
    // vid::clear_screen(vid::attr);
    flog(LOG_INFO, "vid: video inizializzato");
    return true;
}

/*! @brief Inizializza la console (tastiera + video)
 *  @return true in caso di successo, false altrimenti
 */
bool console_init()
{
    des_console* d = &console;

    if((d->mutex = sem_ini(1)) == 0xFFFFFFFF){
        flog(LOG_ERR,"console: impossibile creare mutex");
        return false;
    }

    if((d->sincr = sem_ini(0)) == 0xFFFFFFFF){
        flog(LOG_ERR,"console: impossibile creare sincr");
        return false;
    }
    return kbd_init() && vid_init();
}

/// @}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// @defgroup ioerr Gestione errori
// @{
/////////////////////////////////////////////////////////////////////////////////////////////////////

/*! @brief segnala un errore fatale nel modulo I/O
 *  @param msg messaggio da inviare al log (severità LOG_ERR)
 */
extern "C" void panic(const char* msg)
{
    flog(LOG_ERR,"modulo I/O: %s",msg);
    io_panic();
}

/// @}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// @defgroup ioinit Inizializzazione
// @{
/////////////////////////////////////////////////////////////////////////////////////////////////////

/*! @brief corpo del processo main I/O
 *
 *  Il modulo sistema crea il processo main I/O all'avvio e gli cede il controllo,
 *  passandogli l'indice di un semaforo di sincronizzazione.
 *  Il modulo I/O deve eseguire una sem_signal() su questo semaforo quando ha terminato
 *  la fase di inizializzazione.
 *
 *  @param sem_io indice del semaforo di sincronizzazione
 */
extern "C" void main(natq sem_io)
{
    //inizializzazione semaforo mutua esclusione per heap
    ioheap_mutex = sem_ini(1);
    if(ioheap_mutex == 0xFFFFFFFF){
        panic("Impossibile creare semaforo ioheap_mutex");
    }

    natb* end_ = allinea(end,DIM_PAGINA);
    //inizializzazione heap modulo I/O
    heap_init(allinea_ptr(end_, DIM_PAGINA), DIM_IO_HEAP);
	  flog(LOG_INFO, "Heap del modulo I/O: %lx [%p, %p)", DIM_IO_HEAP,
			end_, end_ + DIM_IO_HEAP);

    //inizializzazione periferiche
    flog(LOG_INFO,"Inizializzo la console (kbd + video)");
    if(!console_init()){
        panic("Inizializzazione console fallita");
    }
    
    flog(LOG_INFO,"Inizializzazione modulo I/O completata");

    sem_signal(sem_io);
    terminate_p();
}

/// @}
