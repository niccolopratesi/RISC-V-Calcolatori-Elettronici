.global test_ctors_asm
test_ctors_asm:
	# Il C++ prevede che si possa eseguire del codice prima di main (per
	# es. nei costruttori degli oggetti globali). gcc organizza questo
	# codice in una serie di funzioni di cui raccoglie i puntatori
	# nell'array __init_array_start. Il C++ run time deve poi chiamare
	# tutte queste funzioni prima di saltare a main.  Poiche' abbiamo
	# compilato il modulo con -nostdlib dobbiamo provvedere da soli a
	# chiamare queste funzioni, altrimenti gli oggetti globali non saranno
	# inizializzati correttamente.
    addi sp, sp, -36
	sd a0, 0(sp) # RBX
	sd a1, 8(sp) # R12
    sd a2, 16(sp)
    sd ra, 24(sp)
	ld a0, __init_array_start
	ld a1, __init_array_end
1:	beq a0, a1, 2f
    ld a2, 0(a0)
	jalr a2 
	addi a0, a0, 8
	j 1b
2:	ld a0, 0(sp) # RBX
	ld a1, 8(sp) # R12
    ld a2, 16(sp)
    ld ra, 24(sp)
    addi sp, sp, 36
	ret
