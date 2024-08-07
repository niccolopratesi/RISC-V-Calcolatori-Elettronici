.global kInterruptHandler
.global k_trap
k_trap:
    addi sp,sp,-8
    sd ra,0(sp)
    call salva_stato

    call kInterruptHandler

    call carica_stato

    ld ra,0(sp)
    addi sp,sp,8

    sret

.global sInterruptHandler
.global s_trap
s_trap:
    addi sp,sp,-8
    sd ra,0(sp)
    call salva_stato

    call sInterruptHandler

    call carica_stato

    ld ra,0(sp)
    addi sp,sp,8

    sret


# Supervisor interrupts are enabled/disabled
# by writing to the 2nd bit of sstatus
.global enableSInterrupts
enableSInterrupts:
    addi sp,sp,-8
    sd a0,0(sp)
    csrr a0, sstatus
    or a0,a0,2         # ...0010
    csrw sstatus, a0
    ld a0,0(sp)
    addi sp, sp, 8
    ret

# Supervisor interrupts are enabled/disabled
# by writing to the 2nd bit of sstatus
.global disableSInterrupts
disableSInterrupts:
    addi sp,sp,-16
    sd a0,0(sp)
    sd a1,8(sp)
    csrr a0, sstatus
    li a1,2         # ...00010
    not a1, a1      # ...11101
    and a0,a0,a1         
    csrw sstatus, a0
    ld a0,0(sp)
    ld a1,8(sp)
    addi sp, sp, 16
    ret
    

# Clear Sip
.global clearSSIP
clearSSIP:
    addi sp,sp,-16
    sd a0,0(sp)
    sd a1,8(sp)

    csrr a0, sip
    li a1, 2            
    not a1, a1          
    and a0,a0,a1         
    csrw sip, a0

    ld a0,0(sp)
    ld a1,8(sp)
    addi sp, sp, 16
    ret

.global schedule_next_timer_interrupt
schedule_next_timer_interrupt:
    csrr t0, stimecmp
    li t1, 500000
    add t0, t0, t1
    csrw stimecmp, t0
    ret
