#ifndef SROGUE90_PORT_TERMIOS_H
#define SROGUE90_PORT_TERMIOS_H

#define VERASE 0
#define VKILL 1
#define B1200 1200

struct termios {
    unsigned char c_cc[2];
};

int srogue90_tcgetattr(int fd, struct termios *term);
int srogue90_cfgetospeed(const struct termios *term);

#define tcgetattr srogue90_tcgetattr
#define cfgetospeed srogue90_cfgetospeed

#endif
