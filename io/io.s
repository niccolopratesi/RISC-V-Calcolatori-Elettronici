#/// @file io.s
#/// @brief Parte Assembler del modulo I/O


.text
.global _start, start 

_start:
start:

    call ctors
    call main
    ret




#############################################
# primitive chiamabili dal modulo io	    #
#############################################
# ( tipi delle primitive
.equ TIPO_A,			0x00	# activate_p
.equ TIPO_T,			0x01	# terminate_p
.equ TIPO_SI,			0x02	# sem_ini
.equ TIPO_W,			0x03	# sem_wait
.equ TIPO_S,			0x04	# sem_signal
.equ TIPO_D,			0x05	# delay
.equ TIPO_L,			0x06	# do_log
.equ TIPO_GMI,  		0x07	# getmeminfo (debug)

.equ TIPO_APE,		    0x30    #activate_pe
.equ TIPO_WFI,	    	0x31	#wfi
#.equ TIPO_FG,			0x32	#fill_gate
.equ TIPO_AB,			0x33	#abort_p
.equ TIPO_IOP,		    0x34	#io_panic
.equ TIPO_TRA,		    0x35	#trasforma
.equ TIPO_ACC,		    0x36	#access

.equ IO_TIPO_RCON,		0x44	#readconsole
.equ IO_TIPO_WCON,		0x45	#writeconsole
.equ IO_TIPO_INIC,		0x46	#iniconsole
# }

#///////////////////////////////////////////////////////////////////////////////////
#//                         CHIAMATE DI SISTEMA GENERICHE                         //
#///////////////////////////////////////////////////////////////////////////////////


    .global activate_p
activate_p:
	.cfi_startproc
	li a7, TIPO_A 
    ecall
	ret
	.cfi_endproc

    .global terminate_p
terminate_p:
	.cfi_startproc
	li a7, TIPO_T 
    ecall
	ret
	.cfi_endproc

	.global sem_ini
sem_ini:
	.cfi_startproc
	li a7, TIPO_SI 
    ecall
	ret
	.cfi_endproc

	.global sem_wait
sem_wait:
	.cfi_startproc
	li a7, TIPO_W 
    ecall
	ret
	.cfi_endproc

    .global sem_signal
sem_signal:
    .cfi_startproc
    li a7, TIPO_S 
    ecall
    ret
    .cfi_endproc

    .global delay
delay:
    .cfi_startproc
    li a7, TIPO_D 
    ecall
    ret
    .cfi_endproc

    .global do_log
do_log:
    .cfi_startproc
    li a7, TIPO_L 
    ecall
    ret
    .cfi_endproc

#     .global getmeminfo
# getmeminfo:
#     .cfi_startproc
#     li a7, TIPO_GMI 
#     ecall
#     ret
#     .cfi_endproc

#///////////////////////////////////////////////////////////////////////////////////
#//                CHIAMATE DI SISTEMA  SPECIFICHE PER IL MODULO IO               //
#///////////////////////////////////////////////////////////////////////////////////


    .global activate_pe
activate_pe:
    .cfi_startproc
    li a7, TIPO_APE
    ecall
    ret
    .cfi_endproc

    .global wfi
wfi:
    .cfi_startproc
    li a7, TIPO_WFI
    ecall
    ret
    .cfi_endproc

    .global abort_p
abort_p:
    .cfi_startproc
    li a7, TIPO_AB
    ecall
    ret
    .cfi_endproc

    .global io_panic
io_panic:
    .cfi_startproc
    li a7, TIPO_IOP
    ecall
    ret
    .cfi_endproc

    .global trasforma
trasforma:
    .cfi_startproc
    li a7, TIPO_TRA
    ecall
    ret
    .cfi_endproc

    .global access
access:
    .cfi_startproc
    li a7, TIPO_ACC
    ecall
    ret
    .cfi_endproc



#///////////////////////////////////////////////////////////////////////////////////
#//                               PRIMITIVE DI IO                                 //
#///////////////////////////////////////////////////////////////////////////////////

    .extern c_iniconsole
a_iniconsole:
    .cfi_startproc

    addi sp,sp,-8
    sd ra,0(sp)

    call c_iniconsole

    ld ra,0(sp)
    addi sp,sp,8
    
    sret
    .cfi_endproc

    .extern c_readconsole
a_readconsole:
    .cfi_startproc

    addi sp,sp,-8
    sd ra,0(sp)

    call c_readconsole

    ld ra,0(sp)
    addi sp,sp,8
    
    sret
    .cfi_endproc


    .extern c_writeconsole
a_writeconsole:
    .cfi_startproc

    addi sp,sp,-8
    sd ra,0(sp)

    call c_writeconsole

    ld ra,0(sp)
    addi sp,sp,8
    
    sret
    .cfi_endproc
