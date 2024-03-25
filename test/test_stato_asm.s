#  offset dei vari registri all'interno di des_proc
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
	sd gp, GP(a0)
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

	# Ripristiniamo a1 e a0 dalla pila
	ld a1, 0(sp)
	.cfi_adjust_cfa_offset -8
	.cfi_restore a1
	ld a0, 8(sp)
	.cfi_adjust_cfa_offset -8
	.cfi_restore a0
	addi sp, sp, 16

	ret # jr ra: Indirizzo di ritorno salvato in ra
	.cfi_endproc


.global carica_stato
#  carica nei registri del processore lo stato contenuto nel des_proc del
#  processo puntato da esecuzione.  Questa funzione sporca tutti i registri.
carica_stato:
	.cfi_startproc

	# Carica l'indirizzo del contesto in a0 per usarlo come base
	ld a0, esecuzione # RBX

	# TODO: Cambio paginazione
1:

	# Ripristiniamo pila vecchia
	ld sp, SP(a0) 

	# se il processo precedente era terminato o abortito la sua pila
	# sistema non era stata distrutta, in modo da permettere a noi di
	# continuare ad usarla. Ora che abbiamo cambiato pila possiamo
	# disfarci della precedente.
	# SKIP_TESTING
	# ld a1, ultimo_terminato
	# beqz a1, 1f
	# Salviamo il contenuto attuale di RA per non farcelo sovrascrivere dalla call
	addi sp, sp, -8
	sd ra, 0(sp)
	# SKIP_TESTING
	# call distruggi_pila_precedente
	# Ripristiniamo RA
	ld ra, 0(sp)
	addi sp, sp, 8

1:

	# SKIP_TESTING
	# aggiorniamo il puntatore alla pila sistema usata dal meccanismo
	# delle interruzioni
	# ld a1, PUNT_NUCLEO(a0)
	# sd a1, tss_punt_nucleo

	# Ripristiniamo i registri
	ld gp, GP(a0)
	ld tp, TP(a0)
	ld t0, T0(a0)
	ld t1, T1(a0)
	ld t2, T2(a0)
	ld s0, S0(a0)
	ld s1, S1(a0)
	ld a2, A2(a0)
	ld a3, A3(a0)
	ld a4, A4(a0)
	ld a5, A5(a0)
	ld a6, A6(a0)
	ld a7, A7(a0)
	ld s2, S2(a0)
	ld s3, S3(a0)
	ld s4, S4(a0)
	ld s5, S5(a0)
	ld s6, S6(a0)
	ld s7, S7(a0)
	ld s8, S8(a0)
	ld s9, S9(a0)
	ld s10, S10(a0)
	ld s11, S11(a0)
	ld t3, T3(a0)
	ld t4, T4(a0)
	ld t5, T5(a0)
	ld t6, T6(a0)
	# TEST:
	ld t5, RA(a0)
	# Ripristiniamo a0 e a1
	ld a1, A1(a0)
	ld a0, A0(a0)

	ret 
	.cfi_endproc

.global test_stato_asm
test_stato_asm:
	addi sp,sp,-8
	sd ra,0(sp)
	call salva_stato
	call salva_success
	call carica_stato
	call carica_success
	ld ra,0(sp)
	addi sp,sp,8
	ret
