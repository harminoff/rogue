/*
 * Windows compatibility and RogueTiles bridge helpers for Super-Rogue 9.0.1.
 */

#include "srogue90_port.h"
#include <curses.h>
#include "port/pwd.h"
#include "port/termios.h"
#include "rogue.h"
#include "rogue.ext"

static unsigned long srogue90_rng_state = 1;

extern int updpack();

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
srogue90_getpwuid(int uid)
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
srogue90_getpass(const char *prompt)
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
srogue90_tcgetattr(int fd, struct termios *term)
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
srogue90_cfgetospeed(const struct termios *term)
{
    (void) term;
    return 9600;
}

void
srogue90_srand48(long seed)
{
    srogue90_rng_state = (unsigned long) seed;
    if (srogue90_rng_state == 0)
	srogue90_rng_state = 1;
}

long
srogue90_lrand48(void)
{
    srogue90_rng_state = srogue90_rng_state * 1103515245UL + 12345UL;
    return (long)((srogue90_rng_state >> 1) & 0x7fffffffUL);
}

int
srogue90_bridge_hero_y(void)
{
    return player.t_pos.y;
}

int
srogue90_bridge_hero_x(void)
{
    return player.t_pos.x;
}

int
srogue90_bridge_level(void)
{
    return level;
}

int
srogue90_bridge_map_rows(void)
{
    return LINES > 1 ? LINES - 1 : LINES;
}

int
srogue90_bridge_status_row_start(void)
{
    return LINES > 3 ? LINES - 3 : LINES;
}

int
srogue90_bridge_map_cols(void)
{
    return COLS;
}

int
srogue90_bridge_gold(void)
{
    return purse;
}

int
srogue90_bridge_hp(void)
{
    return player.t_stats.s_hpt;
}

int
srogue90_bridge_max_hp(void)
{
    return player.t_stats.s_maxhp;
}

unsigned int
srogue90_bridge_strength(void)
{
    return (unsigned int) player.t_stats.s_ef.a_str;
}

unsigned int
srogue90_bridge_strength_base(void)
{
    return (unsigned int) player.t_stats.s_re.a_str;
}

unsigned int
srogue90_bridge_dexterity(void)
{
    return (unsigned int) player.t_stats.s_ef.a_dex;
}

unsigned int
srogue90_bridge_dexterity_base(void)
{
    return (unsigned int) player.t_stats.s_re.a_dex;
}

unsigned int
srogue90_bridge_wisdom(void)
{
    return (unsigned int) player.t_stats.s_ef.a_wis;
}

unsigned int
srogue90_bridge_wisdom_base(void)
{
    return (unsigned int) player.t_stats.s_re.a_wis;
}

unsigned int
srogue90_bridge_constitution(void)
{
    return (unsigned int) player.t_stats.s_ef.a_con;
}

unsigned int
srogue90_bridge_constitution_base(void)
{
    return (unsigned int) player.t_stats.s_re.a_con;
}

int
srogue90_bridge_carry_weight(void)
{
    updpack();
    return player.t_stats.s_pack / 10;
}

int
srogue90_bridge_carry_capacity(void)
{
    updpack();
    return player.t_stats.s_carry / 10;
}

int
srogue90_bridge_volume_percent(void)
{
    updpack();
    return (packvol * 100) / V_PACK;
}

int
srogue90_bridge_armor(void)
{
    return cur_armor != NULL ? cur_armor->o_ac : player.t_stats.s_arm;
}

int
srogue90_bridge_exp_level(void)
{
    return player.t_stats.s_lvl;
}

long
srogue90_bridge_exp_points(void)
{
    return player.t_stats.s_exp;
}

int
srogue90_bridge_hungry_state(void)
{
    return hungry_state;
}

const char *
srogue90_bridge_message(void)
{
    return huh;
}

char
srogue90_bridge_visible_ch(int y, int x)
{
    return window_ch(cw, y, x);
}

char
srogue90_bridge_terrain_ch(int y, int x)
{
    return window_ch(stdscr, y, x);
}

char
srogue90_bridge_monster_window_ch(int y, int x)
{
    return window_ch(mw, y, x);
}

void *
srogue90_bridge_monster_at(int y, int x)
{
    struct linked_list *item;
    struct thing *monster;

    for (item = mlist; item != NULL; item = next(item))
    {
	monster = (struct thing *) ldata(item);
	if (monster->t_pos.y == y && monster->t_pos.x == x)
	    return (void *) monster;
    }
    return NULL;
}

int
srogue90_bridge_player_is_blind(void)
{
    return on(player, ISBLIND);
}

int
srogue90_bridge_cansee(int y, int x)
{
    if (srogue90_bridge_player_is_blind())
	return FALSE;

    return cansee(y, x) ? TRUE : FALSE;
}

int
srogue90_bridge_see_monst(void *monster)
{
    if (monster == NULL)
	return FALSE;
    if (on(*(struct thing *) monster, ISINVIS) && off(player, CANSEE))
	return FALSE;
    return TRUE;
}

char
srogue90_bridge_monster_type(void *monster)
{
    return monster == NULL ? '\0' : ((struct thing *) monster)->t_type;
}

char
srogue90_bridge_monster_disguise(void *monster)
{
    return monster == NULL ? '\0' : ((struct thing *) monster)->t_disguise;
}

char
srogue90_bridge_object_type_at(int y, int x)
{
    struct linked_list *item;
    struct object *obj;

    for (item = lvl_obj; item != NULL; item = next(item))
    {
	obj = (struct object *) ldata(item);
	if (obj->o_pos.y == y && obj->o_pos.x == x)
	    return (char) obj->o_type;
    }

    return '\0';
}

int
srogue90_bridge_level_type(void)
{
    return levtype;
}
