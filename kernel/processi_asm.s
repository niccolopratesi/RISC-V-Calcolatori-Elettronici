#  offset dei vari registri all'interno di des_proc
.equ LIVELLO, 2
.equ PUNT_NUCLEO, 8
.equ CTX, 16
.equ RA, CTX+0
.equ SP, CTX+8
.equ GP, CTX+16
.equ TP, CTX+24
.equ T0, CTX+32
.equ T1, CTX+40
.equ T2, CTX+48
.equ S0, CTX+56
.equ S1,  CTX+64
.equ A0,  CTX+72
.equ A1, CTX+80
.equ A2, CTX+88
.equ A3, CTX+96
.equ A4, CTX+104
.equ A5, CTX+112
.equ A6, CTX+120
.equ A7, CTX+128
.equ S2, CTX+136
.equ S3, CTX+144
.equ S4, CTX+152
.equ S5, CTX+160
.equ S6, CTX+168
.equ S7, CTX+176
.equ S8, CTX+184
.equ S9, CTX+192
.equ S10, CTX+200
.equ S11, CTX+208
.equ T3, CTX+216
.equ T4, CTX+224
.equ T5, CTX+232
.equ T6, CTX+240
.equ EPC, CTX+248		# In the x86 Arch, the RIP lies on the Stack. In RISC-V, we have to store it into our Context
.equ SATP, CTX+256	    # The equivalent of CR3: the address were the head of the page table resides
.equ SPIE, CTX+264

# Campi dei registri CSRs
.equ SSTATUS_SPP, (1L << 8)   
.equ SSTATUS_SPIE, (1L << 5)   
.equ SSTATUS_UPIE, (1L << 4)   
.equ SSTATUS_SIE, (1L << 1)  
.equ SSTATUS_UIE, (1L << 0) 

.global esecuzione
.global esecuzione_precedente
.global salva_stato
# copia lo stato dei registri generali nel des_proc del processo puntato da
# esecuzione.  Nessun registro viene sporcato.
# Necessita di una push sullo stack di ra prima di essere chiamata.
salva_stato:
	# salviamo lo stato di un paio di registri in modo da poterli
	# temporaneamente riutilizzare. In particolare, useremo a1 come
	# registro di lavoro e a0 come puntatore al des_proc.
	.cfi_startproc
	addi sp, sp, -16
	.cfi_def_cfa_offset 8
	sd a1, 0(sp)	# RAX
	.cfi_adjust_cfa_offset 8
	.cfi_offset a1, -8
	sd a0, 8(sp)	# RBX
	.cfi_adjust_cfa_offset 8
	.cfi_offset a0, -16

	la a1, esecuzione_precedente
	ld a0, esecuzione
	sd a0, 0(a1)

	# copiamo per primo il vecchio valore di a1
	ld a1, 0(sp)
	sd a1, A1(a0)
	# usiamo a1 come appoggio per copiare il vecchio a0
	ld a1, 8(sp)
	sd a1, A0(a0)
	# la funzione che ha invocato salva_stato ha salvato RA sullo stack, poiché sovrascritto nella chiamata a funzione
	# salviamo il valore che sp aveva prima della chiamata a salva stato
	mv a1, sp
	addi a1, a1, 16
	sd a1, SP(a0)
	# Il chiamante deve aver lasciato RA sullo stack,
	# dato che il registro verrebbe sovrascitto
	# dalla chiamata alla jal/call per salva_stato
	ld a1, 16(sp)
	sd a1, RA(a0)
	# copiamo gli altri registri
	#sd gp, GP(a0)
	sd tp, TP(a0)
	sd t0, T0(a0)
	sd t1, T1(a0)
	sd t2, T2(a0)
	sd s0, S0(a0)
	sd s1, S1(a0)
	sd a2, A2(a0)
	sd a3, A3(a0)
	sd a4, A4(a0)
	sd a5, A5(a0)
	sd a6, A6(a0)
	sd a7, A7(a0)
	sd s2, S2(a0)
	sd s3, S3(a0)
	sd s4, S4(a0)
	sd s5, S5(a0)
	sd s6, S6(a0)
	sd s7, S7(a0)
	sd s8, S8(a0)
	sd s9, S9(a0)
	sd s10, S10(a0)
	sd s11, S11(a0)
	sd t3, T3(a0)
	sd t4, T4(a0)
	sd t5, T5(a0)
	sd t6, T6(a0)
	# salviamo l'indirizzo di ritorno in EPC
	csrr a1, sepc
	sd a1, EPC(a0)

	# salviamo il valore di sstaus.SPIE
	csrr a1, sstatus
	andi a1, a1, SSTATUS_SPIE
	sw a1, SPIE(a0)

	# Ripristiniamo a1 e a0 dalla pila
	ld a1, 0(sp)
	.cfi_adjust_cfa_offset -8
	.cfi_restore a1
	ld a0, 8(sp)
	.cfi_adjust_cfa_offset -8
	.cfi_restore a0
	addi sp, sp, 16

	# se il processo gira a livello sistema non devo fare altro
	# se il processo gira a livello utente devo spostare sp alla pila sistema
	# sporco t0 e t1
	ld t0, esecuzione
	lh t1, LIVELLO(t0)
	bne t1, zero, 1f

	# CAMBIO
	# in x86 rsp salta automaticamente a tss_punt_nucleo, in RISC-V sp punta ancora alla pila utente del processo
	# salviamo a0 in sscratch per poterlo usare come puntatore al des_proc
	# carichiamo il puntatore alla pila del sistema in sp
	ld sp, PUNT_NUCLEO(t0)
1:
	ret # jr ra: Indirizzo di ritorno salvato in ra
	.cfi_endproc


.global carica_stato
#  carica nei registri del processore lo stato contenuto nel des_proc del
#  processo puntato da esecuzione.  Questa funzione sporca tutti i registri.
carica_stato:
	.cfi_startproc

	# Carica l'indirizzo del contesto in s0 per usarlo come base (preservato)
	ld s0, esecuzione # RBX

	# CAMBIO: Cambio paginazione
	ld a1, SATP(s0)
	csrr a2, satp
	# se satp e SATP nel des_proc sono lo stesso, non invalido il tlb
	beq a1, a2, 1f

	# Flush delle operazioni di memoria
	sfence.vma zero, zero
	# Cambio SATP
	csrw satp, a1
	# invalido il tlb
	sfence.vma zero, zero

1:
	# carica_stato si aspetta che sp punti alla pila sistema di esecuzione_precedente
	# Spostiamo sp alla base della pila sistema di esecuzione
	ld sp, PUNT_NUCLEO(s0) 

	# Se esecuzione gira a livello sistema, sopra a punt_nucleo c'è il RA salvato da salva_stato
	lh a1, LIVELLO(s0)
	beqz a1, 1f
	# decrementiamo sp di 8 per farlo puntare al RA salvato da salva_stato
	addi sp, sp, -8

1:
	# se il processo precedente era terminato o abortito la sua pila
	# sistema non era stata distrutta, in modo da permettere a noi di
	# continuare ad usarla. Ora che abbiamo cambiato pila possiamo
	# disfarci della precedente.
	# CAMBIO
	ld a1, ultimo_terminato
	beqz a1, 1f
	
	# Salviamo il contenuto attuale di RA per non farcelo sovrascrivere dalla call
	addi sp, sp, -8
	sd ra, 0(sp)
	# CAMBIO
	call distruggi_pila_precedente
	# Ripristiniamo RA
	ld ra, 0(sp)
	addi sp, sp, 8

1:

	### in x86 alcune informazioni sono salvate in pila e gestite dalle iretq, in risc-v vanno aggiornate manualmente
	
	## Carichiamo in sepc l'indirizzo di ritorno salvato nel des_proc
	ld a1, EPC(s0)
	csrw sepc, a1
	
	## Settiamo sstaus.spp a 0 se il processo è utente, 1 se è sistema
	## Settiamo stvec a s_trap se il processo è utente, a k_trap se è sistema
	lh a1, LIVELLO(s0)
	# Salviamo il contenuto attuale di RA per non farcelo sovrascrivere dalla call
	addi sp, sp, -8
	sd ra, 0(sp)
	# se a1 = 0 (utente => clear SPP, s_trap)
	beqz a1, 1f
	# se a1 = 1 (sistema => set SPP, k_trap)
	call setSPreviousPrivilege
	la a0, k_trap
	csrw stvec, a0
	j 2f
1:
	# livello utente
	call clearSPreviousPrivilege
	la a0, s_trap
	csrw stvec, a0
2:	
	# Ripristiniamo RA
	ld ra, 0(sp)
	addi sp, sp, 8


	## Settiamo sstatus.spie al valore salvato nel des_proc
	lw a1, SPIE(s0)
	# Salviamo il contenuto attuale di RA per non farcelo sovrascrivere dalla call
	addi sp, sp, -8
	sd ra, 0(sp)
	# se a1 = 0 (interrupt disabilitati => clear SPIE)
	beqz a1, 1f
	# se a1 = 1 (interrupt abilitati => set SPIE)
	call setSPreviousInterruptEnable
	j 2f
1:
	call clearSPreviousInterruptEnable
2:	
	# Ripristiniamo RA
	ld ra, 0(sp)
	addi sp, sp, 8


	## In Risc-V non serve aggiornare tss_punt_nucleo, in quanto dopo un'intr 
	# salva_stato sposta manualmente sp a punt_nucleo

	# Ripristiniamo i registri
	#ld gp, GP(s0)
	ld tp, TP(s0)
	ld t0, T0(s0)
	ld t1, T1(s0)
	ld t2, T2(s0)
	ld s1, S1(s0)
	ld a0, A0(s0)
	ld a1, A1(s0)
	ld a2, A2(s0)
	ld a3, A3(s0)
	ld a4, A4(s0)
	ld a5, A5(s0)
	ld a6, A6(s0)
	ld a7, A7(s0)
	ld s2, S2(s0)
	ld s3, S3(s0)
	ld s4, S4(s0)
	ld s5, S5(s0)
	ld s6, S6(s0)
	ld s7, S7(s0)
	ld s8, S8(s0)
	ld s9, S9(s0)
	ld s10, S10(s0)
	ld s11, S11(s0)
	ld t3, T3(s0)
	ld t4, T4(s0)
	ld t5, T5(s0)
	ld t6, T6(s0)
	
	# Nel contesto, sp punta a RA (che è stato salvato in pila) a prescindere dal livello di esecuzione
	ld sp, SP(s0)

	# Ripristiniamo s0
	ld s0, S0(s0)

	ret 
	.cfi_endproc

.global salta_a_main
salta_a_main:
	.cfi_startproc
	call carica_stato
	sret
	.cfi_endproc

# Clear S Previous Privilege mode (User mode)
.global clearSPreviousPrivilege
clearSPreviousPrivilege:
    addi sp,sp,-16
    sd a0,0(sp)
    sd a1,8(sp)
    csrr a0, sstatus
    li a1, SSTATUS_SPP  # ...0100000000
    not a1, a1          # ...1011111111
    and a0,a0,a1         
    csrw sstatus, a0
    ld a0,0(sp)
	ld a1,8(sp)
	addi sp,sp,16
    ret

# Set S Previous Privilege mode (Supervisor mode)
.global setSPreviousPrivilege
setSPreviousPrivilege:
    addi sp,sp,-8
    sd a0,0(sp)
    csrr a0, sstatus
    or a0, a0, SSTATUS_SPP           # ...0100000000      
    csrw sstatus, a0
    ld a0,0(sp)
	addi sp,sp,8
    ret

# Clear S Previous Interrupt Enable
.global clearSPreviousInterruptEnable
clearSPreviousInterruptEnable:
  	addi sp,sp,-16
    sd a0,0(sp)
    sd a1,8(sp)
    csrr a0, sstatus
    li a1, SSTATUS_SPIE           	# ...00100000
    not a1, a1          # ...1101111111
    and a0,a0,a1         
    csrw sstatus, a0
    ld a0,0(sp)
	ld a1,8(sp)
	addi sp,sp,16
    ret

# Set S Previous Interrupt Enable
.global setSPreviousInterruptEnable
setSPreviousInterruptEnable:
    addi sp,sp,-8
    sd a0,0(sp)
    csrr a0, sstatus
    or a0,a0, SSTATUS_SPIE         # ...00100000
    csrw sstatus, a0
    ld a0,0(sp)
	addi sp,sp,8
    ret

.global halt # CAMBIO
halt:
    wfi
    j halt
    ret

    .global end_program
end_program:
	call reboot
	call halt
	ret
	
