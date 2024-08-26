// MACRO per gestione interruzioni
// Machine Status Register
#define MSTATUS_MPP_MASK (3L << 11) 
#define MSTATUS_MPP_M (3L << 11)
#define MSTATUS_MPP_S (1L << 11)
#define MSTATUS_MPP_U (0L << 11)
#define MSTATUS_MIE (1L << 3)   

// Supervisor Status Register
#define SSTATUS_SPP (1L << 8)   
#define SSTATUS_SPIE (1L << 5)   
#define SSTATUS_UPIE (1L << 4)   
#define SSTATUS_SIE (1L << 1)  
#define SSTATUS_UIE (1L << 0) 

extern "C" void s_trap();
extern "C" void k_trap();
extern "C" void disableSInterrupts();
extern "C" void enableSInterrupts();
extern "C" void sInterruptReturn();
extern "C" void clearSPreviousPrivilege();
extern "C" void clearSSIP();
extern "C" int  readSSIP();
extern "C" void setSPreviousInterruptEnable();
extern "C" void schedule_next_timer_interrupt();
