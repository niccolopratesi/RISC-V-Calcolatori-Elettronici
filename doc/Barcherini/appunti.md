# Appunti Risc-V
## Toolchain
`sudo apt install gcc-riscv64-unknown-elf`
`sudo apt install qemu-system`
Non viene installato automaticamente il gdb, quindi bisogna installarlo e compilarlo manualmente dalla repo https://github.com/riscv-collab/riscv-gnu-toolchain e aggiungere /opt/riscv/bin alla variabile d'ambiente PATH.

## Boot segmento utente
Il segmento utente va compilato con -fpic per renderlo position-independent e linkato separatamente dal segmento kernel. Uso kernel.ld per spostare tutte le sezioni dei file nella cartella user nella sezione .user e mi salvo l'indirizzo di testo e dati definiti dal collegatore in kernel.ld per poterli usare nel kernel.

- creare libreria libce
- inizializzare lo heap sistema e utente
- compilare ogni modulo con la libreria --> posso usare new e delete --> lib.cpp per mappare lo heap utente
- mappare la memoria

Aggiornata ctors con registri preservati e modificati i file c in cpp (tranne pci, vga, plic e uart)