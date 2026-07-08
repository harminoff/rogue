#ifndef ROGUETILES_ANDROID_PROCESS_H
#define ROGUETILES_ANDROID_PROCESS_H

#include <stdarg.h>

#ifndef P_WAIT
#define P_WAIT 0
#endif

static int
spawnl(int mode, const char *path, const char *arg0, ...)
{
    (void) mode;
    (void) path;
    (void) arg0;
    return -1;
}

#endif
