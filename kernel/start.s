.globl timer_machine_handler
.equ TIMER_DELAY, 10000000
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

  # delegate all interrupts and exceptions to supervisor mode
  li a0, 0xffff
  csrw medeleg, a0
  csrw mideleg, a0

  # configure physical memory protection to access to all
  # physical Memory
  li a0, 0x3fffffffffffffULL
  csrw pmpaddr0, a0
  li a0, 0xf
  csrw pmpcfg0, a0

  # Init timer #####
  
  # Schedule the first interrupt
  li a0, 0x200BFF8    # MTIME mem location
  ld a1, 0(a0)        # Load MTIME, which are cycles since boot
  li a2, TIMER_DELAY  # The delay at which to fire the int
  add a2, a2, a1      # Get next cycle at which to fire the int
  li a0, 0x2004000    # MTIMECMP mem location
  sd a2, 0(a0)

  # Set timer_machine_handler as the machine interrupt handler
  la t0, timer_machine_handler
  csrw mtvec, t0

  # Enable machine mode interrupts
  li t0, 8
  csrs mstatus, t0

  # Enable timer interrupts
  li t0, 128
  csrw mie, t0

  # Set k_trap as Supervisor interrupt handler
  la t0, k_trap
  csrw stvec, t0

  # Enable Supervisor Mode Interrupts
  csrr a0, sie
  ori a0, a0, 0x200
  ori a0, a0, 0x20
  ori a0, a0, 0x2
  csrw sie, a0

  # Enable memory access at S-mode when BIT_U=1 
  # Set the SUM bit (18) in sstatus 
  csrr a0, sstatus
  li a1, 0x40000
  or a0, a0, a1
  csrw sstatus, a0

  mret
