#include "internal.h"
#include "tipo.h"
#include "vm.h"

// The type of page table is defined by the 4 most significant bit of SATP.
// Our system uses a virtual address space of 48 bits, so we set the SATP
// to mode Sv48(9). Link for reference: https://riscv.org/wp-content/uploads/2017/05/riscv-privileged-v1.10.pdf (Page 60)
#define SATP_SV48 (9L << 60)
#define MAKE_SATP(pagetable) (SATP_SV48 | (((paddr)pagetable) >> 12))

void writeSATP(paddr addr){
    writeSATP_asm(MAKE_SATP(addr));
    invalida_TLB();
}
