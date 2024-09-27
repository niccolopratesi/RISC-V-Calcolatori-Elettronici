#include "libce.h"
#include "kbd.h"

namespace kbd {

  void disable_intr()
  {
    msix_cap->message_control |= 0x4000;
  }

}
