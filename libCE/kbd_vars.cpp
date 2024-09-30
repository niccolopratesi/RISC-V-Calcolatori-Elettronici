#include "libce.h"
#include "kbd.h"

namespace kbd {
  bool shift;
  natb tab[MAX_CODE] = {
    0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
    0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x39, 0x1c, 0x0e, 0x01
  };
  char tabmin[MAX_CODE] = {
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', '+',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', ' ', '\r', '\b', 0x1b
  };
  char tabmai[MAX_CODE] = {
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ' ', '\n', '\b', 0x1b
  };

  paddr eventq_notify_addr;
  MSIX_capability *msix_cap = nullptr;

  virtio_input_event *buf = nullptr;
  natw next_idx_read = 0;
  virtq *eventq = nullptr;
  virtq *statusq = nullptr;
}
