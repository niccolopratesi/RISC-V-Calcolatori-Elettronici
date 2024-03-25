#include "vid.h"

namespace vid {

volatile natw* video = (natw*)(0x50000000 | (0xb8000 - 0xa0000));

}
