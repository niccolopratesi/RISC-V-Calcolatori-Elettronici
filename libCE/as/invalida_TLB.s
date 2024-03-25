.global invalida_TLB
invalida_TLB:
    sfence.vma zero, zero
    ret
    