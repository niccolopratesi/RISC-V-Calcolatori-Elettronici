# Lettura
.global readSEPC
readSEPC:
    csrr a0, sepc
    ret

.global readSCAUSE
readSCAUSE:
    csrr a0, scause
    ret

.global readSSTATUS
readSSTATUS:
    csrr a0, sstatus
    ret

.global readSSIP
readSSIP:
    csrr a0, sip
    ret

.global readSTVEC
readSTVEC:
    csrr a0, stvec
    ret

.global readSATP
readSATP:
    csrr a0, satp
    ret

# readCR2 legge il contenuto di CR2, che in un'architettura
# x86 contiene l'indirizzo virtuale che era in fase di traduzione.
# Sfruttiamo questo registro durante un PageFault.
# In RISCV, l'indirizzo colpevole viene salvato nel registro stval.
.global readSTVAL
readSTVAL:
	.cfi_startproc
	csrr a0, stval
	ret
	.cfi_endproc

# Scrittura
.global writeSEPC
writeSEPC:
    csrw sepc, a0
    ret

.global writeSSTATUS
writeSSTATUS:
    csrw sstatus, a0
    ret

.global writeSTVEC
writeSTVEC:
    csrw stvec, a0
    ret

.global writeSATP_asm
writeSATP_asm:
    csrw satp, a0
    ret
    