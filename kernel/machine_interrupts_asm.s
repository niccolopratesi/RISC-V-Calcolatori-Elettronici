.global machine_interrupts
.align 2
machine_interrupts:
  call machine_handler
  mret
