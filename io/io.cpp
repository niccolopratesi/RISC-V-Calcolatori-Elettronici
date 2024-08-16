include "all.h"

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
}

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

bool vit_init(){



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
    


    flog(LOG_INFO,"inizializzo la console (kbd + video)");
    if(!console_init()){
        panic("inizializzazione console fallita");
    }


    flog(LOG_INFO,"inizializzazione modulo I/O completata");
    sem_signa(sem_io);
    terminate_p();
}

/// @}