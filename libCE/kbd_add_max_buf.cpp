#include "libce.h"
#include "kbd.h"

namespace kbd {

  void add_max_buf()
  {
    int added = 0;
    int i = eventq.avail->idx;
    while (i % QUEUE_SIZE != next_idx_read % QUEUE_SIZE || (i == eventq.used->idx && next_idx_read == eventq.used->idx)) {
      int true_i = i % QUEUE_SIZE;
      add_buf_desc(eventq, true_i, (natq) &buf[true_i], sizeof(virtio_input_event), VIRTQ_DESC_F_WRITE, 0);
      eventq.avail->ring[(eventq.avail->idx + added++) % QUEUE_SIZE] = true_i;
      i++;
    }
    memory_barrier();
    eventq.avail->idx += added;
    memory_barrier();
    if (eventq.used->flags == 0) {
      *((natw *) eventq_notify_addr) = 0;
    }
  }

}
