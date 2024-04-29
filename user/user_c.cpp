#include "tipo.h"
#include "libce.h"
#include "costanti.h"

extern "C" natl u_activate_p(void f(natq), natq a, natl prio, natl liv);
extern "C" void u_terminate_p();
extern "C" natq get_addr_funzione();
extern "C" void func();

extern "C" void funzione(natq i) {
    func();
    u_terminate_p();
}

int __attribute__((constructor)) funzione2() {
    return 2;
}

extern "C" void __attribute__((section(".main"))) main() {
    int a;
    int b = 13;
    // c = '\0';
    // funzione(1);
    // func();
    //for(;;);
    void (*f)(natq) = ptr_cast<void(natq)>(get_addr_funzione());
    u_activate_p(f, 0, MIN_PRIORITY, 0);
    u_terminate_p();
}

