//
// simple PCI initialization
// https://wiki.osdev.org/PCI#Address_and_size_of_the_BAR
//
#include "tipo.h"
#include "costanti.h"
//#include "libce.h"


struct PCI_config{
  natw vendorID;
  natw deviceID;
  natw command;
  natw status;
  natb revisionID;
  natb progIF;
  natb subclass;
  natb classcode;
  natb cachelineSize;
  natb latencyTimer;
  natb headerType;
  natb BIST;
  natl bar0;
  natl bar1;
  natl bar2;
  natl bar3;
  natl bar4;
  natl bar5;
  natl cardbus__cis_pointer;
  natw subsystem_vendorID;
  natw subsystemID;
  natl expansionROMaddress;
  natb capabilities_pointer;
  natb padding1;
  natw padding2;
  natl padding3;
  natb interruptLine;
  natb interruptPIN;
  natb minGrant;
  natb maxLatency;
};

void pci_init(){

  // buffer per la vga
  //natq vga_frame_buffer = VGA_FRAMEBUFFER;

  // qemu -machine virt puts PCIe config space here.
  natl  *ecam = (natl *) PCI_ECAM;

  // look at each device on bus 0.
  for(int dev = 0; dev < 32; dev++){
    int bus = 0;
    int func = 0;
    int offset = 0;
    natl off = (bus << 16) | (dev << 11) | (func << 8) | (offset);
    volatile natl *base = ecam + off;
    natl id = base[0];

    if(id == 0x11111234){
      //flog(LOG_INFO,"VGA trovata");
      // PCI device ID 1111:1234 is VGA
      // VGA is at 00:01.0, using extended control registers (4096 bytes)

      for(int i = 0; i < 6; i++){
        natl old = base[4+i];

        // writing all 1's to the BAR causes it to be
        // replaced with its size (!).
        base[4+i] = 0xffffffff;
        base[4+i] = old;
      }

      // tell the VGA to reveal its framebuffer at
      // physical address 0x50000000.
      base[4+0] = VGA_FRAMEBUFFER;
      // tell the VGA to set up I/O ports at 0x40000000
      base[4+2] = VGA_MMIO_PORTS;

      // command and status register.
      // bit 0 : I/O access enable
      // bit 1 : memory access enable
      // bit 2 : enable mastering
      //abilitiamo accessi in memoria per il dispositivo (bit 1)
      //nel command register
      base[1] = base[1] | 0x2;

      //setup video mode
      //enable LFB and 8-bit DAC via 0xb0c3 bochs register
      natw* bochs_pointer = (natw*) (VGA_MMIO_PORTS+0x508);
      *bochs_pointer = 0x60;

      vga_init((char*)VGA_FRAMEBUFFER);
      break;
    }
  }

}
