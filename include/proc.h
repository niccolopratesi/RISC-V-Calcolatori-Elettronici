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
extern "C" natl activate_p(void f(natq), natq a, natl prio, natl liv);
extern "C" void terminate_p(bool logms = true);
des_proc *des_p(natw id);
void inserimento_lista(des_proc*& p_lista, des_proc* p_elem);
des_proc* rimozione_lista(des_proc*& p_lista);
extern "C" void inspronti();
extern "C" void schedulatore(void);























// // Saved registers for kernel context switches.
// struct context {
//   natq ra;
//   natq sp;

//   // callee-saved
//   natq s0;
//   natq s1;
//   natq s2;
//   natq s3;
//   natq s4;
//   natq s5;
//   natq s6;
//   natq s7;
//   natq s8;
//   natq s9;
//   natq s10;
//   natq s11;
// };

// // per-process data for the trap handling code in trampoline.S.
// // sits in a page by itself just under the trampoline page in the
// // user page table. not specially mapped in the kernel page table.
// // uservec in trampoline.S saves user registers in the trapframe,
// // then initializes registers from the trapframe's
// // kernel_sp, kernel_hartid, kernel_satp, and jumps to kernel_trap.
// // usertrapret() and userret in trampoline.S set up
// // the trapframe's kernel_*, restore user registers from the
// // trapframe, switch to the user page table, and enter user space.
// // the trapframe includes callee-saved user registers like s0-s11 because the
// // return-to-user path via usertrapret() doesn't return through
// // the entire kernel call stack.
// struct trapframe {
//   /*   0 */ natq kernel_satp;   // kernel page table
//   /*   8 */ natq kernel_sp;     // top of process's kernel stack
//   /*  16 */ natq kernel_trap;   // usertrap()
//   /*  24 */ natq epc;           // saved user program counter
//   /*  40 */ natq ra;
//   /*  48 */ natq sp;
//   /*  56 */ natq gp;
//   /*  64 */ natq tp;
//   /*  72 */ natq t0;
//   /*  80 */ natq t1;
//   /*  88 */ natq t2;
//   /*  96 */ natq s0;
//   /* 104 */ natq s1;
//   /* 112 */ natq a0;
//   /* 120 */ natq a1;
//   /* 128 */ natq a2;
//   /* 136 */ natq a3;
//   /* 144 */ natq a4;
//   /* 152 */ natq a5;
//   /* 160 */ natq a6;
//   /* 168 */ natq a7;
//   /* 176 */ natq s2;
//   /* 184 */ natq s3;
//   /* 192 */ natq s4;
//   /* 200 */ natq s5;
//   /* 208 */ natq s6;
//   /* 216 */ natq s7;
//   /* 224 */ natq s8;
//   /* 232 */ natq s9;
//   /* 240 */ natq s10;
//   /* 248 */ natq s11;
//   /* 256 */ natq t3;
//   /* 264 */ natq t4;
//   /* 272 */ natq t5;
//   /* 280 */ natq t6;
// };
