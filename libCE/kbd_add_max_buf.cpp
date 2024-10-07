#include "libce.h"
#include "kbd.h"

namespace kbd {

  void add_max_buf()
  {
    int added = 0;
    int i = eventq.avail->idx;
    while (i % QUEUE_SIZE != next_idx_read % QUEUE_SIZE || (i == eventq.used->idx && next_idx_read == eventq.used->idx)) {
      eventq.avail->ring[(eventq.avail->idx + added++) % QUEUE_SIZE] = i % QUEUE_SIZE;
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
