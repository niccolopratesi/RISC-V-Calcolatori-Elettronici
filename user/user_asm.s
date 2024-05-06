.global _start, start
_start:
start:
    call ctors
    call main
    ret

#############################################
# primitive chiamabili dal modulo utente	#
#############################################
# ( tipi delle primitive
#   ( comuni
.equ TIPO_A,			0x00	# activate_p
.equ TIPO_T,			0x01	# terminate_p
.equ TIPO_SI,			0x02	# sem_ini
.equ TIPO_W,			0x03	# sem_wait
.equ TIPO_S,			0x04	# sem_signal
.equ TIPO_D,			0x05	# delay
.equ TIPO_L,			0x06	# do_log
.equ TIPO_GMI,  		0x07	# getmeminfo (debug)

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

#     .global delay
# delay:
#     .cfi_startproc
#     li a7, TIPO_D 
#     ecall
#     ret
#     .cfi_endproc

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

