#ifndef ROGUETILES_ANDROID_CURSES_H
#define ROGUETILES_ANDROID_CURSES_H

#include <stdarg.h>

#ifndef __cplusplus
#ifndef bool
typedef int bool;
#endif
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define OK 0
#define ERR (-1)

#define A_CHARTEXT 0x00ff
#define A_STANDOUT 0x0100

#define KEY_LEFT 0404
#define KEY_RIGHT 0405
#define KEY_UP 0403
#define KEY_DOWN 0402
#define KEY_HOME 0406
#define KEY_PPAGE 0523
#define KEY_NPAGE 0522
#define KEY_END 0550
#define KEY_A1 0534
#define KEY_A3 0535
#define KEY_B2 0542
#define KEY_C1 0540
#define KEY_C3 0541
#define KEY_BACKSPACE 0407

typedef unsigned int chtype;

typedef struct android_curses_window {
    int rows;
    int cols;
    int begin_y;
    int begin_x;
    int cur_y;
    int cur_x;
    int standout;
    int owns_cells;
    chtype *cells;
} WINDOW;

#define _cury cur_y
#define _curx cur_x

extern int LINES;
extern int COLS;
extern int ESCDELAY;
extern WINDOW *stdscr;
extern WINDOW *curscr;

WINDOW *initscr(void);
int endwin(void);
int isendwin(void);
WINDOW *newwin(int rows, int cols, int y, int x);
WINDOW *subwin(WINDOW *orig, int rows, int cols, int y, int x);
int delwin(WINDOW *win);
int mvwin(WINDOW *win, int y, int x);
int wmove(WINDOW *win, int y, int x);
int move(int y, int x);
int waddch(WINDOW *win, chtype ch);
int addch(chtype ch);
chtype inch(void);
chtype mvinch(int y, int x);
int mvwaddch(WINDOW *win, int y, int x, chtype ch);
int mvaddch(int y, int x, chtype ch);
int waddstr(WINDOW *win, const char *str);
int addstr(const char *str);
int mvwaddstr(WINDOW *win, int y, int x, const char *str);
int mvaddstr(int y, int x, const char *str);
int wprintw(WINDOW *win, const char *fmt, ...);
int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...);
int printw(const char *fmt, ...);
int mvprintw(int y, int x, const char *fmt, ...);
int wclear(WINDOW *win);
int werase(WINDOW *win);
int clear(void);
int erase(void);
int wrefresh(WINDOW *win);
int refresh(void);
int touchwin(WINDOW *win);
int clearok(WINDOW *win, int flag);
int leaveok(WINDOW *win, int flag);
int idlok(WINDOW *win, int flag);
int keypad(WINDOW *win, int flag);
int noecho(void);
int echo(void);
int raw(void);
int cbreak(void);
int crmode(void);
int nocbreak(void);
int nocrmode(void);
int halfdelay(int tenths);
int flushinp(void);
int erasechar(void);
int killchar(void);
int baudrate(void);
int getmaxx(WINDOW *win);
int getmaxy(WINDOW *win);
int wgetnstr(WINDOW *win, char *str, int n);
int mvcur(int oldrow, int oldcol, int newrow, int newcol);
int wstandout(WINDOW *win);
int wstandend(WINDOW *win);
int standout(void);
int standend(void);
int overlay(WINDOW *src, WINDOW *dst);
int overwrite(WINDOW *src, WINDOW *dst);
int resize_term(int rows, int cols);
chtype winch(WINDOW *win);
chtype mvwinch(WINDOW *win, int y, int x);
int getch(void);
int wgetch(WINDOW *win);
char *unctrl(chtype ch);
char *termname(void);
int clrtoeol(void);
int wclrtoeol(WINDOW *win);
int clrtobot(void);
char *getpass(const char *prompt);
int getloadavg(double *loadavg, int nelem);

#define getyx(win, y, x) do { (y) = (win)->cur_y; (x) = (win)->cur_x; } while (0)

#endif
