#include "tipo.h"

/// Numero di registri nel campo contesto del descrittore di processo
#define N_REG 31

/// @brief Descrittore di processo
struct des_proc {
	/// identificatore numerico del processo
	natw id;
	/// livello di privilegio (LIV_UTENTE o LIV_SISTEMA)
	natw livello;
	/// precedenza nelle code dei processi
	natl precedenza;
	/// indirizzo della base della pila sistema
	vaddr punt_nucleo;
	/// copia dei registri generali del processore
	natq contesto[N_REG];
    /// indirizzo di ritorno dall'interruzione
    natq epc;
	/// radice del TRIE del processo
	paddr satp;
	/// sstatus.SPIE memorizza l'abilitazione delle interruzioni, utile per codice sistema a interruzioni abilitate (modulo I/O)
	natw spie;
	/// prossimo processo in coda
	des_proc* puntatore;

	/// @name Informazioni utili per il debugging
	/// @{

	/// parametro `f` passato alla `activate_p`/`_pe` che ha creato questo processo
	void (*corpo)(natq);
	/// parametro `a` passato alla `activate_p`/`_pe` che ha creato questo processo
	natq  parametro;
	/// @}
};

/// @brief Indici delle copie dei registri nell'array contesto
enum { I_RA, I_SP, I_GP, I_TP, I_T0, I_T1, I_T2, I_S0, I_S1,
        I_A0, I_A1, I_A2, I_A3, I_A4, I_A5, I_A6, I_A7, 
        I_S2, I_S3, I_S4, I_S5, I_S6, I_S7, I_S8, I_S9, I_S10, I_S11, 
        I_T3, I_T4, I_T5, I_T6,  };


extern des_proc *esecuzione;
extern des_proc *esecuzione_precedente;
extern natl processi;
extern des_proc *pronti;

natl crea_dummy();
des_proc* crea_processo(void f(natq), natq a, int prio, char liv);
extern "C" void c_activate_p(void f(natq), natq a, natl prio, natl liv);
extern "C" void c_terminate_p(bool logms = true);
extern "C" void c_abort_p(bool selfdump = true);
extern "C" void c_do_log(log_sev sev, const char* buf, natl quanti);
des_proc *des_p(natw id);
void inserimento_lista(des_proc*& p_lista, des_proc* p_elem);
des_proc* rimozione_lista(des_proc*& p_lista);
extern "C" void inspronti();
extern "C" void schedulatore(void);
