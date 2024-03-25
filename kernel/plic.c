#include "plic.h"
#include "tipo.h"
#include "types.h"
#include "uart.h"

void plic_init() {
    // Set desired IRQ priorities non-zero (otherwise disabled)
    *(uint32*)(PLIC + UART0_IRQ*4) = 1;

    *(uint32*)PLIC_ENABLE = (1 << UART0_IRQ);

    *(uint32*)PLIC_THRESHOLD = 0;

    boot_printf("PLIC Initialized\n");

}

int plic_claim() {
    int irq = *(uint32*)PLIC_CLAIM;
    return irq;
}

void plic_complete(int irq) {
    *(uint32*)PLIC_CLAIM = irq;

    //boot_printf("PLIC Completed\n");
}






