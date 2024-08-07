#include "libce.h"
#include "proc.h"
#include "costanti.h"
#include "vm.h"
#include "semafori.h"

/// @brief Descrittore di semaforo
struct des_sem {
	/// se >= 0, numero di gettoni contenuti;
	/// se < 0, il valore assoluto è il numero di processi in coda
	int counter;
	/// coda di processi bloccati sul semaforo
	des_proc* pointer;
};

/// @brief Array dei descrittori di semaforo.
///
/// I primi MAX_SEM semafori di array_dess sono per il livello utente, gli altri
/// MAX_SEM sono per il livello sistema.
des_sem array_dess[MAX_SEM * 2];

/// Numero di semafori allocati per il livello utente
natl sem_allocati_utente  = 0;

/// Numero di semafori allocati per il livello sistema (moduli sistema e I/O)
natl sem_allocati_sistema = 0;

/*! @brief Alloca un nuovo semaforo.
 *
 *  @return id del nuovo semaforo (0xFFFFFFFF se esauriti)
 */
natl alloca_sem()
{
	// I semafori non vengono mai deallocati, quindi è possibile allocarli
	// sequenzialmente. Per far questo è sufficiente ricordare quanti ne
	// abbiamo già allocati (variabili sem_allocati_utente e
	// sem_allocati_sistema)

	int liv = esecuzione->livello;
	natl i;
	if (liv == LIV_UTENTE) {
		if (sem_allocati_utente >= MAX_SEM)
			return 0xFFFFFFFF;
		i = sem_allocati_utente;
		sem_allocati_utente++;
	} else {
		if (sem_allocati_sistema >= MAX_SEM)
			return 0xFFFFFFFF;
		i = sem_allocati_sistema + MAX_SEM;
		sem_allocati_sistema++;
	}
	return i;
}

/*! @brief Verifica un id di semaforo
 *
 *  @param sem	id da verificare
 *  @return  true se sem è l'id di un semaforo allocato; false altrimenti
 */
bool sem_valido(natl sem)
{
	// dal momento che i semafori non vengono mai deallocati,
	// un semaforo è valido se e solo se il suo indice è inferiore
	// al numero dei semafori allocati

	int liv = esecuzione->livello;
	return sem < sem_allocati_utente ||
		(liv == LIV_SISTEMA && sem - MAX_SEM < sem_allocati_sistema);
}

/*! @brief Parte C++ della primitiva sem_ini().
 *  @param val	numero di gettoni iniziali
 */
extern "C" void c_sem_ini(int val)
{
	natl i = alloca_sem();

	if (i != 0xFFFFFFFF)
		array_dess[i].counter = val;

	esecuzione->contesto[I_A0] = i;
}

/*! @brief Parte C++ della primitiva sem_wait().
 *  @param sem id di semaforo
 */
extern "C" void c_sem_wait(natl sem)
{
	// una primitiva non deve mai fidarsi dei parametri
	if (!sem_valido(sem)) {
		flog(LOG_WARN, "semaforo errato: %d", sem);
		c_abort_p();
		return;
	}

	des_sem* s = &array_dess[sem];
	s->counter--;

	if (s->counter < 0) {
		inserimento_lista(s->pointer, esecuzione);
		schedulatore();
	}
}

/*! @brief Parte C++ della primitiva sem_signal().
 *  @param sem id di semaforo
 */
extern "C" void c_sem_signal(natl sem)
{
	// una primitiva non deve mai fidarsi dei parametri
	if (!sem_valido(sem)) {
		flog(LOG_WARN, "semaforo errato: %d", sem);
		c_abort_p();
		return;
	}

	des_sem* s = &array_dess[sem];
	s->counter++;

	if (s->counter <= 0) {
		des_proc* lavoro = rimozione_lista(s->pointer);
		inspronti();	// preemption
		inserimento_lista(pronti, lavoro);
		schedulatore();	// preemption
	}
}
/// @}
