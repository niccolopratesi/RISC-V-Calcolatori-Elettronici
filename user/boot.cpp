extern "C" void main (natl magic, multiboot_info_t* mbi)
{
	natl entry;
	boot64_info tmp_info;
	char *cmdline = nullptr;
	paddr mem_lower = 640*KiB;
	paddr mem_tot = 8*MiB;

	// inizializzazione della porta seriale per il debugging
	serial::init1();
	flog(LOG_INFO, "Boot loader di Calcolatori Elettronici, v1.0");

	// controlliamo di essere stati caricati da un bootloader che rispetti
	// lo standard multiboot
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		flog(LOG_ERR, "Numero magico non valido: %#x", magic);
		return;
	}

	// se il primo bootloader ci ha passato informazioni sulla memoria, la usiamo
	// per costruire correttamente la finestra FM (altrimenti usiamo valori di default)
	if (mbi->flags & MULTIBOOT_INFO_MEMORY) {
		mem_lower = (mbi->mem_lower + 1) * KiB;
		mem_tot = (mbi->mem_upper + 0x480) * KiB;
	}
	flog(LOG_INFO, "Memoria totale: %lld MiB, heap: %lld KiB", mem_tot/MiB, (mem_lower - DIM_PAGINA)/KiB);

	// se il primo bootloader ci ha passato una stringa di opzioni, proviamo a esaminarla
	if (mbi->flags & MULTIBOOT_INFO_CMDLINE) {
		cmdline = ptr_cast<char>(mbi->cmdline);
		flog(LOG_INFO, "Argomenti: %s", cmdline);
		parse_args(cmdline);
	}

	// Il numero dei moduli viene passato dal primo bootloader in mods_count
	// Abbiamo bisogno di almeno un modulo.
	if (!(mbi->flags & MULTIBOOT_INFO_MODS) || mbi->mods_count < 1)
	{
		flog(LOG_ERR, "Informazioni sui moduli assenti o errate");
		return;
	}

	// raccogliamo le informazioni che ci servono su tutti i moduli
	memset(&tmp_info, 0, sizeof(tmp_info));
	flog(LOG_INFO, "Il boot loader precedente ha caricato %d modul%s:", mbi->mods_count,
			mbi->mods_count > 1 ? "i" : "o");
	multiboot_module_t* mod = ptr_cast<multiboot_module_t>(mbi->mods_addr);
	for (unsigned int i = 0; i < mbi->mods_count; i++) {
		flog(LOG_INFO, "- mod[%d]: start=%x end=%x file=%s",
			i, mod[i].mod_start, mod[i].mod_end, ptr_cast<const char>(mod[i].cmdline));
		// ignoriamo quelli oltre MAX_MODULES
		if (i < MAX_MODULES) {
			tmp_info.mod[i].mod_start = mod[i].mod_start;
			tmp_info.mod[i].mod_end = mod[i].mod_end;
		}
	}
	// e carichiamo il primo
	flog(LOG_INFO, "Copio mod[0] agli indirizzi specificati nel file ELF:");
	entry = carica_modulo(mod, mbi->mods_count);
	if (!entry) {
		flog(LOG_ERR, "ATTENZIONE: impossibile caricare mod[0]");
		return;
	}

	// ora mbi non ci serve più e possiamo riusare la memoria nel primo MiB come heap
	heap_init(voidptr_cast(DIM_PAGINA), mem_lower - DIM_PAGINA);
	// nello heap creiamo tre strutture che devono essere preservate anche dopo che
	// avremo ceduto il controllo al primo modulo:
	// 1) La struttura boot64_info;
	boot64_info *info = new boot64_info{tmp_info};
	if (!info) {
		flog(LOG_ERR, "ATTENZIONE: impossibile allocare boot64_info");
		return;
	}
	// 2) Il segmento TSS
	TSS *tss = new TSS;
	if (!tss) {
		flog(LOG_ERR, "ATTENZIONE: impossibile allocare il segmento TSS");
		return;
	}
	// che inizializziamo, passando al primo modulo l'informazione su dove
	// si trova tss_punt_nucleo al suo interno;
	info->tss_punt_nucleo = tss_init(tss);
	// 3) la GDT
	GDT *gdt = new GDT;
	if (!gdt) {
		flog(LOG_ERR, "ATTENZIONE: impossible allocare la tabella GDT");
		return;
	}
	// che inizializziamo.
	gdt_init(gdt, tss);
	// i dettagli su GDT e TSS si trovano in boot64/boot.s

	// Ora creiamo la finestra FM. Le tabelle verranno allocate nello heap.
	paddr root_tab = alloca_tab();
	if (!root_tab) {
		flog(LOG_ERR, "ATTENZIONE: impossibile allocare la tabella radice");
		return;
	}
	if (!crea_finestra_FM(root_tab, mem_tot)) {
		flog(LOG_ERR, "ATTENZIONE: fallimento in crea_finestra_FM()");
		return;
	}

	// prima di attivare la paginazione dobbiamo caricare cr3
	loadCR3(root_tab);
	if (debug_mode) {
		wait_for_gdb = 1;
		flog(LOG_INFO, "Attendo collegamento da gdb...");
	}
	// Da questo punto in poi non ci sono più allocazioni nello heap. Passiamo
	// l'attuale contenuto di memlibera (testa della lista dei chunk liberi
	// nello heap) al primo modulo, in modo che possa usare il resto della
	// memoria disponibile nel primo MiB.
	info->memlibera = int_cast<uintptr_t>(memlibera);
	flog(LOG_INFO, "Attivo la modalita' a 64 bit e cedo il controllo a mod[0]...");
	attiva_paginazione(info, entry, MAX_LIV);

	/* mai raggiunto */
	return;
}