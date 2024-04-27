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
