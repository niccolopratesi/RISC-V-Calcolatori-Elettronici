#include "libce.h"
#include "kbd.h"

namespace kbd {

  void enable_intr()
  {
    msix_cap->message_control &= ~0x4000;
  }

}
