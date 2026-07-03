/*
 * Windows compatibility shims for embedding Rogue 5.2.1 in RogueTiles.
 */

#include "rogue52_port.h"
#include <curses.h>
#include "port/pwd.h"
#include "port/termios.h"
#include "rogue.h"

struct passwd *
rogue52_getpwuid(int uid)
{
    static struct passwd pw;
    static char name[64];
    static char home[PATH_MAX];
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
    strncpy(home, env, sizeof(home) - 2);
    home[sizeof(home) - 2] = '\0';

    pw.pw_name = name;
    pw.pw_dir = home;
    return &pw;
}

char *
rogue52_getpass(const char *prompt)
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
rogue52_tcgetattr(int fd, struct termios *term)
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
rogue52_cfgetospeed(const struct termios *term)
{
    (void) term;
    return 9600;
}

int
rogue52_bridge_hero_y(void)
{
    return hero.y;
}

int
rogue52_bridge_hero_x(void)
{
    return hero.x;
}

int
rogue52_bridge_level(void)
{
    return level;
}

int
rogue52_bridge_map_rows(void)
{
    return MAXLINES;
}

int
rogue52_bridge_map_cols(void)
{
    return MAXCOLS;
}

int
rogue52_bridge_gold(void)
{
    return purse;
}

int
rogue52_bridge_hp(void)
{
    return pstats.s_hpt;
}

int
rogue52_bridge_max_hp(void)
{
    return max_hp;
}

unsigned int
rogue52_bridge_strength(void)
{
    return pstats.s_str;
}

int
rogue52_bridge_armor(void)
{
    return cur_armor != NULL ? cur_armor->o_ac : pstats.s_arm;
}

int
rogue52_bridge_exp_level(void)
{
    return pstats.s_lvl;
}

long
rogue52_bridge_exp_points(void)
{
    return pstats.s_exp;
}

int
rogue52_bridge_hungry_state(void)
{
    return hungry_state;
}

const char *
rogue52_bridge_message(void)
{
    return huh;
}

char
rogue52_bridge_chat(int y, int x)
{
    return chat(y, x);
}

char
rogue52_bridge_flat(int y, int x)
{
    return flat(y, x);
}

void *
rogue52_bridge_moat(int y, int x)
{
    return moat(y, x);
}

int
rogue52_bridge_player_is_blind(void)
{
    return on(player, ISBLIND);
}

int
rogue52_bridge_cansee(int y, int x)
{
    return cansee(y, x);
}

int
rogue52_bridge_see_monst(void *monster)
{
    return monster != NULL && see_monst((THING *) monster);
}

char
rogue52_bridge_monster_type(void *monster)
{
    return monster == NULL ? '\0' : ((THING *) monster)->t_type;
}

char
rogue52_bridge_monster_disguise(void *monster)
{
    return monster == NULL ? '\0' : ((THING *) monster)->t_disguise;
}

char
rogue52_bridge_object_type_at(int y, int x)
{
    THING *obj;

    for (obj = lvl_obj; obj != NULL; obj = next(obj))
	if (obj->o_pos.y == y && obj->o_pos.x == x)
	    return obj->o_type;

    return '\0';
}
