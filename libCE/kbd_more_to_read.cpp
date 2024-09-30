#include "libce.h"
#include "kbd.h"

namespace kbd {

  bool more_to_read()
  {
    return next_idx_read != eventq->used->idx;
  }

}
