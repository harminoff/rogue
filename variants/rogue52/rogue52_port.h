/*
 * Windows compatibility shims for embedding Rogue 5.2.1 in RogueTiles.
 */

#ifndef ROGUE52_PORT_H
#define ROGUE52_PORT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <direct.h>
#include <io.h>
#include <process.h>

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

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

#define getuid() 0
#define getgid() 0
#define setuid(uid) 0
#define setgid(gid) 0
#define getpid _getpid
#define kill(pid, sig) 0
#define alarm(seconds) 0

#define getpwuid rogue52_getpwuid
#define getpass rogue52_getpass
#define index strchr

#ifndef __DJGPP__
#define __DJGPP__ 1
#endif

char *rogue52_getpass(const char *prompt);

#endif
