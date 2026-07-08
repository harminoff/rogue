/*
 * Windows compatibility shims for embedding Rogue 3.6.2 in RogueTiles.
 */

#ifndef ROGUE36_PORT_H
#define ROGUE36_PORT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef ROGUE_ANDROID
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef O_BINARY
#define O_BINARY 0
#endif
static int _fmode;
#else
#include <direct.h>
#include <io.h>
#include <process.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 260
#endif

#ifndef SIGBUS
#define SIGBUS SIGSEGV
#endif

#ifndef SIGTRAP
#define SIGTRAP SIGTERM
#endif

#ifndef SIGSYS
#define SIGSYS SIGTERM
#endif

#ifndef SIGQUIT
#define SIGQUIT SIGTERM
#endif

#ifndef SIGHUP
#define SIGHUP SIGTERM
#endif

#ifndef SIGPIPE
#define SIGPIPE SIGTERM
#endif

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

#define getuid() 0
#define getgid() 0
#define setuid(uid) 0
#define setgid(gid) 0
#ifndef ROGUE_ANDROID
#define getpid _getpid
#endif
#define kill(pid, sig) 0
#define alarm(seconds) 0
#define fork() (-1)
#define wait(status) (-1)
#define sleep(seconds) 0
#define execl(path, arg0, arg1, nullarg) (-1)
#define sbrk(increment) 0

#define getpwuid rogue36_getpwuid
#define getpass rogue36_getpass
#define index strchr

#ifndef __DJGPP__
#define __DJGPP__ 1
#endif

char *rogue36_getpass(const char *prompt);

#endif
