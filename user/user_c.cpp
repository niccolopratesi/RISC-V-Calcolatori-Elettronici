#include "tipo.h"
//#include "libce.h"
#include "costanti.h"

char c = 'a';

extern "C" natl u_activate_p(void f(natq), natq a, natl prio, natl liv);
extern "C" void u_terminate_p();
extern "C" void func();

extern "C" void funzione(natq i) {
    func();
    u_terminate_p();
}

extern "C" void /*__attribute__((section(".main")))*/ main() {
    int a;
    int b = 13;
    c = 'b';
    
    u_activate_p(funzione, 0, MIN_PRIORITY, 0);
    u_terminate_p();
}

