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
    addi sp,sp,8

    call sInterruptHandler


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
    ret
    

# Clear S Previous Privilege mode (User mode)
.global clearSPreviousPrivilege
clearSPreviousPrivilege:
    addi sp,sp,-16
    sd a0,0(sp)
    sd a1,8(sp)
    csrr a0, sstatus
    li a1,256           # ...0100000000
    not a1, a1          # ...1011111111
    and a0,a0,a1         
    csrw sstatus, a0
    ld a0,0(sp)
    ret

# Set S Previous Interrupt Enable
.global setSPreviousInterruptEnable
setSPreviousInterruptEnable:
    addi sp,sp,-8
    sd a0,0(sp)
    csrr a0, sstatus
    or a0,a0,32         # ...00100000
    csrw sstatus, a0
    ld a0,0(sp)
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


