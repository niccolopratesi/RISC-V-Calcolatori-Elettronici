#include "libce.h"
#include "proc.h"
#include "costanti.h"
#include "vm.h"
#include "timer.h"

struct richiesta {
    natl d_attesa;
    richiesta *p_rich;
    des_proc *pp;
};
richiesta *p_sospesi = nullptr;

extern "C" void c_driver_td()
{
    inspronti();
    if (p_sospesi)
        p_sospesi->d_attesa--;
    while (p_sospesi && p_sospesi->d_attesa == 0) {
        inserimento_lista(pronti, p_sospesi->pp);
        richiesta *p = p_sospesi;
        p_sospesi = p_sospesi->p_rich;
        delete p;
    }
    schedulatore();
}

void inserimento_lista_attesa(richiesta *p)
{
    richiesta *r, *precedente;
    r = p_sospesi;
    precedente = nullptr;
    while (r && p->d_attesa > r->d_attesa) {
        p->d_attesa -= r->d_attesa;
        precedente = r;
        r = r->p_rich;
    }
    p->p_rich = r;
    if (precedente)
        precedente->p_rich = p;
    else
        p_sospesi = p;
    if (r)
        r->d_attesa -= p->d_attesa;
}

extern "C" void c_delay(natl n)
{
    if (!n)
        return;
    richiesta *p = new richiesta;
    p->d_attesa = n;
    p->pp = esecuzione;
    inserimento_lista_attesa(p);
    schedulatore();
}
