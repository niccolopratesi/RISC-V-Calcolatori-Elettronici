#include "libce.h"
#include "vid.h"

namespace vid{

    natb readport(natl port, natb index) {
        natb read;
        // write to this to discard data
        volatile natb __attribute__((unused)) discard;

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
}

