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

