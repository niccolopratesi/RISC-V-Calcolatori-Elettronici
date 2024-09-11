#include "libce.h"
#include "costanti.h"
#include "sys.h"
#include "sysio.h"
#include "io.h"

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
struct des_console{
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

/*! @brief Parte C++ della primitiva iniconsole()
 *  @param cc Attributo colore per il video
 */

extern "C" void c_iniconsole(natb cc){

}

/*! @brief Inizializza la tastiera
 *  @return true in caso di successo, false altrimenti
 */

bool kbd_init(){



    return true;
}

/*! @brief Inizializza il video (modalità testo)
 *  @return true in caso di successo, false altrimenti
 */

bool vid_init(){

    //clear_screen(0x00,0x0F);

    return true;
}

/*! @brief Inizializza la console (tastiera + video)
 *  @return true in caso di successo, false altrimenti
 */

bool console_init(){
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


/*! @brief Parte C++ della primitiva readconsole()
 *  @param buff buffer che deve ricevere i caratteri letti
 *  @param quanti dimensione di _buff_
 *  @return numero di caratteri effettivamente letti
 */

extern "C" natq c_readconsole(char* buff, natq quanti){


    return 0;
}


/*! @brief Parte C++ della primitiva writeconsole()
 *  @param buff buffer da cui leggere i caratteri
 *  @param quanti numero di caratteri da scrivere
 */

 extern "C" void c_writeconsole(const char* buff, natq quanti){
    des_console *p_des = &console;

    if(!access(buff,quanti,false,false)){
        flog(LOG_WARN,"writeconsole: parametri non validi: %p, %lu:",buff,quanti);
        abort_p();
    }

    sem_wait(p_des->mutex);
    #ifndef AUTOCORR
        for(natq i = 0;i < quanti;i++){
            //vid::char_write(buff[i]);  print vga fa la stessa cosa
        }
    #else /*AUTOCORR*/
        if(quanti > 0 && buff[[quanti-1]] == '\n')
            quanti--;
        if(quanti > 0)
            flog(LOG_USR, "%.*s",static_cast<int>(quanti),buff);
    #endif /*AUTOCORR*/

    sem_signal(p_des->mutex);
 }


/// @}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// @defgroup ioerr Gestione errori
// @{
/////////////////////////////////////////////////////////////////////////////////////////////////////

/*! @brief segnala un errore fatale nel modulo I/O
 *  @param msg messaggio da inviare al log (severità LOG_ERR)
 */

extern "C" void panic(const char* msg){
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
extern "C" void main(natq sem_io) {

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