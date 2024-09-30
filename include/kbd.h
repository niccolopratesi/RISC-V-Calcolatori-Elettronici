#ifndef KBD_H
#define KBD_H

#include "tipo.h"
#include "pci.h"
#include "virtio.h"
#include "virtioinput.h"

namespace kbd {
	const natl MAX_CODE = 42; 
  const natw QUEUE_SIZE = 64;

  const paddr PCI  = 0x30010000UL;
  const paddr MMIO = 0x60000000UL;
  const paddr MSIX = 0x60010000UL;
	
	extern bool shift;
	extern natb tab[MAX_CODE];
	extern char tabmin[MAX_CODE];
	extern char tabmai[MAX_CODE];

  extern paddr eventq_notify_addr;
  extern MSIX_capability *msix_cap;

  extern virtio_input_event *buf;
  extern natw next_idx_read;
  extern virtq *eventq;
  extern virtq *statusq;

  bool init();
}

#endif /* KBD_H */
