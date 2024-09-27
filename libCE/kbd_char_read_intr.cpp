#include "libce.h"
#include "kbd.h"

namespace kbd {

  char char_read_intr()
  {
    natw idx = eventq.used->ring[next_idx_read].id;
    // incrementa di due perché per ogni tasto premuto la tastiera usa due buffer
    next_idx_read += 2;
    virtio_input_event vie = *((virtio_input_event *) eventq.desc[idx].addr);
    if (vie.value == 0x2A)
      shift = true;
    else if (vie.value == 0xAA)
      shift = false;
    if (vie.value >= 0x80 || vie.value == 0x2A)
      return 0;
    return conv(vie.value);
  }

}
