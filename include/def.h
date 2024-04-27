#define ID_CODE_SYS		1
#define ID_DATA_SYS		0
#define ID_CODE_USR		2
#define ID_DATA_USR		3
#define ID_SYS_TSS		4
#define SEL_CODICE_SISTEMA	(ID_CODE_SYS << 3)
#define SEL_DATI_SISTEMA 	(ID_DATA_SYS << 3)
#define SEL_CODICE_UTENTE	(ID_CODE_USR << 3)
#define SEL_DATI_UTENTE 	(ID_DATA_USR << 3)
#define SEL_SYS_TSS 		(ID_SYS_TSS  << 3)
#define LIV_UTENTE		    0
#define LIV_SISTEMA         1
#define LIV_MACCHINA        3

#define SERV_PCIBIOS32		0x49435024 // $PCI
#define SERV_PCIIRQRT		0x52495024 // $PIR

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