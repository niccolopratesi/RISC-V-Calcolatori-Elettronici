.global ctors
ctors:
# Il C++ prevede che si possa eseguire del codice prima di main (per
	# es. nei costruttori degli oggetti globali). gcc organizza questo
	# codice in una serie di funzioni di cui raccoglie i puntatori
	# nell'array __init_array_start. Il C++ run time deve poi chiamare
	# tutte queste funzioni prima di saltare a main.  Poiche' abbiamo
	# compilato il modulo con -nostdlib dobbiamo provvedere da soli a
	# chiamare queste funzioni, altrimenti gli oggetti globali non saranno
	# inizializzati correttamente.
    addi sp, sp, -36
	sd s0, 0(sp) # RBX
	sd s1, 8(sp) # R12
    sd s2, 16(sp)
    sd ra, 24(sp)
	la s0, __init_array_start
	la s1, __init_array_end
1:	beq s0, s1, 2f
    ld s2, 0(s0)
	jalr s2 
	addi s0, s0, 8
	j 1b
2:	ld s0, 0(sp) # RBX
	ld s1, 8(sp) # R12
    ld s2, 16(sp)
    ld ra, 24(sp)
    addi sp, sp, 36
	ret
	