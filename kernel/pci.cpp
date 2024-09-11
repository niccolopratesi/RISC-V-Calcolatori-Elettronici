//
// simple PCI initialization
// https://wiki.osdev.org/PCI#Address_and_size_of_the_BAR
//
#include "tipo.h"
#include "costanti.h"
#include "libce.h"

struct PCI_config{
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
  natb padding1;
  natw padding2;
  natl padding3;
  natb interrupt_line;
  natb interrupt_pin;
  natb min_grant;
  natb max_latency;
};

extern "C" void pci_init(){

  // buffer per la vga
  //natq vga_frame_buffer = VGA_FRAMEBUFFER;

  // qemu -machine virt puts PCIe config space here.
  natl  *ecam = (natl *) PCI_ECAM;

  // look at each device on bus 0.
  for (int dev = 0; dev < 32; dev++) {
    int bus = 0;
    int func = 0;
    int offset = 0;
    natl off = (bus << 16) | (dev << 11) | (func << 8) | (offset);

    volatile natl *base = ecam + off;
    natl id = base[0];
    // struttura per configurare pci
    PCI_config *pointer = (PCI_config *)(ecam + off);

    if (id == 0x11111234) {
      //flog(LOG_INFO,"VGA trovata");
      // PCI device ID 1111:1234 is VGA
      // VGA is at 00:01.0, using extended control registers (4096 bytes)

      // for(int i = 0; i < 6; i++){
      //   natl old = base[4+i];

      //   // writing all 1's to the BAR causes it to be
      //   // replaced with its size (!).
      //   base[4+i] = 0xffffffff;
      //   base[4+i] = old;
      // }  inutile?

      // tell the VGA to reveal its framebuffer at
      // physical address 0x50000000
      //base[4+0] = VGA_FRAMEBUFFER;
      pointer->bar0 = VGA_FRAMEBUFFER;
      
      // tell the VGA to set up I/O ports at 0x40000000
      //base[4+2] = VGA_MMIO_PORTS;
      pointer->bar2 = VGA_MMIO_PORTS;

      // command and status register.
      // bit 0 : I/O access enable
      // bit 1 : memory access enable
      // bit 2 : enable mastering
      //abilitiamo accessi in memoria per il dispositivo (bit 1)
      //nel command register
      //base[1] = base[1] | 0x2;
      pointer->command |= 0x2;

      //setup video mode
      //enable LFB and 8-bit DAC via 0xb0c3 bochs register
      // natw* bochs_pointer = (natw*) (VGA_MMIO_PORTS+0x508);
      // *bochs_pointer = 0x60;
      // natl* bochs_pointer2 = (natl*) (VGA_MMIO_PORTS+0x604);
      // *bochs_pointer2 = 0x1e1e1e1e;
      
      flog(LOG_INFO,"Inizializzazione VGA in corso");
      vga_init();
      break;
    }
  }
}
