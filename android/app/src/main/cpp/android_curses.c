#include "curses.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "frontend.h"

#define ANDROID_CURSES_ROWS 24
#define ANDROID_CURSES_COLS 80

int LINES = ANDROID_CURSES_ROWS;
int COLS = ANDROID_CURSES_COLS;
int ESCDELAY = 64;
char *CE = NULL;
WINDOW *stdscr = NULL;
WINDOW *curscr = NULL;

static WINDOW main_screen;
static chtype main_cells[ANDROID_CURSES_ROWS * ANDROID_CURSES_COLS];
static int ended = 1;

static int
valid_cell(WINDOW *win, int y, int x)
{
    return (win != NULL && y >= 0 && y < win->rows && x >= 0 && x < win->cols);
}

static chtype *
cell_at(WINDOW *win, int y, int x)
{
    int screen_y;
    int screen_x;

    if (!valid_cell(win, y, x))
	return NULL;
    if (win->owns_cells)
	return &win->cells[y * win->cols + x];
    if (win->cells == main_cells)
	return &win->cells[y * win->cols + x];

    screen_y = win->begin_y + y;
    screen_x = win->begin_x + x;
    if (screen_y < 0 || screen_y >= LINES || screen_x < 0 || screen_x >= COLS)
	return NULL;
    return &main_cells[screen_y * COLS + screen_x];
}

static void
window_fill(WINDOW *win, chtype ch)
{
    int y, x;
    chtype *cell;

    if (win == NULL)
	return;
    for (y = 0; y < win->rows; y++)
	for (x = 0; x < win->cols; x++)
	{
	    cell = cell_at(win, y, x);
	    if (cell != NULL)
		*cell = ch;
	}
    win->cur_y = 0;
    win->cur_x = 0;
}

WINDOW *
initscr(void)
{
    main_screen.rows = LINES;
    main_screen.cols = COLS;
    main_screen.begin_y = 0;
    main_screen.begin_x = 0;
    main_screen.cur_y = 0;
    main_screen.cur_x = 0;
    main_screen.standout = 0;
    main_screen.owns_cells = 0;
    main_screen.cells = main_cells;
    stdscr = &main_screen;
    curscr = &main_screen;
    ended = 0;
    window_fill(stdscr, ' ');
    return stdscr;
}

int
endwin(void)
{
    ended = 1;
    return OK;
}

int
isendwin(void)
{
    return ended;
}

WINDOW *
newwin(int rows, int cols, int y, int x)
{
    WINDOW *win;

    if (rows <= 0)
	rows = LINES;
    if (cols <= 0)
	cols = COLS;
    win = (WINDOW *) calloc(1, sizeof(*win));
    if (win == NULL)
	return NULL;
    win->rows = rows;
    win->cols = cols;
    win->begin_y = y;
    win->begin_x = x;
    win->owns_cells = 1;
    win->cells = (chtype *) calloc((size_t) rows * (size_t) cols,
				   sizeof(*win->cells));
    if (win->cells == NULL)
    {
	free(win);
	return NULL;
    }
    window_fill(win, ' ');
    return win;
}

WINDOW *
subwin(WINDOW *orig, int rows, int cols, int y, int x)
{
    (void) orig;
    return newwin(rows, cols, y, x);
}

int
delwin(WINDOW *win)
{
    if (win != NULL && win != &main_screen)
    {
	if (win->owns_cells)
	    free(win->cells);
	free(win);
    }
    return OK;
}

int
mvwin(WINDOW *win, int y, int x)
{
    if (win == NULL)
	return ERR;
    win->begin_y = y;
    win->begin_x = x;
    return OK;
}

int
wmove(WINDOW *win, int y, int x)
{
    if (!valid_cell(win, y, x))
	return ERR;
    win->cur_y = y;
    win->cur_x = x;
    return OK;
}

int
move(int y, int x)
{
    return wmove(stdscr, y, x);
}

int
waddch(WINDOW *win, chtype ch)
{
    chtype *cell;

    if (win == NULL)
	return ERR;
    cell = cell_at(win, win->cur_y, win->cur_x);
    if (cell == NULL)
	return ERR;
    *cell = (ch & A_CHARTEXT) | (win->standout ? A_STANDOUT : 0);
    if (++win->cur_x >= win->cols)
    {
	win->cur_x = 0;
	if (win->cur_y < win->rows - 1)
	    win->cur_y++;
    }
    return OK;
}

int
addch(chtype ch)
{
    return waddch(stdscr, ch);
}

chtype
inch(void)
{
    return winch(stdscr);
}

chtype
mvinch(int y, int x)
{
    return mvwinch(stdscr, y, x);
}

int
mvwaddch(WINDOW *win, int y, int x, chtype ch)
{
    if (wmove(win, y, x) == ERR)
	return ERR;
    return waddch(win, ch);
}

int
mvaddch(int y, int x, chtype ch)
{
    return mvwaddch(stdscr, y, x, ch);
}

int
waddstr(WINDOW *win, const char *str)
{
    if (str == NULL)
	return OK;
    while (*str != '\0')
	if (waddch(win, (unsigned char) *str++) == ERR)
	    return ERR;
    return OK;
}

int
addstr(const char *str)
{
    return waddstr(stdscr, str);
}

int
mvwaddstr(WINDOW *win, int y, int x, const char *str)
{
    if (wmove(win, y, x) == ERR)
	return ERR;
    return waddstr(win, str);
}

int
mvaddstr(int y, int x, const char *str)
{
    return mvwaddstr(stdscr, y, x, str);
}

static int
vwprintw(WINDOW *win, const char *fmt, va_list args)
{
    char buffer[512];

    vsnprintf(buffer, sizeof(buffer), fmt, args);
    buffer[sizeof(buffer) - 1] = '\0';
    return waddstr(win, buffer);
}

int
wprintw(WINDOW *win, const char *fmt, ...)
{
    int result;
    va_list args;

    va_start(args, fmt);
    result = vwprintw(win, fmt, args);
    va_end(args);
    return result;
}

int
mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...)
{
    int result;
    va_list args;

    if (wmove(win, y, x) == ERR)
	return ERR;
    va_start(args, fmt);
    result = vwprintw(win, fmt, args);
    va_end(args);
    return result;
}

int
printw(const char *fmt, ...)
{
    int result;
    va_list args;

    va_start(args, fmt);
    result = vwprintw(stdscr, fmt, args);
    va_end(args);
    return result;
}

int
mvprintw(int y, int x, const char *fmt, ...)
{
    int result;
    va_list args;

    if (move(y, x) == ERR)
	return ERR;
    va_start(args, fmt);
    result = vwprintw(stdscr, fmt, args);
    va_end(args);
    return result;
}

int
wclear(WINDOW *win)
{
    window_fill(win, ' ');
    return OK;
}

int
werase(WINDOW *win)
{
    return wclear(win);
}

int
clear(void)
{
    return wclear(stdscr);
}

int
erase(void)
{
    return clear();
}

int
wrefresh(WINDOW *win)
{
    (void) win;
    rogue_frontend_render();
    return OK;
}

int
refresh(void)
{
    return wrefresh(stdscr);
}

int touchwin(WINDOW *win) { (void) win; return OK; }
int clearok(WINDOW *win, int flag) { (void) win; (void) flag; return OK; }
int leaveok(WINDOW *win, int flag) { (void) win; (void) flag; return OK; }
int idlok(WINDOW *win, int flag) { (void) win; (void) flag; return OK; }
int keypad(WINDOW *win, int flag) { (void) win; (void) flag; return OK; }
int noecho(void) { return OK; }
int echo(void) { return OK; }
int raw(void) { return OK; }
int cbreak(void) { return OK; }
int crmode(void) { return cbreak(); }
int nocbreak(void) { return OK; }
int nocrmode(void) { return nocbreak(); }
int halfdelay(int tenths) { (void) tenths; return OK; }
int flushinp(void) { return OK; }
int erasechar(void) { return '\b'; }
int killchar(void) { return 21; }
int baudrate(void) { return 9600; }
int getmaxx(WINDOW *win) { return win != NULL ? win->cols : COLS; }
int getmaxy(WINDOW *win) { return win != NULL ? win->rows : LINES; }
int wgetnstr(WINDOW *win, char *str, int n)
{
    int i;
    int ch;

    (void) win;
    if (str == NULL || n <= 0)
	return ERR;
    for (i = 0; i < n; i++)
    {
	ch = getch();
	if (ch == '\n' || ch == '\r' || ch == ERR)
	    break;
	str[i] = (char) ch;
    }
    str[i] = '\0';
    return OK;
}
int mvcur(int oldrow, int oldcol, int newrow, int newcol)
{
    (void) oldrow;
    (void) oldcol;
    return move(newrow, newcol);
}

int
wstandout(WINDOW *win)
{
    if (win != NULL)
	win->standout = 1;
    return OK;
}

int
wstandend(WINDOW *win)
{
    if (win != NULL)
	win->standout = 0;
    return OK;
}

int standout(void) { return wstandout(stdscr); }
int standend(void) { return wstandend(stdscr); }

static int
copy_window_cells(WINDOW *src, WINDOW *dst, int skip_spaces)
{
    int y, x;
    int rows, cols;
    chtype ch;

    if (src == NULL || dst == NULL)
	return ERR;
    rows = src->rows < dst->rows ? src->rows : dst->rows;
    cols = src->cols < dst->cols ? src->cols : dst->cols;
    for (y = 0; y < rows; y++)
	for (x = 0; x < cols; x++)
	{
	    ch = mvwinch(src, y, x);
	    if (!skip_spaces || (ch & A_CHARTEXT) != ' ')
		mvwaddch(dst, y, x, ch);
	}
    return OK;
}

int overlay(WINDOW *src, WINDOW *dst) { return copy_window_cells(src, dst, TRUE); }
int overwrite(WINDOW *src, WINDOW *dst) { return copy_window_cells(src, dst, FALSE); }
int resize_term(int rows, int cols) { (void) rows; (void) cols; return OK; }

chtype
winch(WINDOW *win)
{
    chtype *cell;

    if (win == NULL)
	return ' ';
    cell = cell_at(win, win->cur_y, win->cur_x);
    return cell != NULL ? *cell : ' ';
}

chtype
mvwinch(WINDOW *win, int y, int x)
{
    if (wmove(win, y, x) == ERR)
	return ' ';
    return winch(win);
}

int
getch(void)
{
    return rogue_frontend_readchar();
}

int
wgetch(WINDOW *win)
{
    (void) win;
    return getch();
}

char *
unctrl(chtype ch)
{
    static char text[8];
    unsigned char c = (unsigned char)(ch & A_CHARTEXT);

    if (c < 32)
    {
	text[0] = '^';
	text[1] = (char)(c + '@');
	text[2] = '\0';
    }
    else
    {
	text[0] = (char)c;
	text[1] = '\0';
    }
    return text;
}

char *
termname(void)
{
    return "xterm";
}

int
wclrtoeol(WINDOW *win)
{
    int x;

    if (win == NULL)
	return ERR;
    for (x = win->cur_x; x < win->cols; x++)
	mvwaddch(win, win->cur_y, x, ' ');
    return OK;
}

int clrtoeol(void) { return wclrtoeol(stdscr); }

int
clrtobot(void)
{
    int y;

    if (stdscr == NULL)
	return ERR;
    wclrtoeol(stdscr);
    for (y = stdscr->cur_y + 1; y < stdscr->rows; y++)
    {
	wmove(stdscr, y, 0);
	wclrtoeol(stdscr);
    }
    return OK;
}

char *
getpass(const char *prompt)
{
    static char password[1];

    (void) prompt;
    password[0] = '\0';
    return password;
}

int
getloadavg(double *loadavg, int nelem)
{
    int i;

    if (loadavg == NULL || nelem <= 0)
	return 0;
    for (i = 0; i < nelem; i++)
	loadavg[i] = 0.0;
    return nelem;
}
