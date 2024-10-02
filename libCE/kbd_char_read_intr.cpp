#include "libce.h"
#include "kbd.h"

namespace kbd {

  char char_read_intr()
  {
    natw idx = eventq->used->ring[next_idx_read++ % QUEUE_SIZE].id;
    virtio_input_event vie = *((virtio_input_event *) eventq->desc[idx].addr);
    if (vie.type != EV_KEY)
      return 0;
    if (vie.code == KEY_LEFTSHIFT)
      shift = (vie.value == KEY_PRESSED);
    if (vie.value == KEY_RELEASED)
      return 0;
    return conv(vie.code);
  }

}
