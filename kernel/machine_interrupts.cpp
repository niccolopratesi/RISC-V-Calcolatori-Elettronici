#include "libce.h"

extern "C" void machine_handler() {
  fpanic("interrupt a livello macchina non previsto");
}
