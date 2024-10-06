#ifndef VIRTIO_H
#define VIRTIO_H

#include <tipo.h>

#define VIRTQ_DESC_F_NEXT      1
#define VIRTQ_DESC_F_WRITE     2
#define VIRTQ_DESC_F_INDIRECT  4

#define VIRTQ_USED_F_NO_NOTIFY      1
#define VIRTQ_AVAIL_F_NO_INTERRUPT  1

#define VIRTIO_F_INDIRECT_DESC  28
#define VIRTIO_F_EVENT_IDX      29
#define VIRTIO_F_ANY_LAYOUT     27

struct alignas(16) virtq_desc {
  natq addr;
  natl len;
  natw flags;
  natw next;
};

struct alignas(2) virtq_avail {
  natw flags;
  natw idx;
  natw ring[];
  /* Only if VIRTIO_F_EVENT_IDX: natw used_event; */
};

struct virtq_used_elem {
  natl id;
  natl len;
};

struct alignas(4) virtq_used {
  natw flags;
  natw idx;
  struct virtq_used_elem ring[];
  /* Only if VIRTIO_F_EVENT_IDX: natw avail_event; */
};

struct virtq {
  unsigned int num;

  struct virtq_desc *desc;
  struct virtq_avail *avail;
  struct virtq_used *used;
};

struct virtio_pci_cap {
  natb cap_vndr;
  natb cap_next;
  natb cap_len;
  natb cfg_type;
  natb bar;
  natb id;
  natb padding[2];
  natl offset;
  natl length;
};

#define VIRTIO_PCI_CAP_COMMON_CFG         1
#define VIRTIO_PCI_CAP_NOTIFY_CFG         2
#define VIRTIO_PCI_CAP_ISR_CFG            3
#define VIRTIO_PCI_CAP_DEVICE_CFG         4
#define VIRTIO_PCI_CAP_PCI_CFG            5
#define VIRTIO_PCI_CAP_SHARED_MEMORY_CFG  8
#define VIRTIO_PCI_CAP_VENDOR_CFG         9

struct virtio_pci_common_cfg {
  natl device_feature_select;
  natl device_feature;
  natl driver_feature_select;
  natl driver_feature;
  natw config_msix_vector;
  natw num_queues;
  natb device_status;
  natb config_generation;

  natw queue_select;
  natw queue_size;
  natw queue_msix_vector;
  natw queue_enable;
  natw queue_notify_off;
  natq queue_desc;
  natq queue_driver;
  natq queue_device;
  natw queue_notify_config_data;
  natw queue_reset;

  natw admin_queue_index;
  natw admin_queue_num;
};

struct virtio_pci_notify_cap {
  struct virtio_pci_cap cap;
  natl notify_off_multiplier;
};

#define ISR_QUEUE_INT_BIT                 1
#define ISR_DEVICE_CONFIGURATION_INT_BIT  2

#define VIRTIO_MSI_NO_VECTOR  0xffff

#define ACKNOWLEDGE_STATUS_BIT  1
#define DRIVER_STATUS_BIT       2
#define DRIVER_OK_STATUS_BIT    4
#define FEATURES_OK_STATUS_BIT  8
#define DEVICE_NEEDS_RESET_BIT  64
#define FAILED_STATUS_BIT       128

#define VIRTIO_F_INDIRECT_DESC  28
#define VIRTIO_F_EVENT_IDX      29
#define VIRTIO_F_VERSION_1      32
#define VIRTIO_F_RING_RESET     40

bool create_virtq(virtq &queue, natw queue_size, natw avail_flags);
bool enable_virtq(virtio_pci_common_cfg &common_cfg, natw queue_select, natw queue_size,
                  natw queue_msix_vector, virtq queue, paddr (*get_real_addr)(void *ff));
void add_buf_desc(virtq &queue, natw index, natq addr, natl len, natw flags, natw next,
                  paddr (*get_real_addr)(void *ff));
void memory_barrier();

#endif /* VIRTIO_H */
