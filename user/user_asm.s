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
.equ TIPO_GMI,  	0x07	# getmeminfo (debug)

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
#   )

#   ( fornite dal modulo I/O
.equ IO_TIPO_HDR,     0x40
.equ IO_TIPO_HDW,     0x41
.equ IO_TIPO_DMAHDR,  0x42
.equ IO_TIPO_DMAHDW,  0x43
.equ IO_TIPO_RCON,    0x44
.equ IO_TIPO_WCON,    0x45
.equ IO_TIPO_INIC,    0x46
.equ IO_TIPO_GMI,     0x47

    .global readconsole
readconsole:
    .cfi_startproc
    li a7, IO_TIPO_RCON
    ecall
    ret
    .cfi_endproc

    .global writeconsole
writeconsole:
    .cfi_startproc
    li a7, IO_TIPO_WCON
    ecall
    ret
    .cfi_endproc

    .global iniconsole 
iniconsole:
    .cfi_startproc
    li a7, IO_TIPO_INIC
    ecall
    ret
    .cfi_endproc
#   )
# )
