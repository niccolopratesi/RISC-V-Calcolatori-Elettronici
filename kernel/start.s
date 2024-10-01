.equ TIMER_DELAY, 500000

.global start
start:
  # Init interrupt ###

  # Disable current privilege interrupts
  # csrci mstatus, 0b1010

  # Disable machine mode interrupts
  li t0, 0
  # csrw mie, t0

  # delegate all interrupts and exceptions to supervisor mode
  li t0, 0xffffffffffffffff
  csrw medeleg, t0
  csrw mideleg, t0

  # set machine_interrupts as Machine interrupt handler
  la t0, machine_interrupts
  csrw mtvec, t0

  # Enable Supervisor Mode Interrupts
  li t0, 0b1000100010
  csrs sie, t0

  # Set k_trap as Supervisor interrupt handler
  la t0, k_trap
  csrw stvec, t0

  # Enable MSI (PCI) interrupts
  li t0, 0x70   # EIDELIVERY
  csrw siselect, t0
  li t1, 1
  csrw sireg, t1
  li t0, 0x72   # EITHRESHOLD
  csrw siselect, t0
  li t1, 0xFFFF
  csrw sireg, t1
  # Keyboard specific
  li t0, 0xC0   # EIE0
  csrw siselect, t0
  li t1, 0xFFFFFFFF
  csrw sireg, t1

  # Init memory ###

  # configure physical memory protection to access to all
  # physical Memory
  li t0, 0x3fffffffffffff
  csrw pmpaddr0, t0
  li t0, 0xf
  csrw pmpcfg0, t0

  # Enable memory access at S-mode when BIT_U=1 
  # Set the SUM bit (18) in sstatus 
  li t0, 0x40000
  csrs sstatus, t0

  # Init timer ###
  
  # Enable Sstc extension
  li t0, 0x8000000000000000
  csrs menvcfg, t0

  # Schedule the first interrupt
  csrr t0, time       # Load time csr, which are cycles since boot
  li t1, TIMER_DELAY  # The delay at which to fire the int
  add t1, t1, t0      # Get next cycle at which to fire the int
  csrw stimecmp, t1   # Save the next cycle into the stimecmp csr

  # Enable time and stimecmp CSRs to be visibe in supervisor mode
  csrsi mcounteren, 0b10

  # Give control to boot_main() ###

  # clear M previous interrupt enable bit
  li t0, 0x80
  csrc mstatus, t0

  # set M Previous mode to Supervisor for mret
  li t0, 0x1000
  csrc mstatus, t0
  li t0, 0x800
  csrs mstatus, t0

  # set M Exception Program Counter to boot_main
  # boot_main() defined in boot_main.c
  la t0, boot_main
  csrw mepc, t0
  # disable paging
  li t0, 0x0
  csrw satp, t0

  mret
