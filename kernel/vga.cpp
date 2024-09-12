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

/*
//standard vga ports
#define AC 0x3c0            //attribute or palette registers                       
#define AC_READ 0x3c1       //attribute read register                            
#define MISC 0x3c2          //miscellaneous register                                       
#define MISC_READ 0x3cc     //miscellaneous read register                      
#define SEQ 0x3c4           //sequencer                                    
#define GC 0x3ce            //graphic address register                     
#define CRTC 0x3d4          //cathode ray tube controller address register
#define PC 0x3c8            //PEL write index                                
#define PD 0x3c9            //PEL write data                                           
#define INPUT_STATUS_REGISTER 0x3da
*/

#define VGA_TEXT_HEIGHT 25
#define VGA_TEXT_WIDTH  80

//remapped VGA ports into qemu
#define AC                    0x400           //attribute or palette registers              
#define AC_READ               0x401           //attribute read register                     
#define MISC                  0x402           //miscellaneous register                        
#define MISC_READ             0x40c           //miscellaneous read register                 
#define SEQ                   0x404           //sequencer                                     
#define GC                    0x40e           //graphic address register                      
#define CRTC                  0x414           //cathode ray tube controller address register  
#define PC                    0x408           //PEL write index                               
#define PD                    0x409           //PEL write data                              
#define INPUT_STATUS_REGISTER 0x41a


natb readport(natl port, natb index);
void writeport(natl port, natb index, natb val);

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
void print_VGA(char *message, natb attr);
void clear_screen(natb fg, natb bg);

// write to this to discard data
volatile natb __attribute__((unused)) discard; 
// vga framebuffer
char *vga_buf;

//memory mapped IO ports base
static volatile natb *const VGA_BASE = (natb *)VGA_MMIO_PORTS;

static inline natw encode_char(char c, natb fg, natb bg) {
  return ((bg & 0xf) << 4 | (fg & 0xf)) << 8 | c;
}


// static inline void memcpy(void *dest, const void *src,natl n){
//   unsigned char *d = (unsigned char *)dest;
//   const unsigned char *s = (unsigned char *)src;

//   for (natl i = 0; i < n; i++) {
//     *(d + i) = *(s + i);
//   }
// }

extern "C" void vga_init() {

  init_textmode_80x25();

  //scritture a 0xa0000 colorano la cella del carattere
  volatile natb *p = (natb *)(VGA_FRAMEBUFFER);

  // int j=0;
  // for(int i=0;i<2000;i++){
  //   p[4*i]=j;
  //   p[4*i+1]=0x0f;
  //   j++;
  // }

//   for(int i=4;i<300;i+=2){
//     if(i==8){
// p[i] = 0x30;
//     p[i+1] = 0x40; 
//     }else{
// p[i] = 0x30;
//     p[i+1] = 0x0f; 
//     }
     //background rosso
//}

  // //scritture a 0xb8000 vengono ignorate, perché?
  // p = (natb *)(VGA_FRAMEBUFFER+0x18000);
  // p[0] = 0x00;
  // p[1] = 0x0f;
  // for(int i=2;i<300*200;i+=2){
  //   p[i] = 0x10;
  //   p[i+1] = 0x0f;
  // }

  //0x0F = bright white on black background
  //clear_screen(0x00,0x0F);

  print_VGA("Hello RISC-V world!  jefjeifiewfj weifjwi deidjwei kldkdkddk dkdkdkkkd pippo puppa\n", 0x0f);



  // //modalità grafica
  // init_graphicmode_320x200();

  // //test graphic mode
  // //metà schermo rosso e metà schermo verde
  // //sia verticalmente che orizzontalmente

  // for(int i=0; i < 320*100;i++){
  //   p[i] = 0xfe;
  // }
  // for(int i=320*100; i < 320*200;i++){
  //   p[i] = 0xfd;
  // }


  // for(int i=0; i < 200;i++){
  //   for(int j=0; j < 320;j++){
  //     if(j<1 || j==2 || j==319){

  //       p[i*320 + j] = 0xfd;
  //     }else{
  //       p[i*320 + j] = 0xfe;
  //     }
  //   }
  // }

  flog(LOG_INFO,"VGA inizializzata");
}

void clear_screen(natb bg, natb fg){
  volatile natb *p = (natb *)(VGA_FRAMEBUFFER | (0xb8000 - 0xa0000));// 0xb8000

  fg &= 0xf;

  natb attribute = (bg << 4) | fg;

  for(int i=0;i<VGA_TEXT_WIDTH*VGA_TEXT_HEIGHT;i+=2){
    p[i] = ' ';
    p[i+1] = attribute;
  }
}

natb readport(natl port, natb index) {
  natb read;

  switch (port) {
    case AC:
      //reset to index mode
      discard = VGA_BASE[INPUT_STATUS_REGISTER];
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
  discard = VGA_BASE[INPUT_STATUS_REGISTER];
  return read;
}

void writeport(natl port, natb index, natb val) {

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
    //-- POSSIBILE OTTIMIZZARE IN CASCATA SEQ GC CRTC --
    case SEQ:
      //sequencer register riceve prima l'indice nell'address register
      //poi scrive il dato nel data register (1 byte successivo)
      VGA_BASE[port] = index;
      VGA_BASE[port + 1] = val;
      break;
    case GC:
      //graphics register riceve prima l'indice nell'address register
      //poi scrive il dato nel data register (1 byte successivo)
      VGA_BASE[port] = index;
      VGA_BASE[port + 1] = val;
      break;
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
  //color registers
  //start index of DAC entry at 0 
  VGA_BASE[PC] = 0x0;
  //load palette into palette RAM via RGB values
  init_PD();

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

  volatile void *vga_buf = (void *)(VGA_FRAMEBUFFER+0xe000*4+2); // 0xa0000
  natl padding = 0;
  for (natl i = 0; i < 256; i++) {
    //memcpy((void *)(vga_buf + 32 * i), (void*)(font_16+16 * i), 16);
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

void print_VGA(char *message, natb attr)
{
  volatile natb *p = (natb*)(VGA_FRAMEBUFFER);
  // posizione attuale del cursore
  int cursor = readport(CRTC, 0x0f);
  cursor = (cursor+1)*2 -2; // dove scrivere
  int j = cursor*4;
  while(*message){
    
    if(*message !=  '\0' && *message !=  '\n' ){
      p[j] = *message;
      p[j+1] = attr;
      j+=4;
      cursor+=2;
    }
    else{
      //andare a capo
      // int line_offset = cursor%(VGA_TEXT_WIDTH*2);
      // cursor = cursor + VGA_TEXT_WIDTH*2 - line_offset;
    }
    message++;
  }

  //riposiziono il cursore su uno spazio vuoto
  p[j] = 0x00;
  p[j+1] = attr;
  cursor+=2;
  // nuova posizione del cursore
  writeport(CRTC, 0x0e, (cursor/2 -1) >> 8);
  writeport(CRTC, 0x0f, (cursor/2 -1) & 0xff);
}

void init_textmode_80x25(){

  writeport(MISC, 0x00, 0x67);

  init_SEQ();
  init_CRTC();
  init_GC();
  init_AC(); 

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

  // Enable display
  //writeport(AC, 0xff, 0x20);
  //reset index mode
  discard = VGA_BASE[INPUT_STATUS_REGISTER];
  //set bit PAS -> attribute controller a regime
  //abilita lo schermo
  VGA_BASE[AC] = 0x20; 


  load_palette();
}

void load_palette(){
  // configura una VGA palette personalizzata
  // for (int i = 0; i < 256; i++) {
  //   std_palette[i] = 0;
  //   std_palette[i] |= ((i & 0xc0)) << 16;
  //   std_palette[i] |= ((i & 0x38) << 2) << 8;
  //   std_palette[i] |= ((i & 0x07) << 5);
  // }
  // std_palette[255] = 0xfcfcfc;



  // Set default VGA palette
  writeport(PC, 0xff, 0x00);
  // for (int i = 0; i < 256; i++) {
  //   writeport(PD, 0xff, (std_palette[i] & 0xfc0000) >> 18);
  //   writeport(PD, 0xff, (std_palette[i] & 0x00fc00) >> 10);
  //   writeport(PD, 0xff, (std_palette[i] & 0x0000fc) >> 2);
  // }

  for (int i = 0; i < 256; i++) {
    writeport(PD, 0xff, (std_palette[i] & 0xff0000) >> 16);
    writeport(PD, 0xff, (std_palette[i] & 0x00ff00) >> 8);
    writeport(PD, 0xff, (std_palette[i] & 0x0000ff));
  }
}
