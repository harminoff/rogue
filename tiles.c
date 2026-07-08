/*
 * Rogue tile presentation descriptors.
 *
 * The classic game still renders through curses. These helpers provide a
 * semantic view of the same 80x24 game state for a future tile renderer.
 */

#include <ctype.h>
#include <string.h>
#include <curses.h>
#include "rogue.h"
#include "tiles.h"
#include "variant.h"
#include "generated/rogue_tile_mapping.h"

#define ROGUE52_MAXLINES 32
#define ROGUE52_MAXCOLS 80
#define ROGUE52_F_PASS 0x80
#define ROGUE52_F_SEEN 0x40
#define ROGUE52_F_REAL 0x10
#define ROGUE36_MAXLINES 32
#define ROGUE36_MAXCOLS 80
#define SROGUE90_MAXLINES 32
#define SROGUE90_MAXCOLS 256
#define SROGUE90_POSTLEV 1
#define SROGUE90_POOL '"'

extern int rogue52_bridge_hero_y(void);
extern int rogue52_bridge_hero_x(void);
extern int rogue52_bridge_level(void);
extern int rogue52_bridge_map_rows(void);
extern int rogue52_bridge_map_cols(void);
extern int rogue52_bridge_gold(void);
extern int rogue52_bridge_hp(void);
extern int rogue52_bridge_max_hp(void);
extern unsigned int rogue52_bridge_strength(void);
extern int rogue52_bridge_armor(void);
extern int rogue52_bridge_exp_level(void);
extern long rogue52_bridge_exp_points(void);
extern int rogue52_bridge_hungry_state(void);
extern const char *rogue52_bridge_message(void);
extern char rogue52_bridge_chat(int y, int x);
extern char rogue52_bridge_flat(int y, int x);
extern void *rogue52_bridge_moat(int y, int x);
extern int rogue52_bridge_player_is_blind(void);
extern int rogue52_bridge_cansee(int y, int x);
extern int rogue52_bridge_see_monst(void *monster);
extern char rogue52_bridge_monster_type(void *monster);
extern char rogue52_bridge_monster_disguise(void *monster);
extern char rogue52_bridge_object_type_at(int y, int x);
extern int rogue36_bridge_hero_y(void);
extern int rogue36_bridge_hero_x(void);
extern int rogue36_bridge_level(void);
extern int rogue36_bridge_map_rows(void);
extern int rogue36_bridge_map_cols(void);
extern int rogue36_bridge_gold(void);
extern int rogue36_bridge_hp(void);
extern int rogue36_bridge_max_hp(void);
extern unsigned int rogue36_bridge_strength(void);
extern int rogue36_bridge_armor(void);
extern int rogue36_bridge_exp_level(void);
extern long rogue36_bridge_exp_points(void);
extern int rogue36_bridge_hungry_state(void);
extern const char *rogue36_bridge_message(void);
extern char rogue36_bridge_visible_ch(int y, int x);
extern char rogue36_bridge_terrain_ch(int y, int x);
extern char rogue36_bridge_monster_window_ch(int y, int x);
extern void *rogue36_bridge_monster_at(int y, int x);
extern int rogue36_bridge_player_is_blind(void);
extern int rogue36_bridge_cansee(int y, int x);
extern int rogue36_bridge_see_monst(void *monster);
extern char rogue36_bridge_monster_type(void *monster);
extern char rogue36_bridge_monster_disguise(void *monster);
extern char rogue36_bridge_trap_type_at(int y, int x);
extern char rogue36_bridge_object_type_at(int y, int x);
extern int srogue90_bridge_hero_y(void);
extern int srogue90_bridge_hero_x(void);
extern int srogue90_bridge_level(void);
extern int srogue90_bridge_map_rows(void);
extern int srogue90_bridge_map_cols(void);
extern int srogue90_bridge_status_row_start(void);
extern int srogue90_bridge_gold(void);
extern int srogue90_bridge_hp(void);
extern int srogue90_bridge_max_hp(void);
extern unsigned int srogue90_bridge_strength(void);
extern unsigned int srogue90_bridge_strength_base(void);
extern unsigned int srogue90_bridge_dexterity(void);
extern unsigned int srogue90_bridge_dexterity_base(void);
extern unsigned int srogue90_bridge_wisdom(void);
extern unsigned int srogue90_bridge_wisdom_base(void);
extern unsigned int srogue90_bridge_constitution(void);
extern unsigned int srogue90_bridge_constitution_base(void);
extern int srogue90_bridge_armor(void);
extern int srogue90_bridge_exp_level(void);
extern long srogue90_bridge_exp_points(void);
extern int srogue90_bridge_carry_weight(void);
extern int srogue90_bridge_carry_capacity(void);
extern int srogue90_bridge_volume_percent(void);
extern int srogue90_bridge_hungry_state(void);
extern const char *srogue90_bridge_message(void);
extern char srogue90_bridge_visible_ch(int y, int x);
extern char srogue90_bridge_terrain_ch(int y, int x);
extern char srogue90_bridge_monster_window_ch(int y, int x);
extern void *srogue90_bridge_monster_at(int y, int x);
extern int srogue90_bridge_player_is_blind(void);
extern int srogue90_bridge_cansee(int y, int x);
extern int srogue90_bridge_see_monst(void *monster);
extern char srogue90_bridge_monster_type(void *monster);
extern char srogue90_bridge_monster_disguise(void *monster);
extern char srogue90_bridge_object_type_at(int y, int x);
extern int srogue90_bridge_level_type(void);

static void
set_empty_cell(int y, int x, ROGUE_TILE_CELL *cell)
{
    cell->y = y;
    cell->x = x;
    cell->glyph = ' ';
    cell->layer = ROGUE_TILE_EMPTY;
    cell->role = "terrain.empty";
    cell->atlas_key = NULL;
    cell->atlas_index = -1;
    cell->has_underlay = FALSE;
    cell->under_glyph = ' ';
    cell->under_role = NULL;
    cell->under_atlas_key = NULL;
    cell->under_atlas_index = -1;
    cell->name = "empty";
    cell->seen = FALSE;
    cell->visible = FALSE;
}

static THING *
object_at(int y, int x)
{
    THING *obj;

    for (obj = lvl_obj; obj != NULL; obj = next(obj))
	if (obj->o_pos.y == y && obj->o_pos.x == x)
	    return obj;

    return NULL;
}

static const ROGUE_GENERATED_TILE_MAPPING *
find_glyph_mapping(char glyph)
{
    int i;

    for (i = 0; i < rogue_tile_glyph_mapping_count; i++)
	if (rogue_tile_glyph_mappings[i].glyph == glyph)
	    return &rogue_tile_glyph_mappings[i];

    return NULL;
}

static char
terrain_glyph_from_flags(char flags, char glyph)
{
    if (flags & F_PASS)
	return PASSAGE;

    switch (glyph)
    {
	case FLOOR:
	case PASSAGE:
	case DOOR:
	case '|':
	case '-':
	case STAIRS:
	case TRAP:
	    return glyph;
	default:
	    return FLOOR;
    }
}

static char
terrain_glyph_at(int y, int x, char glyph)
{
    return terrain_glyph_from_flags(flat(y, x), glyph);
}

static void
apply_underlay_for_flags(int y, int x, char flags, char glyph,
			 ROGUE_TILE_CELL *cell)
{
    const ROGUE_GENERATED_TILE_MAPPING *mapping;
    char terrain_glyph;

    terrain_glyph = terrain_glyph_from_flags(flags, glyph);
    mapping = find_glyph_mapping(terrain_glyph);
    if (mapping == NULL || mapping->atlas_index < 0)
	return;

    cell->has_underlay = TRUE;
    cell->under_glyph = terrain_glyph;
    cell->under_role = mapping->role;
    cell->under_atlas_key = mapping->atlas_key;
    cell->under_atlas_index = mapping->atlas_index;
}

static void
apply_underlay(int y, int x, char glyph, ROGUE_TILE_CELL *cell)
{
    apply_underlay_for_flags(y, x, flat(y, x), glyph, cell);
}

static const ROGUE_GENERATED_MONSTER_MAPPING *
find_monster_mapping(char glyph)
{
    int i;

    for (i = 0; i < rogue_tile_monster_mapping_count; i++)
	if (rogue_tile_monster_mappings[i].glyph == glyph)
	    return &rogue_tile_monster_mappings[i];

    return NULL;
}

static const ROGUE_GENERATED_VARIANT_MONSTER_MAPPING *
find_variant_monster_mapping(const char *variant_id, char glyph)
{
    int i;

    for (i = 0; i < rogue_tile_variant_monster_mapping_count; i++)
	if (rogue_tile_variant_monster_mappings[i].variant_id != NULL
	    && strcmp(rogue_tile_variant_monster_mappings[i].variant_id,
		      variant_id) == 0
	    && rogue_tile_variant_monster_mappings[i].glyph == glyph)
	    return &rogue_tile_variant_monster_mappings[i];

    return NULL;
}

static const ROGUE_GENERATED_VARIANT_TILE_MAPPING *
find_variant_tile_mapping(const char *variant_id, char glyph)
{
    int i;

    for (i = 0; i < rogue_tile_variant_tile_mapping_count; i++)
	if (rogue_tile_variant_tile_mappings[i].variant_id != NULL
	    && strcmp(rogue_tile_variant_tile_mappings[i].variant_id,
		      variant_id) == 0
	    && rogue_tile_variant_tile_mappings[i].glyph == glyph)
	    return &rogue_tile_variant_tile_mappings[i];

    return NULL;
}

static const ROGUE_GENERATED_TRAP_MAPPING *
find_trap_mapping(char trap_type)
{
    int i;

    for (i = 0; i < rogue_tile_trap_mapping_count; i++)
	if (rogue_tile_trap_mappings[i].trap_type == trap_type)
	    return &rogue_tile_trap_mappings[i];

    return NULL;
}

static bool
monster_has_disguise(char disguise, char monster_type)
{
    return (bool)(disguise != '\0' && disguise != monster_type);
}

static void
normalize_stale_player_glyph(char *glyph, char terrain_glyph)
{
    if (glyph == NULL)
	return;
    if (*glyph == PLAYER)
	*glyph = terrain_glyph;
}

static void
apply_glyph_mapping(int y, int x, char glyph, ROGUE_TILE_CELL *cell)
{
    const ROGUE_GENERATED_TILE_MAPPING *mapping;

    mapping = find_glyph_mapping(glyph);
    if (mapping == NULL)
    {
	set_empty_cell(y, x, cell);
	cell->glyph = glyph;
	cell->role = "unknown.glyph";
	cell->name = "unknown";
	return;
    }

    cell->y = y;
    cell->x = x;
    cell->glyph = glyph;
    cell->layer = mapping->layer;
    cell->role = mapping->role;
    cell->atlas_key = mapping->atlas_key;
    cell->atlas_index = mapping->atlas_index;
    cell->has_underlay = FALSE;
    cell->under_glyph = ' ';
    cell->under_role = NULL;
    cell->under_atlas_key = NULL;
    cell->under_atlas_index = -1;
    cell->name = mapping->name;
}

static bool
apply_variant_glyph_mapping(const char *variant_id, int y, int x, char glyph,
			    ROGUE_TILE_CELL *cell)
{
    const ROGUE_GENERATED_VARIANT_TILE_MAPPING *mapping;

    mapping = find_variant_tile_mapping(variant_id, glyph);
    if (mapping == NULL)
	return FALSE;

    cell->y = y;
    cell->x = x;
    cell->glyph = glyph;
    cell->layer = mapping->layer;
    cell->role = mapping->role;
    cell->atlas_key = mapping->atlas_key;
    cell->atlas_index = mapping->atlas_index;
    cell->has_underlay = FALSE;
    cell->under_glyph = ' ';
    cell->under_role = NULL;
    cell->under_atlas_key = NULL;
    cell->under_atlas_index = -1;
    cell->name = mapping->name;
    return TRUE;
}

static bool
apply_trap_mapping(int y, int x, char trap_type, ROGUE_TILE_CELL *cell)
{
    const ROGUE_GENERATED_TRAP_MAPPING *mapping;

    mapping = find_trap_mapping(trap_type);
    if (mapping == NULL)
	return FALSE;

    cell->y = y;
    cell->x = x;
    cell->glyph = TRAP;
    cell->layer = ROGUE_TILE_TERRAIN;
    cell->role = mapping->role;
    cell->atlas_key = mapping->atlas_key;
    cell->atlas_index = mapping->atlas_index;
    cell->has_underlay = FALSE;
    cell->under_glyph = ' ';
    cell->under_role = NULL;
    cell->under_atlas_key = NULL;
    cell->under_atlas_index = -1;
    cell->name = mapping->name;
    return TRUE;
}

static void
apply_variant_underlay_for_glyph(const char *variant_id, int y, int x,
				 char glyph, ROGUE_TILE_CELL *cell)
{
    const ROGUE_GENERATED_VARIANT_TILE_MAPPING *variant_mapping;
    const ROGUE_GENERATED_TILE_MAPPING *mapping;

    variant_mapping = find_variant_tile_mapping(variant_id, glyph);
    if (variant_mapping != NULL && variant_mapping->atlas_index >= 0)
    {
	cell->has_underlay = TRUE;
	cell->under_glyph = glyph;
	cell->under_role = variant_mapping->role;
	cell->under_atlas_key = variant_mapping->atlas_key;
	cell->under_atlas_index = variant_mapping->atlas_index;
	return;
    }

    mapping = find_glyph_mapping(glyph);
    if (mapping == NULL || mapping->atlas_index < 0)
	return;

    cell->has_underlay = TRUE;
    cell->under_glyph = glyph;
    cell->under_role = mapping->role;
    cell->under_atlas_key = mapping->atlas_key;
    cell->under_atlas_index = mapping->atlas_index;
}

static void
apply_monster_mapping(int y, int x, THING *monster, ROGUE_TILE_CELL *cell)
{
    const ROGUE_GENERATED_MONSTER_MAPPING *mapping;
    char glyph;

    glyph = monster->t_type;
    mapping = find_monster_mapping(glyph);

    cell->y = y;
    cell->x = x;
    cell->glyph = glyph;
    cell->layer = ROGUE_TILE_ACTOR;
    cell->role = (mapping == NULL) ? "monster" : mapping->role;
    cell->atlas_key = (mapping == NULL) ? NULL : mapping->atlas_key;
    cell->atlas_index = (mapping == NULL) ? -1 : mapping->atlas_index;
    cell->has_underlay = FALSE;
    cell->under_glyph = ' ';
    cell->under_role = NULL;
    cell->under_atlas_key = NULL;
    cell->under_atlas_index = -1;
    cell->name = (mapping == NULL) ? "monster" : mapping->name;
}

static void
apply_rogue52_monster_mapping(int y, int x, void *monster,
			      ROGUE_TILE_CELL *cell)
{
    const ROGUE_GENERATED_MONSTER_MAPPING *mapping;
    const ROGUE_GENERATED_VARIANT_MONSTER_MAPPING *variant_mapping;
    char glyph;

    glyph = rogue52_bridge_monster_type(monster);
    variant_mapping = find_variant_monster_mapping("rogue52", glyph);
    mapping = (variant_mapping == NULL) ? find_monster_mapping(glyph) : NULL;

    cell->y = y;
    cell->x = x;
    cell->glyph = glyph;
    cell->layer = ROGUE_TILE_ACTOR;
    cell->role = (variant_mapping != NULL)
	? variant_mapping->role
	: ((mapping == NULL) ? "monster" : mapping->role);
    cell->atlas_key = (variant_mapping != NULL)
	? variant_mapping->atlas_key
	: ((mapping == NULL) ? NULL : mapping->atlas_key);
    cell->atlas_index = (variant_mapping != NULL)
	? variant_mapping->atlas_index
	: ((mapping == NULL) ? -1 : mapping->atlas_index);
    cell->has_underlay = FALSE;
    cell->under_glyph = ' ';
    cell->under_role = NULL;
    cell->under_atlas_key = NULL;
    cell->under_atlas_index = -1;
    cell->name = (variant_mapping != NULL)
	? variant_mapping->name
	: ((mapping == NULL) ? "monster" : mapping->name);
}

static void
apply_variant_monster_mapping(const char *variant_id, int y, int x,
			      char glyph, ROGUE_TILE_CELL *cell)
{
    const ROGUE_GENERATED_MONSTER_MAPPING *mapping;
    const ROGUE_GENERATED_VARIANT_MONSTER_MAPPING *variant_mapping;

    variant_mapping = find_variant_monster_mapping(variant_id, glyph);
    mapping = (variant_mapping == NULL) ? find_monster_mapping(glyph) : NULL;

    cell->y = y;
    cell->x = x;
    cell->glyph = glyph;
    cell->layer = ROGUE_TILE_ACTOR;
    cell->role = (variant_mapping != NULL)
	? variant_mapping->role
	: ((mapping == NULL) ? "monster" : mapping->role);
    cell->atlas_key = (variant_mapping != NULL)
	? variant_mapping->atlas_key
	: ((mapping == NULL) ? NULL : mapping->atlas_key);
    cell->atlas_index = (variant_mapping != NULL)
	? variant_mapping->atlas_index
	: ((mapping == NULL) ? -1 : mapping->atlas_index);
    cell->has_underlay = FALSE;
    cell->under_glyph = ' ';
    cell->under_role = NULL;
    cell->under_atlas_key = NULL;
    cell->under_atlas_index = -1;
    cell->name = (variant_mapping != NULL)
	? variant_mapping->name
	: ((mapping == NULL) ? "monster" : mapping->name);
}

static char
rogue52_chat(int y, int x)
{
    return rogue52_bridge_chat(y, x);
}

static char
rogue52_flat(int y, int x)
{
    return rogue52_bridge_flat(y, x);
}

static void *
rogue52_moat(int y, int x)
{
    return rogue52_bridge_moat(y, x);
}

void
rogue_tile_describe_cell(int y, int x, ROGUE_TILE_CELL *cell)
{
    THING *monster;
    char glyph;
    bool visible;
    bool seen;
    static bool known_cells[NUMLINES][NUMCOLS];
    static int known_level = -1;

    if (cell == NULL)
	return;

    set_empty_cell(y, x, cell);

    if (y < 0 || y >= NUMLINES || x < 0 || x >= NUMCOLS)
	return;

    if (known_level != level)
    {
	memset(known_cells, 0, sizeof(known_cells));
	known_level = level;
    }

    glyph = chat(y, x);
    visible = ((hero.y == y && hero.x == x)
	       || (glyph != ' ' && !on(player, ISBLIND) && cansee(y, x)));
    seen = (bool)((flat(y, x) & F_SEEN) != 0);

    if (visible || seen)
	known_cells[y][x] = TRUE;

    cell->seen = known_cells[y][x];
    cell->visible = visible;

    if (!cell->visible && !cell->seen)
	return;

    if (hero.y == y && hero.x == x)
    {
	apply_glyph_mapping(y, x, PLAYER, cell);
	apply_underlay(y, x, glyph, cell);
	cell->seen = TRUE;
	cell->visible = TRUE;
	return;
    }

    monster = moat(y, x);
    if (visible && monster != NULL && see_monst(monster))
    {
	if (monster->t_disguise != monster->t_type)
	{
	    apply_glyph_mapping(y, x, monster->t_disguise, cell);
	    if (cell->layer == ROGUE_TILE_OBJECT)
		apply_underlay(y, x, glyph, cell);
	    cell->seen = TRUE;
	    cell->visible = TRUE;
	    return;
	}

	apply_monster_mapping(y, x, monster, cell);
	apply_underlay(y, x, glyph, cell);
	cell->seen = TRUE;
	cell->visible = TRUE;
	return;
    }

    if (glyph == TRAP && (flat(y, x) & F_REAL) == 0)
	glyph = FLOOR;

    if (find_glyph_mapping(glyph) == NULL && object_at(y, x) != NULL)
	glyph = object_at(y, x)->o_type;

    apply_glyph_mapping(y, x, glyph, cell);
    if (cell->layer == ROGUE_TILE_OBJECT)
	apply_underlay(y, x, glyph, cell);
}

void
rogue52_tile_describe_cell(int y, int x, ROGUE_TILE_CELL *cell)
{
    void *monster;
    char glyph;
    char object_glyph;
    bool visible;
    bool seen;
    static bool known_cells[ROGUE52_MAXLINES][ROGUE52_MAXCOLS];
    static int known_level = -1;
    int rows;
    int cols;

    if (cell == NULL)
	return;

    set_empty_cell(y, x, cell);

    rows = rogue52_bridge_map_rows();
    cols = rogue52_bridge_map_cols();
    if (rows > ROGUE52_MAXLINES)
	rows = ROGUE52_MAXLINES;
    if (cols > ROGUE52_MAXCOLS)
	cols = ROGUE52_MAXCOLS;

    if (y < 0 || y >= rows || x < 0 || x >= cols)
	return;

    if (known_level != rogue52_bridge_level())
    {
	memset(known_cells, 0, sizeof(known_cells));
	known_level = rogue52_bridge_level();
    }

    glyph = rogue52_chat(y, x);
    visible = ((rogue52_bridge_hero_y() == y
		&& rogue52_bridge_hero_x() == x)
	       || (glyph != ' '
		   && !rogue52_bridge_player_is_blind()
		   && rogue52_bridge_cansee(y, x)));
    seen = (bool)((rogue52_flat(y, x) & ROGUE52_F_SEEN) != 0);

    if (visible || seen)
	known_cells[y][x] = TRUE;

    cell->seen = known_cells[y][x];
    cell->visible = visible;

    if (!cell->visible && !cell->seen)
	return;

    if (rogue52_bridge_hero_y() == y
	&& rogue52_bridge_hero_x() == x)
    {
	apply_glyph_mapping(y, x, PLAYER, cell);
	apply_underlay_for_flags(y, x, rogue52_flat(y, x), glyph, cell);
	cell->seen = TRUE;
	cell->visible = TRUE;
	return;
    }

    monster = rogue52_moat(y, x);
    if (visible && monster != NULL && rogue52_bridge_see_monst(monster))
    {
	if (monster_has_disguise(rogue52_bridge_monster_disguise(monster),
				 rogue52_bridge_monster_type(monster)))
	{
	    apply_glyph_mapping(y, x,
				rogue52_bridge_monster_disguise(monster),
				cell);
	    if (cell->layer == ROGUE_TILE_OBJECT)
		apply_underlay_for_flags(y, x, rogue52_flat(y, x), glyph,
					 cell);
	    cell->seen = TRUE;
	    cell->visible = TRUE;
	    return;
	}

	apply_rogue52_monster_mapping(y, x, monster, cell);
	apply_underlay_for_flags(y, x, rogue52_flat(y, x), glyph, cell);
	cell->seen = TRUE;
	cell->visible = TRUE;
	return;
    }

    if (glyph == TRAP && (rogue52_flat(y, x) & ROGUE52_F_REAL) == 0)
	glyph = FLOOR;

    object_glyph = rogue52_bridge_object_type_at(y, x);
    if (find_glyph_mapping(glyph) == NULL && object_glyph != '\0')
	glyph = object_glyph;

    apply_glyph_mapping(y, x, glyph, cell);
    if (cell->layer == ROGUE_TILE_OBJECT)
	apply_underlay_for_flags(y, x, rogue52_flat(y, x), glyph, cell);
}

static char
rogue36_underlay_glyph(int y, int x, char fallback)
{
    char glyph;

    glyph = rogue36_bridge_terrain_ch(y, x);
    switch (glyph)
    {
	case FLOOR:
	case PASSAGE:
	case DOOR:
	case '|':
	case '-':
	case STAIRS:
	case TRAP:
	    return glyph;
	case GOLD:
	case POTION:
	case SCROLL:
	case MAGIC:
	case FOOD:
	case WEAPON:
	case ARMOR:
	case AMULET:
	case RING:
	case STICK:
	    return FLOOR;
	default:
	    return fallback == ' ' ? FLOOR : fallback;
    }
}

void
rogue36_tile_describe_cell(int y, int x, ROGUE_TILE_CELL *cell)
{
    void *monster;
    char glyph;
    char monster_glyph;
    char object_glyph;
    char terrain_glyph;
    bool object_visible;
    bool glyph_is_object;
    bool visible;
    bool seen;
    static bool known_cells[ROGUE36_MAXLINES][ROGUE36_MAXCOLS];
    static int known_level = -1;
    int rows;
    int cols;

    if (cell == NULL)
	return;

    set_empty_cell(y, x, cell);

    rows = rogue36_bridge_map_rows();
    cols = rogue36_bridge_map_cols();
    if (rows > ROGUE36_MAXLINES)
	rows = ROGUE36_MAXLINES;
    if (cols > ROGUE36_MAXCOLS)
	cols = ROGUE36_MAXCOLS;

    if (y <= 0 || y >= rows || x < 0 || x >= cols)
	return;

    if (known_level != rogue36_bridge_level())
    {
	memset(known_cells, 0, sizeof(known_cells));
	known_level = rogue36_bridge_level();
    }

    glyph = rogue36_bridge_visible_ch(y, x);
    terrain_glyph = rogue36_bridge_terrain_ch(y, x);
    monster_glyph = rogue36_bridge_monster_window_ch(y, x);
    object_glyph = rogue36_bridge_object_type_at(y, x);
    glyph_is_object = (object_glyph != '\0' && glyph == object_glyph);
    visible = ((rogue36_bridge_hero_y() == y
		&& rogue36_bridge_hero_x() == x)
	       || (glyph != ' '
		   && !rogue36_bridge_player_is_blind()
		   && rogue36_bridge_cansee(y, x)));
    object_visible = (bool)(object_glyph != '\0' && visible);
    seen = FALSE;

    if (visible || seen)
	known_cells[y][x] = TRUE;

    cell->seen = known_cells[y][x];
    cell->visible = visible;

    if (!cell->visible && !cell->seen)
	return;

    if (rogue36_bridge_hero_y() == y
	&& rogue36_bridge_hero_x() == x)
    {
	apply_glyph_mapping(y, x, PLAYER, cell);
	apply_underlay_for_flags(y, x, 0, rogue36_underlay_glyph(y, x,
								 terrain_glyph),
				 cell);
	cell->seen = TRUE;
	cell->visible = TRUE;
	return;
    }

    normalize_stale_player_glyph(&glyph, terrain_glyph);

    monster = rogue36_bridge_monster_at(y, x);
    if (visible && monster != NULL && monster_glyph != ' '
	&& rogue36_bridge_see_monst(monster))
    {
	if (monster_has_disguise(rogue36_bridge_monster_disguise(monster),
				 rogue36_bridge_monster_type(monster)))
	{
	    apply_glyph_mapping(y, x,
				rogue36_bridge_monster_disguise(monster),
				cell);
	    if (cell->layer == ROGUE_TILE_OBJECT)
		apply_underlay_for_flags(y, x, 0,
					 rogue36_underlay_glyph(y, x,
								terrain_glyph),
					 cell);
	    cell->seen = TRUE;
	    cell->visible = TRUE;
	    return;
	}

	apply_variant_monster_mapping("rogue36", y, x,
				      rogue36_bridge_monster_type(monster),
				      cell);
	apply_underlay_for_flags(y, x, 0,
				 rogue36_underlay_glyph(y, x, terrain_glyph),
				 cell);
	cell->seen = TRUE;
	cell->visible = TRUE;
	return;
    }

    if (!object_visible && glyph_is_object)
	glyph = terrain_glyph;
    if (object_visible && find_glyph_mapping(glyph) == NULL
	&& object_glyph != '\0')
	glyph = object_glyph;
    if (glyph == ' ' && terrain_glyph != ' ')
	glyph = terrain_glyph;

    if (glyph == TRAP
	&& apply_trap_mapping(y, x, rogue36_bridge_trap_type_at(y, x), cell))
    {
	apply_underlay_for_flags(y, x, 0, FLOOR, cell);
	return;
    }

    apply_glyph_mapping(y, x, glyph, cell);
    if (cell->layer == ROGUE_TILE_OBJECT)
	apply_underlay_for_flags(y, x, 0,
				 rogue36_underlay_glyph(y, x, terrain_glyph),
				 cell);
}

static char
srogue90_underlay_glyph(int y, int x, char fallback)
{
    char glyph;

    glyph = srogue90_bridge_terrain_ch(y, x);
    switch (glyph)
    {
	case FLOOR:
	case PASSAGE:
	case DOOR:
	case '|':
	case '-':
	case STAIRS:
	case TRAP:
	case MAGIC:
	case '"':
	case '&':
	case '\\':
	case '>':
	case '{':
	case '}':
	case '~':
	case '`':
	    return glyph;
	case GOLD:
	case POTION:
	case SCROLL:
	case FOOD:
	case WEAPON:
	case ARMOR:
	case AMULET:
	case RING:
	case STICK:
	    return FLOOR;
	default:
	    return fallback == ' ' ? FLOOR : fallback;
    }
}

void
srogue90_tile_describe_cell(int y, int x, ROGUE_TILE_CELL *cell)
{
    void *monster;
    char glyph;
    char monster_glyph;
    char object_glyph;
    char terrain_glyph;
    bool object_visible;
    bool glyph_is_object;
    bool visible;
    bool seen;
    static bool known_cells[SROGUE90_MAXLINES][SROGUE90_MAXCOLS];
    static int known_level = -1;
    int rows;
    int cols;

    if (cell == NULL)
	return;

    set_empty_cell(y, x, cell);

    rows = srogue90_bridge_map_rows();
    cols = srogue90_bridge_map_cols();
    if (rows > SROGUE90_MAXLINES)
	rows = SROGUE90_MAXLINES;
    if (cols > SROGUE90_MAXCOLS)
	cols = SROGUE90_MAXCOLS;

    if (y <= 0 || y >= rows || y >= srogue90_bridge_status_row_start()
	|| x < 0 || x >= cols)
	return;

    if (known_level != srogue90_bridge_level())
    {
	memset(known_cells, 0, sizeof(known_cells));
	known_level = srogue90_bridge_level();
    }

    glyph = srogue90_bridge_visible_ch(y, x);
    terrain_glyph = srogue90_bridge_terrain_ch(y, x);
    monster_glyph = srogue90_bridge_monster_window_ch(y, x);
    object_glyph = srogue90_bridge_object_type_at(y, x);
    glyph_is_object = (object_glyph != '\0' && glyph == object_glyph);
    visible = ((srogue90_bridge_hero_y() == y
		&& srogue90_bridge_hero_x() == x)
	       || (glyph != ' '
		   && !srogue90_bridge_player_is_blind()
		   && srogue90_bridge_cansee(y, x)));
    object_visible = (bool)(glyph_is_object && visible);
    seen = FALSE;

    if (visible || seen)
	known_cells[y][x] = TRUE;

    cell->seen = known_cells[y][x];
    cell->visible = visible;

    if (!cell->visible && !cell->seen)
	return;

    if (srogue90_bridge_hero_y() == y
	&& srogue90_bridge_hero_x() == x)
    {
	apply_glyph_mapping(y, x, PLAYER, cell);
	apply_variant_underlay_for_glyph("srogue90", y, x,
					 srogue90_underlay_glyph(y, x,
								 terrain_glyph),
					 cell);
	cell->seen = TRUE;
	cell->visible = TRUE;
	return;
    }

    normalize_stale_player_glyph(&glyph, terrain_glyph);

    monster = srogue90_bridge_monster_at(y, x);
    if (visible && monster != NULL && monster_glyph != ' '
	&& srogue90_bridge_see_monst(monster))
    {
	if (monster_has_disguise(srogue90_bridge_monster_disguise(monster),
				 srogue90_bridge_monster_type(monster)))
	{
	    apply_glyph_mapping(y, x,
				srogue90_bridge_monster_disguise(monster),
				cell);
	    if (cell->layer == ROGUE_TILE_OBJECT)
		apply_variant_underlay_for_glyph("srogue90", y, x,
						 srogue90_underlay_glyph(
						     y, x, terrain_glyph),
						 cell);
	    cell->seen = TRUE;
	    cell->visible = TRUE;
	    return;
	}

	apply_variant_monster_mapping("srogue90", y, x,
				      srogue90_bridge_monster_type(monster),
				      cell);
	apply_variant_underlay_for_glyph("srogue90", y, x,
					 srogue90_underlay_glyph(y, x,
								 terrain_glyph),
					 cell);
	cell->seen = TRUE;
	cell->visible = TRUE;
	return;
    }

    if (!object_visible && glyph_is_object)
	glyph = terrain_glyph;
    if (object_visible && find_glyph_mapping(glyph) == NULL
	&& object_glyph != '\0')
	glyph = object_glyph;
    if (glyph == ' ' && terrain_glyph != ' ')
	glyph = terrain_glyph;

    if (object_visible && glyph_is_object)
	apply_glyph_mapping(y, x, glyph, cell);
    else if (glyph == MAGIC && terrain_glyph != MAGIC)
	apply_glyph_mapping(y, x, glyph, cell);
    else if (!apply_variant_glyph_mapping("srogue90", y, x, glyph, cell))
	apply_glyph_mapping(y, x, glyph, cell);
    if (cell->layer == ROGUE_TILE_OBJECT)
	apply_variant_underlay_for_glyph("srogue90", y, x,
					 srogue90_underlay_glyph(y, x,
								 terrain_glyph),
					 cell);
}

void
rogue54_variant_action_context(ROGUE_VARIANT_ACTION_CONTEXT *context)
{
    if (context == NULL)
	return;

    context->on_stairs = (bool)(chat(hero.y, hero.x) == STAIRS);
    context->on_object = (bool)(object_at(hero.y, hero.x) != NULL);
}

void
rogue52_variant_status(ROGUE_VARIANT_STATUS *status)
{
    if (status == NULL)
	return;

    status->dungeon_level = rogue52_bridge_level();
    status->gold = rogue52_bridge_gold();
    status->hp = rogue52_bridge_hp();
    status->max_hit_points = rogue52_bridge_max_hp();
    status->strength = rogue52_bridge_strength();
    status->armor = rogue52_bridge_armor();
    status->exp_level = rogue52_bridge_exp_level();
    status->exp_points = rogue52_bridge_exp_points();
    status->hungry_state = rogue52_bridge_hungry_state();
    status->message = rogue52_bridge_message();
}

void
rogue52_variant_hero_position(int *y, int *x)
{
    if (y != NULL)
	*y = rogue52_bridge_hero_y();
    if (x != NULL)
	*x = rogue52_bridge_hero_x();
}

void
rogue52_variant_action_context(ROGUE_VARIANT_ACTION_CONTEXT *context)
{
    int y;
    int x;

    if (context == NULL)
	return;

    y = rogue52_bridge_hero_y();
    x = rogue52_bridge_hero_x();
    context->on_stairs = (bool)(rogue52_bridge_chat(y, x) == STAIRS);
    context->on_object = (bool)(rogue52_bridge_object_type_at(y, x) != '\0');
}

int
rogue52_variant_level_number(void)
{
    return rogue52_bridge_level();
}

int
rogue52_variant_map_rows(void)
{
    return rogue52_bridge_map_rows();
}

int
rogue52_variant_map_cols(void)
{
    return rogue52_bridge_map_cols();
}

bool
rogue52_variant_cell_walkable(int y, int x)
{
    char ch;
    int rows;
    int cols;

    rows = rogue52_bridge_map_rows();
    cols = rogue52_bridge_map_cols();
    if (rows > ROGUE52_MAXLINES)
	rows = ROGUE52_MAXLINES;
    if (cols > ROGUE52_MAXCOLS)
	cols = ROGUE52_MAXCOLS;

    if (y <= 0 || y >= rows - 1 || x < 0 || x >= cols)
	return FALSE;

    ch = rogue52_chat(y, x);
    switch (ch)
    {
	case ' ':
	case '|':
	case '-':
	    return FALSE;
	case FLOOR:
	case PASSAGE:
	case DOOR:
	case TRAP:
	case STAIRS:
	case GOLD:
	case POTION:
	case SCROLL:
	case MAGIC:
	case FOOD:
	case WEAPON:
	case ARMOR:
	case AMULET:
	case RING:
	case STICK:
	    return TRUE;
	default:
	    return FALSE;
    }
}

void
rogue36_variant_status(ROGUE_VARIANT_STATUS *status)
{
    if (status == NULL)
	return;

    status->dungeon_level = rogue36_bridge_level();
    status->gold = rogue36_bridge_gold();
    status->hp = rogue36_bridge_hp();
    status->max_hit_points = rogue36_bridge_max_hp();
    status->strength = rogue36_bridge_strength();
    status->armor = rogue36_bridge_armor();
    status->exp_level = rogue36_bridge_exp_level();
    status->exp_points = rogue36_bridge_exp_points();
    status->hungry_state = rogue36_bridge_hungry_state();
    status->message = rogue36_bridge_message();
}

void
rogue36_variant_hero_position(int *y, int *x)
{
    if (y != NULL)
	*y = rogue36_bridge_hero_y();
    if (x != NULL)
	*x = rogue36_bridge_hero_x();
}

void
rogue36_variant_action_context(ROGUE_VARIANT_ACTION_CONTEXT *context)
{
    int y;
    int x;

    if (context == NULL)
	return;

    y = rogue36_bridge_hero_y();
    x = rogue36_bridge_hero_x();
    context->on_stairs = (bool)(rogue36_bridge_terrain_ch(y, x) == STAIRS);
    context->on_object = (bool)(rogue36_bridge_object_type_at(y, x) != '\0');
}

int
rogue36_variant_level_number(void)
{
    return rogue36_bridge_level();
}

int
rogue36_variant_map_rows(void)
{
    return rogue36_bridge_map_rows();
}

int
rogue36_variant_map_cols(void)
{
    return rogue36_bridge_map_cols();
}

bool
rogue36_variant_cell_walkable(int y, int x)
{
    char ch;
    int rows;
    int cols;

    rows = rogue36_bridge_map_rows();
    cols = rogue36_bridge_map_cols();
    if (rows > ROGUE36_MAXLINES)
	rows = ROGUE36_MAXLINES;
    if (cols > ROGUE36_MAXCOLS)
	cols = ROGUE36_MAXCOLS;

    if (y <= 0 || y >= rows || x < 0 || x >= cols)
	return FALSE;

    ch = rogue36_bridge_visible_ch(y, x);
    switch (ch)
    {
	case ' ':
	case '|':
	case '-':
	    return FALSE;
	case FLOOR:
	case PASSAGE:
	case DOOR:
	case TRAP:
	case STAIRS:
	case GOLD:
	case POTION:
	case SCROLL:
	case MAGIC:
	case FOOD:
	case WEAPON:
	case ARMOR:
	case AMULET:
	case RING:
	case STICK:
	    return TRUE;
	default:
	    return FALSE;
    }
}

const char *
rogue_tile_layer_name(ROGUE_TILE_LAYER layer)
{
    switch (layer)
    {
	case ROGUE_TILE_EMPTY:
	    return "empty";
	when ROGUE_TILE_TERRAIN:
	    return "terrain";
	when ROGUE_TILE_OBJECT:
	    return "object";
	when ROGUE_TILE_ACTOR:
	    return "actor";
	otherwise:
	    return "unknown";
    }
}

void
srogue90_variant_status(ROGUE_VARIANT_STATUS *status)
{
    if (status == NULL)
	return;

    status->dungeon_level = srogue90_bridge_level();
    status->gold = srogue90_bridge_gold();
    status->hp = srogue90_bridge_hp();
    status->max_hit_points = srogue90_bridge_max_hp();
    status->strength = srogue90_bridge_strength();
    status->strength_base = srogue90_bridge_strength_base();
    status->dexterity = srogue90_bridge_dexterity();
    status->dexterity_base = srogue90_bridge_dexterity_base();
    status->wisdom = srogue90_bridge_wisdom();
    status->wisdom_base = srogue90_bridge_wisdom_base();
    status->constitution = srogue90_bridge_constitution();
    status->constitution_base = srogue90_bridge_constitution_base();
    status->armor = srogue90_bridge_armor();
    status->exp_level = srogue90_bridge_exp_level();
    status->exp_points = srogue90_bridge_exp_points();
    status->carry_weight = srogue90_bridge_carry_weight();
    status->carry_capacity = srogue90_bridge_carry_capacity();
    status->volume_percent = srogue90_bridge_volume_percent();
    status->has_extended_stats = TRUE;
    status->hungry_state = srogue90_bridge_hungry_state();
    status->message = srogue90_bridge_message();
}

void
srogue90_variant_hero_position(int *y, int *x)
{
    if (y != NULL)
	*y = srogue90_bridge_hero_y();
    if (x != NULL)
	*x = srogue90_bridge_hero_x();
}

void
srogue90_variant_action_context(ROGUE_VARIANT_ACTION_CONTEXT *context)
{
    char terrain;
    int y;
    int x;

    if (context == NULL)
	return;

    y = srogue90_bridge_hero_y();
    x = srogue90_bridge_hero_x();
    terrain = srogue90_bridge_terrain_ch(y, x);
    context->on_stairs = (bool)(terrain == STAIRS);
    context->on_object = (bool)(srogue90_bridge_object_type_at(y, x) != '\0');
    context->in_trading_post =
	(bool)(srogue90_bridge_level_type() == SROGUE90_POSTLEV);
    context->on_magic_pool = (bool)(terrain == SROGUE90_POOL);
}

int
srogue90_variant_level_number(void)
{
    return srogue90_bridge_level();
}

int
srogue90_variant_map_rows(void)
{
    return srogue90_bridge_map_rows();
}

int
srogue90_variant_map_cols(void)
{
    return srogue90_bridge_map_cols();
}

bool
srogue90_variant_cell_walkable(int y, int x)
{
    char ch;
    int rows;
    int cols;

    rows = srogue90_bridge_map_rows();
    cols = srogue90_bridge_map_cols();
    if (rows > SROGUE90_MAXLINES)
	rows = SROGUE90_MAXLINES;
    if (cols > SROGUE90_MAXCOLS)
	cols = SROGUE90_MAXCOLS;

    if (y <= 0 || y >= rows || y >= srogue90_bridge_status_row_start()
	|| x < 0 || x >= cols)
	return FALSE;

    ch = srogue90_bridge_visible_ch(y, x);
    switch (ch)
    {
	case ' ':
	case '|':
	case '-':
	    return FALSE;
	case FLOOR:
	case PASSAGE:
	case DOOR:
	case TRAP:
	case STAIRS:
	case GOLD:
	case POTION:
	case SCROLL:
	case MAGIC:
	case FOOD:
	case WEAPON:
	case ARMOR:
	case AMULET:
	case RING:
	case STICK:
	    return TRUE;
	default:
	    return FALSE;
    }
}
