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

# Supervisor interrupts are enabled/disabled
# by writing to the 2nd bit of sstatus
.global enableSInterrupts
enableSInterrupts:
    csrsi sstatus, 0b10
    ret

# Supervisor interrupts are enabled/disabled
# by writing to the 2nd bit of sstatus
.global disableSInterrupts
disableSInterrupts:
    csrci sstatus, 0b10
    ret
    

# Clear Sip
.global clearSSIP
clearSSIP:
    csrci sip, 0b10
    ret

.global schedule_next_timer_interrupt
schedule_next_timer_interrupt:
    csrr t0, stimecmp
    li t1, 500000
    add t0, t0, t1
    csrw stimecmp, t0
    ret
