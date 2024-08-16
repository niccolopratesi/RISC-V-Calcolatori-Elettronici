include "all.h"




/////////////////////////////////////////////////////////////////////////
// @defgroup ioerr Gestione errori
// @{
/////////////////////////////////////////////////////////////////////////

/*! @brief segnala un errore fatale nel modulo I/O
 *  @param msg messaggio da inviare al log (severità LOG_ERR)
 */

extern "C" void panic(const char* msg){
    flog(LOG_ERR,"modulo I/O: %s",msg);
    io_panic();
}

/// @}

/////////////////////////////////////////////////////////////////////////
// @defgroup ioinit Inizializzazione
// @{
/////////////////////////////////////////////////////////////////////////

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