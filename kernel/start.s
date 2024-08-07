.global k_trap
.equ TIMER_DELAY, 500000
.global start
start:
  # set M Previous mode to Supervisor for mret
  csrr a0, mstatus
  li a1, 0x1800
  xori a1, a1, -1
  and a0, a0, a1
  li a1, 0x802
  or a0, a0, a1
  csrw mstatus, a0

  # set M Exception Program Counter to boot_main
  # boot_main() defined in boot_main.c
  la a0, boot_main
  csrw mepc, a0
  # disable paging
  li a0, 0x0
  csrw satp, a0

  # Init interrupt ###

  # Disable current privilege interrupts
  li t0, 0b1010
  csrc mstatus, t0

  # Disable machine mode interrupts
  li t0, 0
  csrw mie, t0

  # delegate all interrupts and exceptions to supervisor mode
  li a0, 0xffff
  csrw medeleg, a0
  csrw mideleg, a0

  # Enable Supervisor Mode Interrupts
  li t0, 0b1000100010
  csrs sie, t0

  # Set k_trap as Supervisor interrupt handler
  la t0, k_trap
  csrw stvec, t0

  # Init memory ###

  # configure physical memory protection to access to all
  # physical Memory
  li a0, 0x3fffffffffffffULL
  csrw pmpaddr0, a0
  li a0, 0xf
  csrw pmpcfg0, a0

  # Enable memory access at S-mode when BIT_U=1 
  # Set the SUM bit (18) in sstatus 
  csrr a0, sstatus
  li a1, 0x40000
  or a0, a0, a1
  csrw sstatus, a0

  # Init timer #####
  
  # Enable Sstc extension
  li t0, 0x8000000000000000
  csrs menvcfg, t0

  # Schedule the first interrupt
  csrr t0, time       # Load time csr, which are cycles since boot
  li t1, TIMER_DELAY  # The delay at which to fire the int
  add t1, t1, t0      # Get next cycle at which to fire the int
  csrw stimecmp, t1   # Save the next cycle into the stimecmp csr

  # Enable time and stimecmp CSRs to be visibe in supervisor mode
  li t0, 0b10
  csrs mcounteren, t0

  mret
