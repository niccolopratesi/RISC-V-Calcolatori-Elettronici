#include "libce.h"

extern "C" void init_gdt();
extern "C" void load_gdt();
extern "C" void unload_gdt();
extern "C" void init_idt();
extern "C" void idt_reset();
void itostr(char *buf, unsigned int len, long l);
void htostr(char *buf, unsigned long long l, int cifre = 8);
char* convnat(natl l, char* out, int cifre);
char *strncpy(char *dest, const char *src, natl l);
extern "C" void cli();
extern "C" void sti();
extern "C" void panic(const char *msg);
extern "C" void ctors();
extern "C" void dtors();
