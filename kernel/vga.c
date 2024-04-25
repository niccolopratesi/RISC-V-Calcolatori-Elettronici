//
// pci.c maps the VGA framebuffer at 0x40000000 and
// passes that address to vga_init().
//
// vm.c maps the VGA "IO ports" at 0x3000000.
//
// we're talking to hw/display/vga.c in the qemu source.
//
// http://www.osdever.net/FreeVGA/home.htm
// https://wiki.osdev.org/VGA_Hardware
// https://files.osdev.org/mirrors/geezer/osd/graphics/modes.c
// official qemu documentation:
// https://gitlab.com/qemu-project/qemu/-/blob/master/docs/specs/standard-vga.txt
// for further development:
// https://dev.to/frosnerd/writing-my-own-vga-driver-22nn

// VGA misc infos: http://www.osdever.net/FreeVGA/home.htm
//                 https://wiki.osdev.org/VGA_Hardware


#include "tipo.h"
#include "uart.h"
#include "font.h"
#include "palette.h"

#define MISC 0x3c2
#define SEQ 0x3c4
#define CRTC 0x3d4
#define GC 0x3ce
#define AC 0x3c0
#define PC 0x3c8
#define PD 0x3c9
#define VGA_TEXT_HEIGHT 25
#define VGA_TEXT_WIDTH 80


uint8 readport(uint32 port, uint8 index);
void writeport(uint32 port, uint8 index, uint8 val);

void init_textmode_80x25();
void init_graphicmode_320x300();
void init_MISC();
void init_SEQ();
void init_CRTC();
void init_GC();
void init_AC();
void init_PD();
void load_font(unsigned char* font_16);
void load_palette();
void post_font();
void print_VGA(char *message, uint8 fg, uint8 bg);

volatile uint8 __attribute__((unused)) discard; // write to this to discard
char *vga_buf;

static volatile uint8 *const VGA_BASE = (uint8 *)0x3000000L;

static inline uint16 encode_char(char c, uint8 fg, uint8 bg) {
  return ((bg & 0xf) << 4 | (fg & 0xf)) << 8 | c;
}


static inline void memcpy(void *restrict dest, const void *restrict src,
                          uint n) {
  unsigned char *d = dest;
  const unsigned char *s = src;

  for (uint i = 0; i < n; i++) {
    *(d + i) = *(s + i);
  }
}

void vga_init(char *vga_framebuffer) {

  init_textmode_80x25();
  print_VGA("Hello RISC-V world!\n", 0x02, 0x00);
}

uint8 readport(uint32 port, uint8 index) {
  uint8 read;
  discard = VGA_BASE[0x3da];
  switch (port) {
  case AC:
    VGA_BASE[AC] = index;
    read = VGA_BASE[0x3c1];
    break;
  case 0x3c2:
    read = VGA_BASE[0x3cc];
    break;
  case 0x3c4:
  case 0x3ce:
  case 0x3d4:
    VGA_BASE[port] = index;
    read = VGA_BASE[port + 1];
    break;
  case 0x3d6:
    read = VGA_BASE[0x3d6];
    break;
  case PD:
    read = VGA_BASE[PD];
    break;
  default:
    read = 0xff;
    break;
  }
  discard = VGA_BASE[0x3da];
  return read;
}

void writeport(uint32 port, uint8 index, uint8 val) {
  discard = VGA_BASE[0x3da];
  switch (port) {
  case AC:
    VGA_BASE[AC] = index;
    VGA_BASE[AC] = val;
    break;
  case 0x3c2:
    VGA_BASE[0x3c2] = val;
    break;
  case 0x3c4:
  case 0x3ce:
  case 0x3d4:
    VGA_BASE[port] = index;
    VGA_BASE[port + 1] = val;
    break;
  case 0x3d6:
  case 0x3d5:
    VGA_BASE[0x3d6] = val;
    break;
  case 0x3c7:
  case 0x3c8:
  case PD:
    VGA_BASE[port] = val;
    break;
  }
  discard = VGA_BASE[0x3da];
}

void init_AC(){
  writeport(AC, 0x00, 0x00);
  writeport(AC, 0x01, 0x01);
  writeport(AC, 0x02, 0x02);
  writeport(AC, 0x03, 0x03);
  writeport(AC, 0x04, 0x04);
  writeport(AC, 0x05, 0x05);
  writeport(AC, 0x06, 0x14);
  writeport(AC, 0x07, 0x07);
  writeport(AC, 0x08, 0x38);
  writeport(AC, 0x09, 0x39);
  writeport(AC, 0x0a, 0x3A);
  writeport(AC, 0x0b, 0x3B);
  writeport(AC, 0x0c, 0x3C);
  writeport(AC, 0x0d, 0x3D);
  writeport(AC, 0x0e, 0x3E);
  writeport(AC, 0x0f, 0x3F);

  writeport(AC, 0x10, 0x0c);
  writeport(AC, 0x11, 0x00);
  writeport(AC, 0x12, 0x0f);
  writeport(AC, 0x13, 0x08);
  writeport(AC, 0x14, 0x00);
}

void init_SEQ(){
  writeport(SEQ, 0x00, 0x03);
  writeport(SEQ, 0x01, 0x00);
  writeport(SEQ, 0x02, 0x03);
  writeport(SEQ, 0x03, 0x00);
  writeport(SEQ, 0x04, 0x02);
}

void init_GC(){
  writeport(GC, 0x00, 0x00);
  writeport(GC, 0x01, 0x00);
  writeport(GC, 0x02, 0x00);
  writeport(GC, 0x03, 0x00);
  writeport(GC, 0x04, 0x00);
  writeport(GC, 0x05, 0x10);
  writeport(GC, 0x06, 0x0E);
  writeport(GC, 0x07, 0x0F);
  writeport(GC, 0x08, 0xFF);
}

void init_CRTC(){
  writeport(CRTC, 0x11, 0x00);

  writeport(CRTC, 0x00, 0x5f);
  writeport(CRTC, 0x01, 0x4f);
  writeport(CRTC, 0x02, 0x50);
  writeport(CRTC, 0x03, 0x82);
  writeport(CRTC, 0x04, 0x55);
  writeport(CRTC, 0x05, 0x81);
  writeport(CRTC, 0x06, 0xBF);
  writeport(CRTC, 0x07, 0x1F);

  writeport(CRTC, 0x08, 0x00);
  writeport(CRTC, 0x09, 0x4f);
  writeport(CRTC, 0x0a, 0x0D);
  // writeport(CRTC, 0x0a, 0x10);
  writeport(CRTC, 0x0b, 0x0E);
  writeport(CRTC, 0x0c, 0x00);
  writeport(CRTC, 0x0d, 0x00);
  writeport(CRTC, 0x0e, 0x00);
  writeport(CRTC, 0x0f, 0x00);

  writeport(CRTC, 0x10, 0x9C);
  writeport(CRTC, 0x11, 0x8E);
  writeport(CRTC, 0x12, 0x8F);
  writeport(CRTC, 0x13, 0x28);
  writeport(CRTC, 0x14, 0x1F);
  writeport(CRTC, 0x15, 0x96);
  writeport(CRTC, 0x16, 0xB9);
  writeport(CRTC, 0x17, 0xA3);
  writeport(CRTC, 0x18, 0xFF);
}

void init_PD(){

  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x0;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x2a;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x15;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x3f;
  VGA_BASE[PD] = 0x3f;
}

void load_font(unsigned char* font_16){
  VGA_BASE[PC] = 0x0;
  init_PD();
  writeport(MISC, 0x00, 0x67);
  VGA_BASE[AC] = 0x20;
  writeport(SEQ, 0x04, 0x06);
  writeport(SEQ, 0x01, 0x23);
  writeport(GC, 0x05, 0x00);
  writeport(GC, 0x06, 0x05);
  writeport(SEQ, 0x02, 0x04);

  volatile void *vga_buf = (void *)(0x50000000); // 0xa0000
  for (uint32 i = 0; i < 256; ++i) {
    memcpy((void *)(vga_buf + 32 * i), (void*)(font_16+16 * i), 16);
  }
}

void post_font(){
  writeport(SEQ, 0x04, 0x02);
  writeport(AC, 0x00, 0x00);
  writeport(AC, 0x01, 0x01);
  writeport(AC, 0x02, 0x02);
  writeport(AC, 0x03, 0x03);
  writeport(AC, 0x04, 0x04);
  writeport(AC, 0x05, 0x05);
  writeport(AC, 0x06, 0x14);
  writeport(AC, 0x07, 0x07);
  writeport(AC, 0x08, 0x38);
  writeport(AC, 0x09, 0x39);
  writeport(AC, 0x0a, 0x3a);
  writeport(AC, 0x0b, 0x3b);
  writeport(AC, 0x0c, 0x3c);
  writeport(AC, 0x0d, 0x3d);
  writeport(AC, 0x0e, 0x3e);
  writeport(AC, 0x0f, 0x3f);

  writeport(AC, 0x10, 0x0c);
  writeport(AC, 0x11, 0x00);
  writeport(AC, 0x12, 0x0f);
  writeport(AC, 0x13, 0x08);
  writeport(AC, 0x14, 0x00);

  writeport(SEQ, 0x00, 0x03);
  writeport(SEQ, 0x01, 0x00);
  writeport(SEQ, 0x02, 0x03);
  writeport(SEQ, 0x03, 0x00);
  writeport(SEQ, 0x04, 0x02);

  writeport(GC, 0x00, 0x00);
  writeport(GC, 0x01, 0x00);
  writeport(GC, 0x02, 0x00);
  writeport(GC, 0x03, 0x00);
  writeport(GC, 0x04, 0x00);
  writeport(GC, 0x05, 0x10);
  writeport(GC, 0x06, 0x0e);
  writeport(GC, 0x07, 0x0f);
  writeport(GC, 0x08, 0xff);

  writeport(CRTC, 0x11, 0x00);

  writeport(CRTC, 0x00, 0x5f);
  writeport(CRTC, 0x01, 0x4f);
  writeport(CRTC, 0x02, 0x50);
  writeport(CRTC, 0x03, 0x82);
  writeport(CRTC, 0x04, 0x55);
  writeport(CRTC, 0x05, 0x81);
  writeport(CRTC, 0x06, 0xbf);
  writeport(CRTC, 0x07, 0x1f);
  writeport(CRTC, 0x08, 0x00);
  writeport(CRTC, 0x09, 0x4f);
  writeport(CRTC, 0x0a, 0x0d);
  // writeport(CRTC, 0x0a, 0x10);//disable cursor
  writeport(CRTC, 0x0b, 0x0e);
  writeport(CRTC, 0x0c, 0x00);
  writeport(CRTC, 0x0d, 0x00);
  writeport(CRTC, 0x0e, 0x00);
  writeport(CRTC, 0x0f, 0x00);
  writeport(CRTC, 0x10, 0x9c);
  writeport(CRTC, 0x11, 0x8e);
  writeport(CRTC, 0x12, 0x8f);
  writeport(CRTC, 0x13, 0x28);
  writeport(CRTC, 0x14, 0x1f);
  writeport(CRTC, 0x15, 0x96);
  writeport(CRTC, 0x16, 0xb9);
  writeport(CRTC, 0x17, 0xa3);
  writeport(CRTC, 0x18, 0xff);

  writeport(MISC, 0x00, 0x67);
  VGA_BASE[AC] = 0x20;
}


void print_VGA(char *message, uint8 fg, uint8 bg)
{
  volatile uint8 *p = (void *)(0x50000000 | (0xb8000 - 0xa0000));// 0xb8000
  // take current cursor position
  int cursor = readport(CRTC, 0x0f);
  cursor = (cursor+1)*2 -2; // where to write
  while(*message){
    if(*message !=  0x0a){
      p[cursor++] = *message;
      p[cursor++] = (bg<<4) | fg;
    }
    else{
      int line_offset = cursor%(VGA_TEXT_WIDTH*2);
      cursor = cursor + VGA_TEXT_WIDTH*2 - line_offset;
    }
    message++;
  }
  p[cursor++] = ' ';
  p[cursor++] = (bg<<4) | fg;
  // new cursor position
  writeport(CRTC, 0x0e, (cursor/2 -1) >> 8);
  writeport(CRTC, 0x0f, (cursor/2 -1) & 0xff);
}

void init_textmode_80x25(){
  writeport(SEQ, 0x00, 0x01);
  writeport(MISC, 0x00, 0xc3);
  writeport(SEQ, 0x00, 0x03);
  init_AC();
  init_SEQ();
  init_GC();
  init_CRTC();
  load_font(font_16);
  post_font();
}

void init_graphicmode_320x300(){
  writeport(MISC, 0xff, 0x63);

  // Set up register SEQ
  writeport(SEQ, 0x00, 0x03);
  writeport(SEQ, 0x01, 0x01);
  writeport(SEQ, 0x02, 0x0f);
  writeport(SEQ, 0x03, 0x00);
  writeport(SEQ, 0x04, 0x0e);

  // Unlock VGA register access
  writeport(CRTC, 0x11, 0x0e);

  // Set up register CRTC
  writeport(CRTC, 0x00, 0x5f);
  writeport(CRTC, 0x01, 0x4f);
  writeport(CRTC, 0x02, 0x50);
  writeport(CRTC, 0x03, 0x82);
  writeport(CRTC, 0x04, 0x54);
  writeport(CRTC, 0x05, 0x82);
  writeport(CRTC, 0x06, 0xbf);
  writeport(CRTC, 0x07, 0x1f);

  writeport(CRTC, 0x08, 0x00);
  writeport(CRTC, 0x09, 0x41);

  writeport(CRTC, 0x10, 0x9c);
  writeport(CRTC, 0x12, 0x8f);
  writeport(CRTC, 0x13, 0x28);
  writeport(CRTC, 0x14, 0x40);
  writeport(CRTC, 0x15, 0x96);
  writeport(CRTC, 0x16, 0xb9);
  writeport(CRTC, 0x17, 0xa3);
  writeport(CRTC, 0x18, 0xff);
  // Set up register GC
  writeport(GC, 0x00, 0x00);
  writeport(GC, 0x01, 0x00);
  writeport(GC, 0x02, 0x00);
  writeport(GC, 0x03, 0x00);
  writeport(GC, 0x04, 0x00);
  writeport(GC, 0x05, 0x40);
  writeport(GC, 0x06, 0x05);
  writeport(GC, 0x07, 0x0f);
  writeport(GC, 0x08, 0xff);
  // Set up AC
  writeport(AC, 0x00, 0x00);
  writeport(AC, 0x01, 0x01);
  writeport(AC, 0x02, 0x02);
  writeport(AC, 0x03, 0x03);
  writeport(AC, 0x04, 0x04);
  writeport(AC, 0x05, 0x05);
  writeport(AC, 0x06, 0x06);
  writeport(AC, 0x07, 0x07);

  writeport(AC, 0x08, 0x08);
  writeport(AC, 0x09, 0x09);
  writeport(AC, 0x0a, 0x0a);
  writeport(AC, 0x0b, 0x0b);
  writeport(AC, 0x0c, 0x0c);
  writeport(AC, 0x0d, 0x0d);
  writeport(AC, 0x0e, 0x0e);
  writeport(AC, 0x0f, 0x0f);

  writeport(AC, 0x10, 0x41);
  writeport(AC, 0x11, 0x00);
  writeport(AC, 0x12, 0x0f);
  writeport(AC, 0x13, 0x00);
  writeport(AC, 0x14, 0x00);

  // Enable display
  writeport(AC, 0xff, 0x20);
  load_palette();
}

void load_palette(){
  // configure a custom VGA palette
  for (int i = 0; i < 256; i++) {
    std_palette[i] = 0;
    std_palette[i] |= ((i & 0xc0)) << 16;
    std_palette[i] |= ((i & 0x38) << 2) << 8;
    std_palette[i] |= ((i & 0x07) << 5);
  }
  std_palette[255] = 0xfcfcfc;

  // Set default VGA palette
  writeport(PC, 0xff, 0x00);
  for (int i = 0; i < 256; i++) {
    writeport(PD, 0xff, (std_palette[i] & 0xfc0000) >> 18);
    writeport(PD, 0xff, (std_palette[i] & 0x00fc00) >> 10);
    writeport(PD, 0xff, (std_palette[i] & 0x0000fc) >> 2);
  }
}
