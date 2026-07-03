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
#include "generated/rogue_tile_mapping.h"

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
terrain_glyph_at(int y, int x, char glyph)
{
    char flags;

    flags = flat(y, x);

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

static void
apply_underlay(int y, int x, char glyph, ROGUE_TILE_CELL *cell)
{
    const ROGUE_GENERATED_TILE_MAPPING *mapping;
    char terrain_glyph;

    terrain_glyph = terrain_glyph_at(y, x, glyph);
    mapping = find_glyph_mapping(terrain_glyph);
    if (mapping == NULL || mapping->atlas_index < 0)
	return;

    cell->has_underlay = TRUE;
    cell->under_glyph = terrain_glyph;
    cell->under_role = mapping->role;
    cell->under_atlas_key = mapping->atlas_key;
    cell->under_atlas_index = mapping->atlas_index;
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
    cell->role = "monster";
    cell->atlas_key = (mapping == NULL) ? NULL : mapping->atlas_key;
    cell->atlas_index = (mapping == NULL) ? -1 : mapping->atlas_index;
    cell->has_underlay = FALSE;
    cell->under_glyph = ' ';
    cell->under_role = NULL;
    cell->under_atlas_key = NULL;
    cell->under_atlas_index = -1;
    cell->name = (mapping == NULL) ? "monster" : mapping->name;
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
