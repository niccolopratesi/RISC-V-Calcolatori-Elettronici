//#include "libce.h"
#include "tipo.h"
#include "plic.h"
#include "uart.h"

void plic_init() {
    // Set desired IRQ priorities non-zero (otherwise disabled)
    // *(natl*)(PLIC + UART0_IRQ*4) = 1;

    // *(natl*)PLIC_ENABLE = (1 << UART0_IRQ);

    // *(natl*)PLIC_THRESHOLD = 0;
}

int plic_claim() {
    int irq = *(natl*)PLIC_CLAIM;
    return irq;
}

void plic_complete(int irq) {
    *(natl*)PLIC_CLAIM = irq;
}






