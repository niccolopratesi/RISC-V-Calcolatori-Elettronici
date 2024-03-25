#include "internal.h"
extern "C" void panic(const char *);
void fpanic(const char *fmt, ...)
{
        va_list ap;
        char buf[1024];
        int l;

        va_start(ap, fmt);
        l = vsnprintf(buf, 1024, fmt, ap);
        va_end(ap);

        panic(buf);
}
