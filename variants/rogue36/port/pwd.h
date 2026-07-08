#ifndef ROGUE36_PORT_PWD_H
#define ROGUE36_PORT_PWD_H

struct passwd {
    char *pw_name;
    char *pw_dir;
};

struct passwd *rogue36_getpwuid(int uid);

#endif
