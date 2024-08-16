/// @file io.s
/// @brief Parte Assembler del modulo I/O

#include <libce.h>
#include <costanti.h>

.text
.global _start, start 

_start:
start:

    call ctors
    call main
    ret



///////////////////////////////////////////////////////////////////////////////////
//                         CHIAMATE DI SISTEMA GENERICHE                         //
///////////////////////////////////////////////////////////////////////////////////


    .global activate_p
activate_p:
	.cfi_startproc
	li a7, $TIPO_A 
    ecall
	ret
	.cfi_endproc

    .global terminate_p
terminate_p:
	.cfi_startproc
	li a7, $TIPO_T 
    ecall
	ret
	.cfi_endproc

	.global sem_ini
sem_ini:
	.cfi_startproc
	li a7, $TIPO_SI 
    ecall
	ret
	.cfi_endproc

	.global sem_wait
sem_wait:
	.cfi_startproc
	li a7, $TIPO_W 
    ecall
	ret
	.cfi_endproc

    .global sem_signal
sem_signal:
    .cfi_startproc
    li a7, $TIPO_S 
    ecall
    ret
    .cfi_endproc

    .global delay
delay:
    .cfi_startproc
    li a7, $TIPO_D 
    ecall
    ret
    .cfi_endproc

    .global do_log
do_log:
    .cfi_startproc
    li a7, $TIPO_L 
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

///////////////////////////////////////////////////////////////////////////////////
//                CHIAMATE DI SISTEMA  SPECIFICHE PER IL MODULO IO               //
///////////////////////////////////////////////////////////////////////////////////


    .global activate_pe
activate_pe:
    .cfi_startproc
    li a7, $TIPO_APE
    ecall
    ret
    .cfi_endproc

    .global wfi
wfi:
    .cfi_startproc
    li a7, $TIPO_WFI
    ecall
    ret
    .cfi_endproc

    .global abort_p
abort_p:
    .cfi_startproc
    li a7, $TIPO_AB
    ecall
    ret
    .cfi_endproc

    .global io_panic
io_panic:
    .cfi_startproc
    li a7, $TIPO_IOP
    ecall
    ret
    .cfi_endproc

    .global fill_gate
fill_gate:
    .cfi_startproc
    li a7, $TIPO_FG
    ecall
    ret
    .cfi_endproc

    .global trasforma
trasforma:
    .cfi_startproc
    li a7, $TIPO_TRA
    ecall
    ret
    .cfi_endproc

    .global access
access:
    .cfi_startproc
    li a7, $TIPO_ACC
    ecall
    ret
    .cfi_endproc



///////////////////////////////////////////////////////////////////////////////////
//                               PRIMITIVE DI IO                                 //
///////////////////////////////////////////////////////////////////////////////////

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
