#include "internal.h"
#include "heap.h"

void heap_init(void *start, size_t size, natq initmem, natq initdim)
{
  if (initmem) {
    memlibera = ptr_cast<des_mem>(initmem);
    memlibera->dimensione = initdim;
  }
	free_interna(start, size);
}
