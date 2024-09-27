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

// input event codes
#define EV_SYN        0x00
#define EV_KEY        0x01
#define EV_REL        0x02
#define EV_ABS        0x03
#define EV_MSC        0x04
#define EV_SW         0x05
#define EV_LED        0x11
#define EV_SND        0x12
#define EV_REP        0x14
#define EV_FF         0x15
#define EV_PWR        0x16
#define EV_FF_STATUS  0x17

namespace kbd {
  const natl MAX_CODE = 42;
  const natw QUEUE_SIZE = 64;
  const paddr KBD_PCI = 0x0UL;
  const paddr KBD_MMIO = 0x60000000UL;
  const paddr KBD_MSIX = 0x60010000UL;

  bool shift;
  natb tab[MAX_CODE];
  char tabmin[MAX_CODE];
  char tabmax[MAX_CODE];

  natl notify_off_multiplier;
  volatile natl msix_data_buffer;

  virtio_input_event buf[QUEUE_SIZE];
  virtq eventq;
  virtq statusq;
};

bool msix_add_entry(MSIX_entry *table, int index, paddr addr, natl data)
{
  if (addr % 4 || index < 0) {
    return false;
  }

  table[index].message_address = addr;
  table[index].message_upper_address = addr >> 32;
  table[index].message_data = data;
  table[index].vector_control &= ~0b1;

  return true;
}

bool kbd_setup(PCI_config *conf)
{
  // flog(LOG_INFO, "virtio_keyboard_pci trovata");
  // flog(LOG_INFO, "spazio di configurazione pci a 0x%x", conf);

  // verifica che il dispositivo trovato vada bene
  if (conf->revision_id < 1) {
    // flog(LOG_INFO, "campo revision id non valido");
    return false;
  }
  // flog(LOG_INFO, "campo revision id corretto");
  if (conf->subsystem_id < 0x40) {
    // flog(LOG_INFO, "campo subsytem (device) id non valido");
    return false;
  }
  // flog(LOG_INFO, "campo subsytem (device) id corretto");

  // flog(LOG_INFO, "interrupt pin %d", conf->interrupt_pin);
  // flog(LOG_INFO, "interrupt line %d", conf->interrupt_line);

  // verifica della presenza della capabilities list
  if ((conf->status & 0b10000) == 0) {
    // flog(LOG_INFO, "capabilities list non presente");
    return false;
  }
  // flog(LOG_INFO, "capabilities list presente");
  // scansione della pci capabilities list
  // flog(LOG_INFO, "offset della capabilities list 0x%x", conf->capabilities_pointer);
  paddr conf_base = (paddr) conf;
  capability_elem *cap = (capability_elem *) (conf_base + conf->capabilities_pointer);
  while (true) {
    if (cap->id == 0x11) {
      // flog(LOG_INFO, "msi-x capability a 0x%x", cap);
      MSIX_capability *msix = (MSIX_capability *) cap;
      // flog(LOG_INFO, "msi-x table size %d", (msix->message_control & 0x7FF) + 1);
      // flog(LOG_INFO, "msi-x table bar indicator %d", msix->table_offset & 0b111);
      // flog(LOG_INFO, "msi-x table offset %d", msix->table_offset & ~0b111);
      // flog(LOG_INFO, "msi-x pending bit array bar indicator %d", msix->pending_bit_offset & 0b111);
      // flog(LOG_INFO, "msi-x pending bit array offset %d", msix->pending_bit_offset & ~0b111);
      // flog(LOG_INFO, "abilito la funzione msi-x");
      msix->message_control |= 0x8000;
      // flog(LOG_INFO, "maschero la funzione");
      // msix->message_control |= 0x4000;
    } else if (cap->id == 0x09) {
      // flog(LOG_INFO, "virtio capability a 0x%x", cap);
      virtio_pci_cap *vpc = (virtio_pci_cap *) cap;
      switch (vpc->cfg_type) {
      case VIRTIO_PCI_CAP_NOTIFY_CFG:
        virtio_pci_notify_cap *vpnc = (virtio_pci_notify_cap *) vpc;
        kbd::notify_off_multiplier = vpnc->notify_off_multiplier;
        break;
      }
    // } else {
      // flog(LOG_INFO, "capability non riconosciuta a 0x%x", cap);
    }
    if (!cap->next_pointer)
      break;
    cap = (capability_elem *) (conf_base + cap->next_pointer);
  }

  // assegnare indirizzo al bar1
  // "a value of 0x0 means the base register is 32-bit wide and can be mapped anywhere in in the 32-bit memory space"
  // flog(LOG_INFO, "zona di memoria assegnata al bar 1 0x%x", KBD_MSIX);
  conf->bar1 = kbd::KBD_MSIX;
  // flog(LOG_INFO, "bar1 0x%x", conf->bar1);
  // assegnare indirizzo ai bar4 e bar5
  // "a value of 0x2 means the base register is 64-bits wide and can be mapped anywhere in the 64-bit memory space"
  // flog(LOG_INFO, "zona di memoria assegnata ai bar 4 e 5 0x%lx", KBD_MMIO);
  conf->bar4 = kbd::KBD_MMIO;
  conf->bar5 = kbd::KBD_MMIO >> 32;
  // flog(LOG_INFO, "bar4 0x%x", conf->bar4);
  // flog(LOG_INFO, "bar5 0x%x", conf->bar5);
  // abilitiamo il dispositivo a rispondere ad accessi in memoria
  // abilitiamo il dispositivo a fare bus mastering
  conf->command |= 0b110;
  // flog(LOG_INFO, "command 0x%x", conf->command);
  // flog(LOG_INFO, "status 0x%x", conf->status);

  virtio_pci_common_cfg *comm_cfg = (virtio_pci_common_cfg *) kbd::KBD_MMIO;
  virtio_input_config *in_cfg = (virtio_input_config *) (kbd::KBD_MMIO + 0x2000);

  // "1. Reset device."
  // flog(LOG_INFO, "reset del device");
  comm_cfg->device_status = 0;
  while (comm_cfg->device_status);

  // "2. Set the ACKNOWLEDGE status bit: the guest OS has noticed the device."
  // flog(LOG_INFO, "set del acknowledge status bit");
  comm_cfg->device_status |= ACKNOWLEDGE_STATUS_BIT;

  // "3. Set the DRIVER status bit: the guest OS knows how to drive the device."
  // flog(LOG_INFO, "set del driver status bit");
  comm_cfg->device_status |= DRIVER_STATUS_BIT;

  // "4. Read device feature bits, and write the subset of feature bits understood by the OS and driver to the device."
  // flog(LOG_INFO, "lettura e set dei feature bits");
  /* natl before, after;
  natl feature_bits;
  do {
    before = comm_cfg->config_generation;
    int i = 0;
    for (int sel = 0x0; sel < 0x4; sel++) {
      comm_cfg->device_feature_select = sel;
      feature_bits = comm_cfg->device_feature;
      for (int mask = 1; mask != 0; mask <<= 1) {
        if (feature_bits & mask) {
          flog(LOG_INFO, "feature bit %d settato", i);
        }
        i++;
      }
    }
    after = comm_cfg->config_generation;
  } while (before != after); */

  // flog(LOG_INFO, "scrivo il bit %d nel secondo feature driver", VIRTIO_F_VERSION_1 - 32);
  // comm_cfg->driver_feature_select = 0x0;
  // comm_cfg->driver_feature = 0;
  comm_cfg->driver_feature_select = 0x1;
  comm_cfg->driver_feature = (1 << (VIRTIO_F_VERSION_1 - 32));
  // comm_cfg->driver_feature_select = 0x2;
  // comm_cfg->driver_feature = 0;
  // comm_cfg->driver_feature_select = 0x3;
  // comm_cfg->driver_feature = 0;

  // "5. Set the FEATURE_OK status bit."
  // flog(LOG_INFO, "set del feature ok status bit");
  comm_cfg->device_status |= FEATURES_OK_STATUS_BIT;

  // "6. Re-read device status to ensure the FEATURE_OK bit is still set."
  if ((comm_cfg->device_status & FEATURES_OK_STATUS_BIT) == 0) {
    // flog(LOG_ERR, "il dispositivo non supporta questo sottoinsieme di feature bit");
    // goto errore_setup;
    return false;
  }

  // "7. Perform device-specific setup."
  // "7.1. Discovery of virtqueues for the device."
  // flog(LOG_INFO, "numero di virtqueue offerte %d", comm_cfg->num_queues);

  // "7.2. Optional per-bus setup."
  // flog(LOG_INFO, "inzializzazione della msix vector table");
  kbd::msix_data_buffer = 0;
  #define VIRT_IMSIC_M 0x24000000
  #define VIRT_IMSIC_S 0x28000000
  if (!msix_add_entry((MSIX_entry *) kbd::KBD_MSIX, 0, VIRT_IMSIC_M, 1)) {
    return false;
  }
  // flog(LOG_INFO, "vector control 0x%x", me[0].vector_control);
  if (!msix_add_entry((MSIX_entry *) kbd::KBD_MSIX, 1, VIRT_IMSIC_M, 1)) {
    return false;
  }
  // flog(LOG_INFO, "map dei config change event nel vettore msi-x 0");
  comm_cfg->config_msix_vector = 0x0;
  if (comm_cfg->config_msix_vector == VIRTIO_MSI_NO_VECTOR) {
    // flog(LOG_INFO, "errore nell'assegnamento del config msix vector");
    return false;
  }

  // "7.3. Reading and possibly writing the device's virtio configuration space."
  /* in_cfg->select = VIRTIO_INPUT_CFG_ID_NAME;
  in_cfg->subsel = 0;
  flog(LOG_INFO, "nome del device %.*s", in_cfg->size, in_cfg->u.string);

  in_cfg->select = VIRTIO_INPUT_CFG_EV_BITS;
  in_cfg->subsel = EV_KEY;
  if (in_cfg->size != 0) {
    flog(LOG_INFO, "il dispositivo supporta eventi di tipo EV_KEY");
  } */

  // "7.4. Population of virtqueues."
  // coda 0
  if (!create_virtq(kbd::eventq, kbd::QUEUE_SIZE, 0)) {
    // flog(LOG_ERR, "errore nella creazione della queue 0");
    return false;
  }
  if (!enable_virtq(*comm_cfg, 0, kbd::QUEUE_SIZE, 0x1, kbd::eventq)) {
    // flog(LOG_INFO, "errore nell'abilitazione della queue");
    return false;
  }
  // coda 1
  if (!create_virtq(kbd::statusq, kbd::QUEUE_SIZE, 0)) {
    // flog(LOG_ERR, "errore nella creazione della queue 1");
    return false;
  }
  if (!enable_virtq(*comm_cfg, 1, kbd::QUEUE_SIZE, 0x1, kbd::statusq)) {
    // flog(LOG_INFO, "errore nell'abilitazione della queue");
    return false;
  }
  // fine code
  // flog(LOG_INFO, "queue inizializzate");

  // "8. Set the DRIVER_OK status bit."
  // flog(LOG_INFO, "set del driver ok status bit");
  comm_cfg->device_status |= DRIVER_OK_STATUS_BIT;
  if (comm_cfg->device_status & DEVICE_NEEDS_RESET_BIT) {
    // flog(LOG_ERR, "errore nell'inizializzazione");
    return false;
  }

  // TENTATIVO DI LETTURA DI UN DATO
  // creazione e passaggio alla periferica del buffer
  // flog(LOG_INFO, "creo il buffer per ricevere un dato");
  int added = 0;
  for (int i = 0; i < kbd::QUEUE_SIZE; i++) {
    add_buf_desc(kbd::eventq, i, (natq) &kbd::buf[i], sizeof(virtio_input_event), VIRTQ_DESC_F_WRITE, 0);
    kbd::eventq.avail->ring[(kbd::eventq.avail->idx + added++) % kbd::QUEUE_SIZE] = i;
  }
  memory_barrier();
  kbd::eventq.avail->idx += added;
  memory_barrier();
  if (kbd::eventq.used->flags == 0) {
    comm_cfg->queue_select = 0;
    natw *not_data_p = (natw *) (kbd::KBD_MMIO + 0x3000 + comm_cfg->queue_notify_off * kbd::notify_off_multiplier);
    *not_data_p = 0;
    // flog(LOG_INFO, "richiesto invio di una notifica all'indirizzo 0x%x", not_data_p);
  // } else {
  //   flog(LOG_INFO, "non e' richiesta nessuna notifica");
  }
  // attesa che il buffer venga consumato dalla periferica
  MSIX_PBA_entry *mpbae = (MSIX_PBA_entry *) (kbd::KBD_MSIX + 2048);
  // flog(LOG_INFO, "mi metto in attesa di una notifica a 0x%x", mpbae);
  while (true);
  // while (kbd::msix_data_buffer == 0);
  // while (mpbae[0].pending_bits == 0);
  // while (mpbae[0].pending_bits == 0 && kbd::msix_data_buffer == 0);
  // lettura del buffer usato dalla periferica
  flog(LOG_INFO, "FINALMENTE UN DATO");

  return true;

/* errore_setup:
  comm_cfg->device_status |= FAILED_STATUS_BIT;
errore:
  flog(LOG_ERR, "errore durante l'inizializzazione della tastiera"); */
}

void vga_setup(PCI_config *pointer)
{
  // flog(LOG_INFO,"VGA trovata");
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
      // vga_setup(pointer);
    }
    if (pointer->device_id == 0x1052 && pointer->vendor_id == 0x1af4) {
      flog(LOG_INFO, "=================== KDB =================");
      if (kbd_setup(pointer)) {
        flog(LOG_INFO, "tastiera ok");
      } else {
        flog(LOG_ERR, "tastiera NO");
      }
      flog(LOG_INFO, "=================== KDB =================");
    }
  }
}
