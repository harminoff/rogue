/*
 * Windows compatibility and RogueTiles bridge helpers for Rogue 3.6.2.
 */

#include "rogue36_port.h"
#include <curses.h>
#include "port/pwd.h"
#include "port/termios.h"
#include "rogue.h"

static char
window_ch(WINDOW *win, int y, int x)
{
    chtype ch;

    if (win == NULL || y < 0 || x < 0 || y >= LINES || x >= COLS)
	return ' ';
    ch = mvwinch(win, y, x);
    if (ch == (chtype) ERR)
	return ' ';
    return (char)(ch & A_CHARTEXT);
}

struct passwd *
rogue36_getpwuid(int uid)
{
    static struct passwd pw;
    static char name[64];
    static char home_dir[PATH_MAX];
    char *env;

    (void) uid;

    env = getenv("USERNAME");
    if (env == NULL || env[0] == '\0')
	env = "player";
    strncpy(name, env, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';

    env = getenv("USERPROFILE");
    if (env == NULL || env[0] == '\0')
	env = ".";
    strncpy(home_dir, env, sizeof(home_dir) - 2);
    home_dir[sizeof(home_dir) - 2] = '\0';

    pw.pw_name = name;
    pw.pw_dir = home_dir;
    return &pw;
}

char *
rogue36_getpass(const char *prompt)
{
    static char password[128];

    if (prompt != NULL)
	fputs(prompt, stdout);
    if (fgets(password, sizeof(password), stdin) == NULL)
	password[0] = '\0';
    password[strcspn(password, "\r\n")] = '\0';
    return password;
}

int
rogue36_tcgetattr(int fd, struct termios *term)
{
    (void) fd;
    if (term != NULL)
    {
	term->c_cc[VERASE] = '\b';
	term->c_cc[VKILL] = 21;
    }
    return 0;
}

int
rogue36_cfgetospeed(const struct termios *term)
{
    (void) term;
    return 9600;
}

int
rogue36_bridge_hero_y(void)
{
    return hero.y;
}

int
rogue36_bridge_hero_x(void)
{
    return hero.x;
}

int
rogue36_bridge_level(void)
{
    return level;
}

int
rogue36_bridge_map_rows(void)
{
    return LINES;
}

int
rogue36_bridge_map_cols(void)
{
    return COLS;
}

int
rogue36_bridge_gold(void)
{
    return purse;
}

int
rogue36_bridge_hp(void)
{
    return pstats.s_hpt;
}

int
rogue36_bridge_max_hp(void)
{
    return max_hp;
}

unsigned int
rogue36_bridge_strength(void)
{
    return (unsigned int) pstats.s_str.st_str;
}

int
rogue36_bridge_armor(void)
{
    return cur_armor != NULL ? cur_armor->o_ac : pstats.s_arm;
}

int
rogue36_bridge_exp_level(void)
{
    return pstats.s_lvl;
}

long
rogue36_bridge_exp_points(void)
{
    return pstats.s_exp;
}

int
rogue36_bridge_hungry_state(void)
{
    return hungry_state;
}

const char *
rogue36_bridge_message(void)
{
    return huh;
}

char
rogue36_bridge_visible_ch(int y, int x)
{
    return window_ch(cw, y, x);
}

char
rogue36_bridge_terrain_ch(int y, int x)
{
    return window_ch(stdscr, y, x);
}

char
rogue36_bridge_monster_window_ch(int y, int x)
{
    return window_ch(mw, y, x);
}

void *
rogue36_bridge_monster_at(int y, int x)
{
    struct linked_list *item;

    item = find_mons(y, x);
    return item == NULL ? NULL : (void *) ldata(item);
}

int
rogue36_bridge_player_is_blind(void)
{
    return on(player, ISBLIND);
}

int
rogue36_bridge_cansee(int y, int x)
{
    return cansee(y, x);
}

int
rogue36_bridge_see_monst(void *monster)
{
    if (monster == NULL)
	return FALSE;
    if (on(*(struct thing *) monster, ISINVIS) && off(player, CANSEE))
	return FALSE;
    return TRUE;
}

char
rogue36_bridge_monster_type(void *monster)
{
    return monster == NULL ? '\0' : ((struct thing *) monster)->t_type;
}

char
rogue36_bridge_monster_disguise(void *monster)
{
    return monster == NULL ? '\0' : ((struct thing *) monster)->t_disguise;
}

char
rogue36_bridge_object_type_at(int y, int x)
{
    struct linked_list *item;
    struct object *obj;

    for (item = lvl_obj; item != NULL; item = next(item))
    {
	obj = (struct object *) ldata(item);
	if (obj->o_pos.y == y && obj->o_pos.x == x)
	    return obj->o_type;
    }

    return '\0';
}
