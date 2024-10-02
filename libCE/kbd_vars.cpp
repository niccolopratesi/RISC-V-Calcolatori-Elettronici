#include "libce.h"
#include "kbd.h"

namespace kbd {
  bool shift = false;
  natw tab[MAX_CODE] = {
    KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL,
    KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P,
    KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L,
    KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_SPACE, KEY_ENTER, KEY_BACKSPACE, KEY_ESC
  };
  char tabmai[MAX_CODE] = {
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', '+',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', ' ', '\r', '\b', 0x1b
  };
  char tabmin[MAX_CODE] = {
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
