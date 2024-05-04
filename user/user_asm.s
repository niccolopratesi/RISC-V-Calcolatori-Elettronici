.global _start, start
_start:
start:
	nop
    nop
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

