#ifndef VIRTIOINPUT_H
#define VIRTIOINPUT_H

#include <virtio.h>

enum virtio_input_config_select {
  VIRTIO_INPUT_CFG_UNSET      = 0x00,
  VIRTIO_INPUT_CFG_ID_NAME    = 0x01,
  VIRTIO_INPUT_CFG_ID_SERIAL  = 0x02,
  VIRTIO_INPUT_CFG_ID_DEVIDS  = 0x03,
  VIRTIO_INPUT_CFG_PROP_BITS  = 0x10,
  VIRTIO_INPUT_CFG_EV_BITS    = 0x11,
  VIRTIO_INPUT_CFG_ABS_INFO   = 0x12,
};

struct virtio_input_absinfo {
  natl min;
  natl max;
  natl fuzz;
  natl flat;
  natl res;
};

struct virtio_input_devids {
  natw bustype;
  natw vendor;
  natw product;
  natw version;
};

struct virtio_input_config {
  natb select;
  natb subsel;
  natb size;
  natb reserved[5];
  union {
    char string[128];
    natb bitmap[128];
    struct virtio_input_absinfo abs;
    struct virtio_input_devids ids;
  } u;
};

#endif /* VIRTIOINPUT_H */
