#include "libce.h"
#include "pci.h"
#include "kbd.h"

namespace kbd {
  
  bool init(paddr (*func)(void *ff))
  {
    get_real_addr = func;

    PCI_config *conf = (PCI_config *) PCI;
    virtio_pci_common_cfg *comm_cfg = (virtio_pci_common_cfg *) MMIO;
    natl notify_off_multiplier;

    // verifica che il dispositivo trovato vada bene
    if (conf->revision_id < 1) {
      return false;
    }
    if (conf->subsystem_id < 0x40) {
      return false;
    }

    // verifica della presenza della capabilities list
    if ((conf->status & 0b10000) == 0) {
      return false;
    }
    // scansione della pci capabilities list
    paddr conf_base = (paddr) conf;
    capability_elem *cap = (capability_elem *) (conf_base + conf->capabilities_pointer);
    while (true) {
      if (cap->id == 0x11) {
        msix_cap = (MSIX_capability *) cap;
        msix_cap->message_control |= 0x8000;
        msix_cap->message_control |= 0x4000;
      } else if (cap->id == 0x09) {
        virtio_pci_cap *vpc = (virtio_pci_cap *) cap;
        switch (vpc->cfg_type) {
        case VIRTIO_PCI_CAP_NOTIFY_CFG:
          virtio_pci_notify_cap *vpnc = (virtio_pci_notify_cap *) vpc;
          notify_off_multiplier = vpnc->notify_off_multiplier;
          break;
        }
      }
      if (!cap->next_pointer)
        break;
      cap = (capability_elem *) (conf_base + cap->next_pointer);
    }

    // "1. Reset device."
    comm_cfg->device_status = 0;
    while (comm_cfg->device_status);

    // "2. Set the ACKNOWLEDGE status bit: the guest OS has noticed the device."
    comm_cfg->device_status |= ACKNOWLEDGE_STATUS_BIT;

    // "3. Set the DRIVER status bit: the guest OS knows how to drive the device."
    comm_cfg->device_status |= DRIVER_STATUS_BIT;

    // "4. Read device feature bits, and write the subset of feature bits understood by the OS and driver to the device."
    comm_cfg->driver_feature_select = 0x1;
    comm_cfg->driver_feature = (1 << (VIRTIO_F_VERSION_1 - 32));

    // "5. Set the FEATURE_OK status bit."
    comm_cfg->device_status |= FEATURES_OK_STATUS_BIT;

    // "6. Re-read device status to ensure the FEATURE_OK bit is still set."
    if ((comm_cfg->device_status & FEATURES_OK_STATUS_BIT) == 0) {
      goto error_set_failed;
    }

    // "7. Perform device-specific setup."
    // "7.1. Discovery of virtqueues for the device."
    comm_cfg->queue_select = 0;
    eventq_notify_addr = MMIO + 0x3000 + comm_cfg->queue_notify_off * notify_off_multiplier;

    // "7.2. Optional per-bus setup."
    if (!msix_add_entry((MSIX_entry *) MSIX, 0, VIRT_IMSIC_S, 1)) {
      goto error_set_failed;
    }
    if (!msix_add_entry((MSIX_entry *) MSIX, 1, VIRT_IMSIC_S, 1)) {
      goto error_set_failed;
    }
    comm_cfg->config_msix_vector = 0;
    if (comm_cfg->config_msix_vector == VIRTIO_MSI_NO_VECTOR) {
      goto error_set_failed;
    }

    // "7.3. Reading and possibly writing the device's virtio configuration space."

    // "7.4. Population of virtqueues."
    // coda 0
    if (!create_virtq(eventq, QUEUE_SIZE, 0)) {
      goto error_set_failed;
    }
    if (!enable_virtq(*comm_cfg, 0, QUEUE_SIZE, 1, eventq, get_real_addr)) {
      goto error_set_failed;
    }
    for (int i = 0; i < QUEUE_SIZE; i++) {
      add_buf_desc(eventq, i, (natq) &buf[i], sizeof(buf[i]), VIRTQ_DESC_F_WRITE, 0, get_real_addr);
    }
    // coda 1
    if (!create_virtq(statusq, QUEUE_SIZE, 0)) {
      goto error_set_failed;
    }
    if (!enable_virtq(*comm_cfg, 1, QUEUE_SIZE, 1, statusq, get_real_addr)) {
      goto error_set_failed;
    }

    // "8. Set the DRIVER_OK status bit."
    comm_cfg->device_status |= DRIVER_OK_STATUS_BIT;
    if (comm_cfg->device_status & DEVICE_NEEDS_RESET_BIT) {
      goto error_set_failed;
    }

    // prepariamo la eventq a ricevere i dati
    add_max_buf();
    
    return true;

error_set_failed:
    comm_cfg->device_status |= FAILED_STATUS_BIT;
    return false;
  }

}
