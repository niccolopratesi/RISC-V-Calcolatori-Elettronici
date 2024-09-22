#include "libce.h"
#include "vid.h"

namespace vid{
    
    void writeport(natl port, natb index, natb val) {
        // write to this to discard data
        volatile natb __attribute__((unused)) discard;

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
}