#include "internal.h"
#include "kbd.h"

// attende che venga premuto un tato carattere
void pause()
{
	str_write("Premere un tasto carattere per proseguire\n");
	char_read();
}
