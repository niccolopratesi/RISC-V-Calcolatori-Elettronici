.equ TIMER_DELAY, 59659
.global timer_debug
.global timer_machine_handler
.align 4
timer_machine_handler:

    # We need to use some registers, so we push them to the stack
    addi sp, sp, -24
    sd a0, 0(sp)
    sd a1, 8(sp)
    sd a2, 16(sp)

    # We schedule the next timer interrupt by reading the old MTIMECMP
    li a0, 0x2004000        # MTIMECMP mem location
    ld a1, 0(a0)            # Load old MTIMECMP
    li a2, TIMER_DELAY      # The delay at which to fire the int
    add a2, a2, a1          # Get next cycle at which to fire the int
    sd a2, 0(a0)

    # TEST:
    # Notify Supervisor Mode that a timer int fired
    li a1, 2
    csrw sip, a1

    # We restore the registers
    ld a0, 0(sp)
    ld a1, 8(sp)
    ld a2, 16(sp)
    addi sp, sp, 24
    
    mret



