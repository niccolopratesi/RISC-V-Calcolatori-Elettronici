#ifndef PCI_H
#define PCI_H

namespace pci {

	const ioaddr CAP = 0x0CF8;
	const ioaddr CDP = 0x0CFC;

}

natl* make_CAP(natb bus, natb dev, natb fun, natb off);

struct PCI_config {
  natw vendor_id;
  natw device_id;
  natw command;
  natw status;
  natb revision_id;
  natb prog_if;
  natb subclass;
  natb class_code;
  natb cache_line_size;
  natb latency_timer;
  natb header_type;
  natb bist;
  natl bar0;
  natl bar1;
  natl bar2;
  natl bar3;
  natl bar4;
  natl bar5;
  natl cardbus_cis_pointer;
  natw subsystem_vendor_id;
  natw subsystem_id;
  natl expansion_rom_base_address;
  natb capabilities_pointer;
  natb padding[7];
  natb interrupt_line;
  natb interrupt_pin;
  natb min_grant;
  natb max_latency;
};

#define VIRT_IMSIC_M 0x24000000
#define VIRT_IMSIC_S 0x28000000

struct MSIX_capability {
  natb id;
  natb next_pointer;
  natw message_control;
  natl table_offset;
  natl pending_bit_offset;
};

struct MSIX_entry {
  natl message_address;
  natl message_upper_address;
  natl message_data;
  natl vector_control;
};

struct MSIX_PBA_entry {
  natq pending_bits;
};

struct capability_elem {
  natb id;
  natb next_pointer;
};

bool msix_add_entry(MSIX_entry *table, int index, paddr addr, natl data);

#endif /* PCI_H */
