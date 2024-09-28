#include "internal.h"
#include "kbd.h"
#include "vid.h"
using namespace vid;

// attende che venga premuto un tato carattere
void pause()
{
	str_write("Premere un tasto carattere per proseguire\n");
	char_read();
}
