#ifndef ROGUETILES_ANDROID_DIRECT_H
#define ROGUETILES_ANDROID_DIRECT_H

#include <sys/stat.h>

#define _mkdir(path) mkdir((path), 0777)

#endif
