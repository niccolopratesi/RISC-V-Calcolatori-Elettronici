#include "tipo.h"

// Saved registers for kernel context switches.
struct context {
  natq ra;
  natq sp;

  // callee-saved
  natq s0;
  natq s1;
  natq s2;
  natq s3;
  natq s4;
  natq s5;
  natq s6;
  natq s7;
  natq s8;
  natq s9;
  natq s10;
  natq s11;
};

// per-process data for the trap handling code in trampoline.S.
// sits in a page by itself just under the trampoline page in the
// user page table. not specially mapped in the kernel page table.
// uservec in trampoline.S saves user registers in the trapframe,
// then initializes registers from the trapframe's
// kernel_sp, kernel_hartid, kernel_satp, and jumps to kernel_trap.
// usertrapret() and userret in trampoline.S set up
// the trapframe's kernel_*, restore user registers from the
// trapframe, switch to the user page table, and enter user space.
// the trapframe includes callee-saved user registers like s0-s11 because the
// return-to-user path via usertrapret() doesn't return through
// the entire kernel call stack.
struct trapframe {
  /*   0 */ natq kernel_satp;   // kernel page table
  /*   8 */ natq kernel_sp;     // top of process's kernel stack
  /*  16 */ natq kernel_trap;   // usertrap()
  /*  24 */ natq epc;           // saved user program counter
  /*  40 */ natq ra;
  /*  48 */ natq sp;
  /*  56 */ natq gp;
  /*  64 */ natq tp;
  /*  72 */ natq t0;
  /*  80 */ natq t1;
  /*  88 */ natq t2;
  /*  96 */ natq s0;
  /* 104 */ natq s1;
  /* 112 */ natq a0;
  /* 120 */ natq a1;
  /* 128 */ natq a2;
  /* 136 */ natq a3;
  /* 144 */ natq a4;
  /* 152 */ natq a5;
  /* 160 */ natq a6;
  /* 168 */ natq a7;
  /* 176 */ natq s2;
  /* 184 */ natq s3;
  /* 192 */ natq s4;
  /* 200 */ natq s5;
  /* 208 */ natq s6;
  /* 216 */ natq s7;
  /* 224 */ natq s8;
  /* 232 */ natq s9;
  /* 240 */ natq s10;
  /* 248 */ natq s11;
  /* 256 */ natq t3;
  /* 264 */ natq t4;
  /* 272 */ natq t5;
  /* 280 */ natq t6;
};
