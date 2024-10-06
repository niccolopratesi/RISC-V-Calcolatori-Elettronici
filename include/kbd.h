#ifndef KBD_H
#define KBD_H

#include "tipo.h"
#include "pci.h"
#include "virtio.h"
#include "virtioinput.h"

namespace kbd {
	const natl MAX_CODE = 42; 
  const natw QUEUE_SIZE = 64;

  const paddr PCI  = 0x30010000UL;
  const paddr MMIO = 0x60000000UL;
  const paddr MSIX = 0x60010000UL;
	
	extern bool shift;
	extern natw tab[MAX_CODE];
	extern char tabmin[MAX_CODE];
	extern char tabmai[MAX_CODE];

  extern paddr (*get_real_addr)(void *ff);

  extern paddr eventq_notify_addr;
  extern MSIX_capability *msix_cap;

  extern virtio_input_event *buf;
  extern natw next_idx_read;
  extern virtq *eventq;
  extern virtq *statusq;

  bool init(paddr (*func)(void *ff));
}

// evdev type
#define EV_KEY        0x01

// evdev code
#define KEY_RESERVED    0
#define KEY_ESC         1
#define KEY_1           2
#define KEY_2           3
#define KEY_3           4
#define KEY_4           5
#define KEY_5           6
#define KEY_6           7
#define KEY_7           8
#define KEY_8           9
#define KEY_9           10
#define KEY_0           11
#define KEY_MINUS       12
#define KEY_EQUAL       13
#define KEY_BACKSPACE   14
#define KEY_TAB         15
#define KEY_Q           16
#define KEY_W           17
#define KEY_E           18
#define KEY_R           19
#define KEY_T           20
#define KEY_Y           21
#define KEY_U           22
#define KEY_I           23
#define KEY_O           24
#define KEY_P           25
#define KEY_LEFTBRACE   26
#define KEY_RIGHTBRACE  27
#define KEY_ENTER       28
#define KEY_LEFTCTRL    39
#define KEY_A           30
#define KEY_S           31
#define KEY_D           32
#define KEY_F           33
#define KEY_G           34
#define KEY_H           35
#define KEY_J           36
#define KEY_K           37
#define KEY_L           38
#define KEY_SEMICOLON   39
#define KEY_APOSTROPHE  40
#define KEY_GRAVE       41
#define KEY_LEFTSHIFT   42
#define KEY_BACKSLASH   43
#define KEY_Z           44
#define KEY_X           45
#define KEY_C           46
#define KEY_V           47
#define KEY_B           48
#define KEY_N           49
#define KEY_M           50
#define KEY_COMMA       51
#define KEY_DOT         52
#define KEY_SLASH       53
#define KEY_RIGHTSHIFT  54
#define KEY_KPASTERISK  55
#define KEY_LEFTALT     56
#define KEY_SPACE       57
#define KEY_CAPSLOCK    58

// evdev value
#define KEY_PRESSED   1
#define KEY_RELEASED  0

#endif /* KBD_H */
