#include "tipo.h"

extern "C" void writeSTVEC(void*);
extern "C" void writeSEPC(void*);
extern "C" natq readSEPC();
extern "C" natq readSCAUSE();
extern "C" natq readSTVAL();
extern "C" natq readSTVEC();
extern "C" natq readSSTATUS();
extern "C" paddr readSATP();
extern "C" void writeSATP_asm(natq);
extern "C" void writeSATP(paddr);