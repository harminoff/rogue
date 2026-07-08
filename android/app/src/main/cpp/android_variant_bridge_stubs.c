#include "curses.h"

int rogue36_bridge_hero_y(void) { return 0; }
int rogue36_bridge_hero_x(void) { return 0; }
int rogue36_bridge_level(void) { return 0; }
int rogue36_bridge_map_rows(void) { return LINES; }
int rogue36_bridge_map_cols(void) { return COLS; }
int rogue36_bridge_gold(void) { return 0; }
int rogue36_bridge_hp(void) { return 0; }
int rogue36_bridge_max_hp(void) { return 0; }
unsigned int rogue36_bridge_strength(void) { return 0; }
int rogue36_bridge_armor(void) { return 0; }
int rogue36_bridge_exp_level(void) { return 0; }
long rogue36_bridge_exp_points(void) { return 0; }
int rogue36_bridge_hungry_state(void) { return 0; }
const char *rogue36_bridge_message(void) { return ""; }
char rogue36_bridge_visible_ch(int y, int x) { (void) y; (void) x; return ' '; }
char rogue36_bridge_terrain_ch(int y, int x) { (void) y; (void) x; return ' '; }
char rogue36_bridge_monster_window_ch(int y, int x) { (void) y; (void) x; return '\0'; }
void *rogue36_bridge_monster_at(int y, int x) { (void) y; (void) x; return 0; }
int rogue36_bridge_player_is_blind(void) { return TRUE; }
int rogue36_bridge_cansee(int y, int x) { (void) y; (void) x; return FALSE; }
int rogue36_bridge_see_monst(void *monster) { (void) monster; return FALSE; }
char rogue36_bridge_monster_type(void *monster) { (void) monster; return '\0'; }
char rogue36_bridge_monster_disguise(void *monster) { (void) monster; return '\0'; }
char rogue36_bridge_trap_type_at(int y, int x) { (void) y; (void) x; return '\0'; }
char rogue36_bridge_object_type_at(int y, int x) { (void) y; (void) x; return '\0'; }

int srogue90_bridge_hero_y(void) { return 0; }
int srogue90_bridge_hero_x(void) { return 0; }
int srogue90_bridge_level(void) { return 0; }
int srogue90_bridge_map_rows(void) { return LINES; }
int srogue90_bridge_map_cols(void) { return COLS; }
int srogue90_bridge_status_row_start(void) { return LINES > 3 ? LINES - 3 : LINES; }
int srogue90_bridge_gold(void) { return 0; }
int srogue90_bridge_hp(void) { return 0; }
int srogue90_bridge_max_hp(void) { return 0; }
unsigned int srogue90_bridge_strength(void) { return 0; }
unsigned int srogue90_bridge_strength_base(void) { return 0; }
unsigned int srogue90_bridge_dexterity(void) { return 0; }
unsigned int srogue90_bridge_dexterity_base(void) { return 0; }
unsigned int srogue90_bridge_wisdom(void) { return 0; }
unsigned int srogue90_bridge_wisdom_base(void) { return 0; }
unsigned int srogue90_bridge_constitution(void) { return 0; }
unsigned int srogue90_bridge_constitution_base(void) { return 0; }
int srogue90_bridge_armor(void) { return 0; }
int srogue90_bridge_exp_level(void) { return 0; }
long srogue90_bridge_exp_points(void) { return 0; }
int srogue90_bridge_carry_weight(void) { return 0; }
int srogue90_bridge_carry_capacity(void) { return 0; }
int srogue90_bridge_volume_percent(void) { return 0; }
int srogue90_bridge_hungry_state(void) { return 0; }
const char *srogue90_bridge_message(void) { return ""; }
char srogue90_bridge_visible_ch(int y, int x) { (void) y; (void) x; return ' '; }
char srogue90_bridge_terrain_ch(int y, int x) { (void) y; (void) x; return ' '; }
char srogue90_bridge_monster_window_ch(int y, int x) { (void) y; (void) x; return '\0'; }
void *srogue90_bridge_monster_at(int y, int x) { (void) y; (void) x; return 0; }
int srogue90_bridge_player_is_blind(void) { return TRUE; }
int srogue90_bridge_cansee(int y, int x) { (void) y; (void) x; return FALSE; }
int srogue90_bridge_see_monst(void *monster) { (void) monster; return FALSE; }
char srogue90_bridge_monster_type(void *monster) { (void) monster; return '\0'; }
char srogue90_bridge_monster_disguise(void *monster) { (void) monster; return '\0'; }
char srogue90_bridge_object_type_at(int y, int x) { (void) y; (void) x; return '\0'; }
int srogue90_bridge_level_type(void) { return 0; }
