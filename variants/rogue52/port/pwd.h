#ifndef ROGUE52_PORT_PWD_H
#define ROGUE52_PORT_PWD_H

struct passwd {
    char *pw_name;
    char *pw_dir;
};

struct passwd *rogue52_getpwuid(int uid);

#endif
