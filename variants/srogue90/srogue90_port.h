/*
 * Windows compatibility shims for embedding Super-Rogue 9.0.1 in RogueTiles.
 */

#ifndef SROGUE90_PORT_H
#define SROGUE90_PORT_H

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

#ifndef NSIG
#define NSIG 32
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
#define sleep(seconds) 0
#define fork() (-1)
#define wait(status) (-1)
#define execl(path, arg0, arg1, nullarg) (-1)
#define sbrk(increment) 0

#define getpwuid srogue90_getpwuid
#define getpass srogue90_getpass
#define index strchr

#ifndef __DJGPP__
#define __DJGPP__ 1
#endif

void srogue90_srand48(long seed);
long srogue90_lrand48(void);
char *srogue90_getpass(const char *prompt);

#define srand48 srogue90_srand48
#define lrand48 srogue90_lrand48

#endif
