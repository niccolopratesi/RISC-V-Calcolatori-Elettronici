//
// pci.cpp maps the VGA framebuffer at 0x50000000 and
// the VGA "IO ports" at 0x4000000.
//
// we're talking to hw/display/vga.c in the qemu source.
//
// http://www.osdever.net/FreeVGA/home.htm
// https://wiki.osdev.org/VGA_Hardware
// https://files.osdev.org/mirrors/geezer/osd/graphics/modes.c
// official qemu documentation:
// https://gitlab.com/qemu-project/qemu/-/blob/master/docs/specs/standard-vga.txt
// https://dev.to/frosnerd/writing-my-own-vga-driver-22nn

// VGA misc infos: http://www.osdever.net/FreeVGA/home.htm
//                 https://wiki.osdev.org/VGA_Hardware


// text mode 80x25:
// Display Memory at 0xB8000
// Primo byte contiene codifica ASCII del carattere
// Secondo byte contiene 4 bit per il foreground e 
// 4 bit o 3+1 bit per background o background+(blink foreground)
// a seconda se il blink è abilitato

//   7  6  5  4   3  2  1  0               7    6  5  4   3  2  1  0  
// ###########################          ##############################
// #    bkgnd   |    frgnd   #          #blink| bkgnd   |    frgnd   #
// ###########################          ##############################

// bit 3 del foreground può selezionare il font 0 o 1 se abilitato

#include "tipo.h"
#include "uart.h"
#include "font.h"
#include "palette.h"
#include "costanti.h"
#include "libce.h"
#include "vid.h"
using namespace vid;

void init_textmode_80x25();
void init_graphicmode_320x200();
void init_MISC();
void init_SEQ();
void init_CRTC();
void init_GC();
void init_AC();
void init_PD();
void load_font(unsigned char* font_16);
void load_palette();
void post_font();

// funzioni per leggere e scrivere nei registri interni della VGA
void writeport(natl port, natb index, natb val);
natb readport(natl port, natb index);

// write to this to discard data
volatile natb __attribute__((unused)) discard; 

static inline natw encode_char(char c, natb fg, natb bg) {
  return ((bg & 0xf) << 4 | (fg & 0xf)) << 8 | c;
}

extern "C" void vga_init() {

  init_textmode_80x25();

  //scritture a 0xa0000 colorano la cella del carattere
  //volatile natb *p = (natb *)(VGA_FRAMEBUFFER);

  //test font
  // int j=0;
  // for(int i=0;i<2000;i++){
  //   p[4*i]=j;
  //   p[4*i+1]=0x0f;
  //   j++;
  // }
  //test clear
  // vid::clear_screen(0x0f);

  //test print
  // print_VGA("Hello RISC-V world!", 0x0f);
  // print_VGA("This is a very very very long text, really really really long, to try it out", 0x0f);
  // print_VGA("\nLet's go on a new line", 0x0f);
  // vid::str_write("Hello RISC-V world!");
  // vid::str_write("This is a very very very long text, really really really long, to try it out");
  // vid::str_write("\nLet's go on a new line");

  //test scroll
  //vid::scroll();


  // //modalità grafica
  // init_graphicmode_320x200();

  // for(int i = 0; i<200; i++){
  //   for(int j = 0; j<320; j++){
  //     if(((i >= 10 && i < 15) || (i > 85 && i <= 90)|| (i >= 110 && i < 115) || (i > 185 && i <= 190)) && (j > 129 && j < 190)){
  //       vga_buf[i*320+j] = 0xfa;
  //     }else if((i > 147 && i < 153) && (j > 130 && j < 170)){
  //       vga_buf[i*320+j] = 0xfa;
  //     }else if(((i>=10 && i<=90) || (i>=110 && i<=190)) && (j > 127 && j < 133)){
  //       vga_buf[i*320+j] = 0xfa;
  //     }else if(j > 99 && j < 220){
  //       vga_buf[i*320+j] = 0xfb;
  //     }else{
  //       vga_buf[i*320+j] = 0xfa;
  //     }
  //   }
  // }

  // for(int i = 0; i < 200; i++){
  //   for(int j = 0; j < 320; j++){
  //     if(((j>88 && j<=99) || (j>=220 && j<231)) && i!=0 && i%40==0){
  //       for(int k = -10; k < 11; k++){
  //         vga_buf[(i+k)*320+j] = 0xf9;
  //       }
  //     }
  //   }
  // }

  // //test graphic mode
  // for(int i=0; i < 200;i++){
  //   for(int j=0; j < 320;j++){
  //     if(j<5 || j>314 || j== 159 || j==160){

  //       p[i*320 + j] = 0xfd;
  //     }else{
  //       p[i*320 + j] = 0xfe;
  //     }
  //   }
  // }

  flog(LOG_INFO,"VGA inizializzata");
}

natb readport(natl port, natb index) 
{
  natb read;

  switch (port) {
  case AC:
      //reset to index mode
      discard= VGA_BASE[INPUT_STATUS_REGISTER];
      VGA_BASE[AC] = index;
      read = VGA_BASE[AC_READ];
      break;
  case MISC:
      read = VGA_BASE[MISC_READ];
      break;
  case SEQ:
      VGA_BASE[port] = index;
      read = VGA_BASE[port + 1];
      break;
  case GC:
      VGA_BASE[port] = index;
      read = VGA_BASE[port + 1];
      break;
  case CRTC:
      VGA_BASE[port] = index;
      read = VGA_BASE[port + 1];
      break;
  case PD:
      read = VGA_BASE[PD];
      break;
  default:
      read = 0xff;
      break;
  }
  discard= VGA_BASE[INPUT_STATUS_REGISTER];
  return read;
}

void writeport(natl port, natb index, natb val) 
{
  switch (port) {
      case AC:
          //reset AC to index mode
          discard = VGA_BASE[INPUT_STATUS_REGISTER];
          //attribute register riceve prima l'indirizzo del registro da indicizzare 
          //e poi il dato da trasferire
          VGA_BASE[AC] = index;
          VGA_BASE[AC] = val;
          break;
      case MISC:
          //miscellaneous output register
          //essendo un external register non ha bisogno di essere indicizzato
          VGA_BASE[MISC] = val;
          break;
      // SEQ, GC e CRTC accedono ai registri interni con lo stesso formato
      case SEQ:
          //sequencer register riceve prima l'indice nell'address register
          //poi scrive il dato nel data register (1 byte successivo)
      case GC:
          //graphics register riceve prima l'indice nell'address register
          //poi scrive il dato nel data register (1 byte successivo)
      case CRTC:
          //cathode ray tube controller register riceve prima l'indice nell'address register
          //poi scrive il dato nel data register (1 byte successivo)
          VGA_BASE[port] = index;
          VGA_BASE[port + 1] = val;
          break;
      case PC:
          //dac register
          //poiché prima bisogna scrivere l'indice di partenza e poi terne di colori RGB
          //nel data register, si evita di fare tutto nella stessa write
          VGA_BASE[port] = index;
          break;
      case PD:
          VGA_BASE[port] = val;
          break;
  }
}
  
void init_AC(){
  //PAS bit reset to load color values into internal palette registers
  //carica 6 bit rgbRGB nel palette register indicizzato
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

  //attribute mode control register: blink enable and line graphics enable
  writeport(AC, 0x10, 0x0c);
  //overscan color register: border color used in text mode
  writeport(AC, 0x11, 0x00);
  //color plane enable register: enable all display-memory color planes
  writeport(AC, 0x12, 0x0f);
  //horizontal pixel planning register: PEL panning video data shift
  writeport(AC, 0x13, 0x08);
  //color select register
  writeport(AC, 0x14, 0x00);
}

void init_SEQ(){
  //reset register: allow sequencer to operate
  writeport(SEQ, 0x00, 0x03);
  //clocking mode register: display enable, 9bit dot mode per character
  writeport(SEQ, 0x01, 0x00);
  //map mask register: plane 0 and 1 memory write enabled
  writeport(SEQ, 0x02, 0x03);
  //character map select register: text mode font in plane2 at 0000-1FFF offset
  writeport(SEQ, 0x03, 0x00);
  //sequencer memory mode register: enable character map selection and odd/even addressing
  writeport(SEQ, 0x04, 0x02);
}

void init_GC(){
  //set/reset register
  writeport(GC, 0x00, 0x00);
  //enable set/reset register
  writeport(GC, 0x01, 0x00);
  //color compare register
  writeport(GC, 0x02, 0x00);
  //data rotate register: input unmodified
  writeport(GC, 0x03, 0x00);
  //read map select register
  writeport(GC, 0x04, 0x00);
  //graphics mode register: odd/even addressing
  writeport(GC, 0x05, 0x10);
  //miscellaneous graphics register: B8000-BFFFF (32K region)
  writeport(GC, 0x06, 0x0E);
  //color don't care register
  writeport(GC, 0x07, 0x0F);
  //writeport(GC, 0x07, 0x00);?
  //bitmask register
  writeport(GC, 0x08, 0xFF);
}

void init_CRTC(){
  //unlock sui registri 0x00-0x07
  writeport(CRTC, 0x11, 0x0E);

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
  //cursor start register: cursor enabled
  writeport(CRTC, 0x0a, 0x0D);

  //cursor end register
  writeport(CRTC, 0x0b, 0x0E);

  //start address register: impostiamo indirizzo di partenza a A000:0000
  writeport(CRTC, 0x0c, 0x00);
  writeport(CRTC, 0x0d, 0x00);
  //cursor location register: impostiamo la partenza a 0
  writeport(CRTC, 0x0e, 0x00);
  writeport(CRTC, 0x0f, 0x00);
  //writeport(CRTC, 0x0f, 0x50);

  writeport(CRTC, 0x10, 0x9C);
  //lock sui registri 0x00-0x07
  writeport(CRTC, 0x11, 0x8E);

  writeport(CRTC, 0x12, 0x8F);
  writeport(CRTC, 0x13, 0x28);
  writeport(CRTC, 0x14, 0x1F);
  writeport(CRTC, 0x15, 0x96);
  writeport(CRTC, 0x16, 0xB9);
  //mode control register
  writeport(CRTC, 0x17, 0xA3);
  //line compare register
  writeport(CRTC, 0x18, 0xFF);
}

void init_PD(){
  //color registers
  //start index of DAC entry at 0 
  VGA_BASE[PC] = 0x0;
  //load palette into palette RAM via RGB values
  //init_PD();
  for(int i = 0; i < 64 * 3; i++){
    VGA_BASE[PD] = text_palette[i];
  }
}

void load_font(unsigned char* font_16){
  
  //reset index mode
  discard = VGA_BASE[INPUT_STATUS_REGISTER];
  //set bit PAS -> attribute controller a regime, abilita display
  VGA_BASE[AC] = 0x20;
  //reset index mode
  discard = VGA_BASE[INPUT_STATUS_REGISTER];

  //modalità di accesso sequenziale
  writeport(SEQ, 0x04, 0x06);
  //display disabilitato
  writeport(SEQ, 0x01, 0x23);
  //azzeriamo il graphics mode register
  writeport(GC, 0x05, 0x00);
  //selezioniamo zona a0000-affff e abilitiamo modalità grafica
  writeport(GC, 0x06, 0x05);
  //font da AE000 a AFFFF
  writeport(SEQ, 0x03, 0x3f);
  //selezioniamo plane 2
  writeport(SEQ, 0x02, 0x04);

  //indirizzo di partenza memoria interna VGA in cui allocare il font
  volatile void *vga_buf = (void *)(VGA_FRAMEBUFFER+0xe000*4+2);
  natl padding = 0;
  for (natl i = 0; i < 256; i++) {
    for(natl j = 0; j < 16; j++){
        memcpy((void *)(vga_buf + 64 * i + 64 * padding + 4 * j), (void*)(font_16+16 * i+j), 1);
    }
    padding++;
  }

  //rigenera registri
  writeport(SEQ, 0x04, 0x02);
  writeport(SEQ, 0x01, 0x00);
  writeport(SEQ, 0x02, 0x03);
  writeport(GC, 0x05, 0x10);
  writeport(GC, 0x06, 0x0e);
}

void init_textmode_80x25(){

  writeport(MISC, 0x00, 0x67);

  init_SEQ();
  init_CRTC();
  init_GC();
  init_AC(); 
  init_PD();

  load_font(font_16);
}

void init_graphicmode_320x200(){
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

  //reset index mode
  discard = VGA_BASE[INPUT_STATUS_REGISTER];
  //set bit PAS -> attribute controller a regime
  //abilita lo schermo
  VGA_BASE[AC] = 0x20; 

  load_palette();
}

void load_palette(){
  // Set default VGA palette
  writeport(PC, 0xff, 0x00);

  for (int i = 0; i < 256; i++) {
    writeport(PD, 0xff, (std_palette[i] & 0xff0000) >> 16);
    writeport(PD, 0xff, (std_palette[i] & 0x00ff00) >> 8);
    writeport(PD, 0xff, (std_palette[i] & 0x0000ff));
  }
}
