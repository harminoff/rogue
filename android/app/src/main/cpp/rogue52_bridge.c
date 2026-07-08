#include <curses.h>
#include "rogue.h"

int rogue52_bridge_hero_y(void) { return hero.y; }
int rogue52_bridge_hero_x(void) { return hero.x; }
int rogue52_bridge_level(void) { return level; }
int rogue52_bridge_map_rows(void) { return NUMLINES; }
int rogue52_bridge_map_cols(void) { return NUMCOLS; }
int rogue52_bridge_gold(void) { return purse; }
int rogue52_bridge_hp(void) { return pstats.s_hpt; }
int rogue52_bridge_max_hp(void) { return pstats.s_maxhp; }
unsigned int rogue52_bridge_strength(void) { return pstats.s_str; }
int rogue52_bridge_armor(void) { return pstats.s_arm; }
int rogue52_bridge_exp_level(void) { return pstats.s_lvl; }
long rogue52_bridge_exp_points(void) { return pstats.s_exp; }
int rogue52_bridge_hungry_state(void) { return hungry_state; }
const char *rogue52_bridge_message(void) { return huh; }
char rogue52_bridge_chat(int y, int x) { return chat(y, x); }
char rogue52_bridge_flat(int y, int x) { return flat(y, x); }
void *rogue52_bridge_moat(int y, int x) { return moat(y, x); }
int rogue52_bridge_player_is_blind(void) { return on(player, ISBLIND); }
int rogue52_bridge_cansee(int y, int x) { return cansee(y, x); }
int rogue52_bridge_see_monst(void *monster) { return see_monst((THING *) monster); }
char rogue52_bridge_monster_type(void *monster)
{
    return monster != NULL ? ((THING *) monster)->t_type : '\0';
}
char rogue52_bridge_monster_disguise(void *monster)
{
    return monster != NULL ? ((THING *) monster)->t_disguise : '\0';
}
char rogue52_bridge_object_type_at(int y, int x)
{
    THING *obj;

    for (obj = lvl_obj; obj != NULL; obj = obj->l_next)
	if (obj->o_pos.y == y && obj->o_pos.x == x)
	    return (char) obj->o_type;
    return '\0';
}
