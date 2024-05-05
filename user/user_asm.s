.global _start, start
.global main
_start:
start:
    call ctors
    call main
    ret

.globl func
func:
    nop
    nop
    nop
    ret

.global u_activate_p
u_activate_p:
    li a7, 0
    ecall
    ret

.global u_terminate_p
u_terminate_p:
    li a7, 1
    ecall
    ret

.global do_log
do_log:
    li a7, 2
    ecall
    ret
