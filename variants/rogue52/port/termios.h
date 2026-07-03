#ifndef ROGUE52_PORT_TERMIOS_H
#define ROGUE52_PORT_TERMIOS_H

#define VERASE 0
#define VKILL 1
#define B1200 1200

struct termios {
    unsigned char c_cc[2];
};

int rogue52_tcgetattr(int fd, struct termios *term);
int rogue52_cfgetospeed(const struct termios *term);

#define tcgetattr rogue52_tcgetattr
#define cfgetospeed rogue52_cfgetospeed

#endif
