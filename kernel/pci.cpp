//
// simple PCI initialization
// https://wiki.osdev.org/PCI#Address_and_size_of_the_BAR
//
#include "tipo.h"
#include "costanti.h"
#include "libce.h"
#include "pci.h"
#include "virtio.h"
#include "virtioinput.h"
#include "kbd.h"

void kbd_setup(PCI_config *conf)
{
  // Nota: i bar da utilizzare non sono individuati in maniera dinamica, ma sono hard-coded
  // Un miglioramento futuro potrebbe consistenere nell'implementare una funzione che svolge
  // il lavoro del BIOS e produce una struttura dati che può essere usata dai driver nel
  // modulo io per scoprire a quali indirizzi è stata mappata la propria periferica

  // Assegnare l'indirizzo al bar1
  conf->bar1 = kbd::MSIX;
  // Assegnare l'indirizzo ai bar4 e bar5
  conf->bar4 = kbd::MMIO;
  conf->bar5 = kbd::MMIO >> 32;
  // Abilitiamo il dispositivo a rispondere ad accessi in memoria
  // Abilitiamo il dispositivo a fare bus mastering
  conf->command |= 0b110;
}

void vga_setup(PCI_config *pointer)
{
  // flog(LOG_INFO,"VGA trovata");
  // PCI device ID 1111:1234 is VGA
  //VGA is at 00:01.0, using extended control registers (4096 bytes)

  // for(int i = 0; i < 6; i++){
  //   natl old = base[4+i];

  //   // writing all 1's to the BAR causes it to be
  //   // replaced with its size (!).
  //   base[4+i] = 0xffffffff;
  //   base[4+i] = old;
  // }  inutile?

  // tell the VGA to reveal its framebuffer at
  // physical address 0x50000000
  // base[4+0] = VGA_FRAMEBUFFER;
  pointer->bar0 = VGA_FRAMEBUFFER;
  
  // tell the VGA to set up I/O ports at 0x40000000
  // base[4+2] = VGA_MMIO_PORTS;
  pointer->bar2 = VGA_MMIO_PORTS;

  // command and status register.
  // bit 0 : I/O access enable
  // bit 1 : memory access enable
  // bit 2 : enable mastering
  // abilitiamo accessi in memoria per il dispositivo (bit 1)
  // nel command register
  // base[1] = base[1] | 0x2;
  pointer->command |= 0x2;

  // setup video mode
  // enable LFB and 8-bit DAC via 0xb0c3 bochs register
  // natw* bochs_pointer = (natw*) (VGA_MMIO_PORTS+0x508);
  // *bochs_pointer = 0x60;
  // natl* bochs_pointer2 = (natl*) (VGA_MMIO_PORTS+0x604);
  // *bochs_pointer2 = 0x1e1e1e1e;
  
  flog(LOG_INFO, "Inizializzazione VGA in corso");
  vga_init();
}

extern "C" void pci_init()
{

  // buffer per la vga
  // natq vga_frame_buffer = VGA_FRAMEBUFFER;

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
    PCI_config *pointer = (PCI_config *) base;

    if (id == 0x11111234) {
      vga_setup(pointer);
    }
    if (pointer->device_id == 0x1052 && pointer->vendor_id == 0x1af4) {
      kbd_setup(pointer);
      flog(LOG_INFO, "KBD inizializzata");
    }
  }
}
