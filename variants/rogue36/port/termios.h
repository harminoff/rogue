#ifndef ROGUE36_PORT_TERMIOS_H
#define ROGUE36_PORT_TERMIOS_H

#define VERASE 0
#define VKILL 1
#define B1200 1200

struct termios {
    unsigned char c_cc[2];
};

int rogue36_tcgetattr(int fd, struct termios *term);
int rogue36_cfgetospeed(const struct termios *term);

#define tcgetattr rogue36_tcgetattr
#define cfgetospeed rogue36_cfgetospeed

#endif
