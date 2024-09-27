#include "libce.h"
#include "pci.h"

bool msix_add_entry(MSIX_entry *table, int index, paddr addr, natl data)
{
  if (addr % 4 || index < 0)
    return false;

  table[index].message_address = addr;
  table[index].message_upper_address = addr >> 32;
  table[index].message_data = data;
  table[index].vector_control &= ~0b1;

  return true;
}
