#include <stdio.h>
#include <stdarg.h>
#include "mesh.h"

int dbg_printf(const char *fmt, ...) {
    if (config.debug == 0) return 0;

    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return n;
}
