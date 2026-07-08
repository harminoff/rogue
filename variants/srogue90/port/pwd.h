#ifndef SROGUE90_PORT_PWD_H
#define SROGUE90_PORT_PWD_H

struct passwd {
    char *pw_name;
    char *pw_dir;
};

struct passwd *srogue90_getpwuid(int uid);

#endif
