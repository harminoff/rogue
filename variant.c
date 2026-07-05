/*
 * RogueTiles variant registry.
 */

#include <stdio.h>
#include <string.h>
#include <curses.h>
#include "rogue.h"
#include "variant.h"

void rogue52_variant_status(ROGUE_VARIANT_STATUS *status);
void rogue52_variant_hero_position(int *y, int *x);
int rogue52_variant_level_number(void);
int rogue52_variant_map_rows(void);
int rogue52_variant_map_cols(void);
bool rogue52_variant_cell_walkable(int y, int x);
void rogue52_tile_describe_cell(int y, int x, ROGUE_TILE_CELL *cell);
void rogue36_variant_status(ROGUE_VARIANT_STATUS *status);
void rogue36_variant_hero_position(int *y, int *x);
int rogue36_variant_level_number(void);
int rogue36_variant_map_rows(void);
int rogue36_variant_map_cols(void);
bool rogue36_variant_cell_walkable(int y, int x);
void rogue36_tile_describe_cell(int y, int x, ROGUE_TILE_CELL *cell);
void srogue90_variant_status(ROGUE_VARIANT_STATUS *status);
void srogue90_variant_hero_position(int *y, int *x);
int srogue90_variant_level_number(void);
int srogue90_variant_map_rows(void);
int srogue90_variant_map_cols(void);
bool srogue90_variant_cell_walkable(int y, int x);
void srogue90_tile_describe_cell(int y, int x, ROGUE_TILE_CELL *cell);

static const ROGUE_VARIANT_INFO variants[] = {
    {
	"rogue54",
	"Rogue 5.4.4",
	1999,
	"1999 restoration of the 1985 public Rogue 5.4 line",
	"BSD Rogue 5.x lineage",
	"BSD-style",
	"Bundled",
	"Later public Rogue 5.x ruleset",
	"The later Rogue 5.x ruleset used as the default here. Compared with 5.2.1, it has a different monster alphabet, a more scroll-heavy item mix, split identify scrolls, levitation and hallucination potions, and several late-game behavior changes.",
	{
	    "Monster roster uses names such as aquator, emu, venus flytrap, griffin, jabberwock, kestrel, medusa, phantom, rattlesnake, black unicorn, and xeroc.",
	    "X is the disguise monster in this ruleset; M is medusa, whose gaze can confuse the player.",
	    "Object generation favors scrolls more strongly than 5.2.1, with rings, sticks, armor, and weapons slightly rarer.",
	    "Potions include hallucination and levitation instead of 5.2.1's paralysis and thirst-quenching entries.",
	    "Scrolls split identify by item class and include food detection and protect armor rather than 5.2.1's generic identify, blank paper, and genocide.",
	    "Deep dungeon monsters can become hasted after level 29, making the late game sharper than the earlier table.",
	    NULL
	},
	{
	    {
		"Bundled Unix man page",
		"Bundled manual",
		"rogue54.6",
		"rogue54.6",
		"BSD-style source distribution",
		"Short command-line and gameplay overview shipped with this Rogue 5.4.4 source line."
	    },
	    {
		"A Guide to the Dungeons of Doom",
		"Bundled guide",
		"rogue54.doc",
		"rogue54.doc",
		"BSD-style source distribution",
		"Long-form Unix Rogue guide included with the Rogue 5.4.4 source distribution."
	    },
	    { NULL, NULL, NULL, NULL, NULL, NULL }
	}
    },
    {
	"rogue36",
	"Rogue 3.6.2",
	1981,
	"1981 BSD Rogue 3.6 release line",
	"Early BSD Rogue 3.6 lineage",
	"BSD-style",
	"Bundled",
	"First widely released Rogue line",
	"An early Rogue ruleset from the 3.6 line, close to the original public Unix experience. Compared with the later 5.x games, it has older room/window handling, the classic early monster alphabet, a smaller command surface, and older item and score behavior.",
	{
	    "Monster roster uses the early alphabet: giant ant, bat, centaur, dragon, floating eye, violet fungi, gnome, hobgoblin, invisible stalker, mimic, umber hulk, xorn, and others.",
	    "M is the mimic and U is the umber hulk, matching the older pre-5.4 danger alphabet.",
	    "The visible dungeon is driven directly by the early curses windows rather than the later 5.x map arrays.",
	    "Commands include shell escape, save, help, identify, item use, stairs, and classic vi movement; later discovery-list conveniences are not part of this branch.",
	    "Status displays classic strength with optional exceptional strength instead of the later simplified presentation.",
	    "The bundled long guide is the original nroff-style Rogue guide from the 3.6.2 source archive.",
	    NULL
	},
	{
	    {
		"Bundled Rogue 3.6.2 man page",
		"Bundled manual",
		"variants/rogue36/rogue.6",
		"variants/rogue36/rogue.6",
		"BSD-style source distribution",
		"Short command-line and gameplay overview shipped with the Rogue 3.6.2 source."
	    },
	    {
		"Rogue 3.6.2 guide",
		"Bundled guide",
		"variants/rogue36/rogue.r",
		"variants/rogue36/rogue.r",
		"BSD-style source distribution",
		"Long-form nroff guide shipped with the Rogue 3.6.2 source archive."
	    },
	    { NULL, NULL, NULL, NULL, NULL, NULL }
	}
    },
    {
	"rogue52",
	"Rogue 5.2.1",
	1982,
	"1982 BSD Rogue 5.2 release",
	"BSD Rogue 5.2 lineage",
	"BSD-style",
	"Bundled",
	"Earlier Rogue 5.x ruleset",
	"The 1982 5.2 ruleset is an older branch of Rogue 5.x with a more familiar early monster alphabet, different potion and scroll tables, and looser monster treasure behavior. It is useful when you want to feel the older balance beside 5.4.4.",
	{
	    "Monster roster includes giant ant, floating eye, gnome, invisible stalker, mimic, purple worm, quasit, rust monster, umber hulk, xorn, and other older names.",
	    "M is the mimic in this ruleset; U is the gaze-confusion monster, so the danger alphabet differs from 5.4.4.",
	    "Item generation is less scroll-heavy than 5.4.4 and gives slightly more weight to food, weapons, armor, rings, and sticks.",
	    "Potions include paralysis and thirst quenching instead of hallucination and levitation.",
	    "Scrolls include a generic identify scroll, gold detection, blank paper, and genocide.",
	    "Monster carried treasure is less restricted than in 5.4.4, changing how some monster rewards feel.",
	    NULL
	},
	{
	    {
		"Bundled Rogue 5.2 man page",
		"Bundled manual",
		"variants/rogue52/rogue.6",
		"variants/rogue52/rogue.6",
		"BSD-style source distribution",
		"Short command-line and gameplay overview shipped with the Rogue 5.2.1 source."
	    },
	    {
		"A Guide to the Dungeons of Doom",
		"Bundled guide",
		"rogue54.doc",
		"rogue54.doc",
		"BSD-style source distribution",
		"Long-form Unix Rogue guide referenced by the 5.2 manual page and included with the Rogue 5.4.4 source distribution."
	    },
	    { NULL, NULL, NULL, NULL, NULL, NULL }
	}
    },
    {
	"srogue90",
	"Super-Rogue 9.0.1",
	1984,
	"1984 expanded Rogue 3.6 descendant",
	"Super-Rogue line based on Rogue 3.6",
	"BSD-style with name-use restrictions",
	"Bundled",
	"Expanded Rogue ruleset",
	"Super-Rogue 9.0.1 is an ambitious Rogue 3.6 descendant with a much larger monster table, deeper dungeon progression, extra player abilities, carrying volume, trading posts, magic pools, maze levels, and a greatly expanded item catalog.",
	{
	    "Monster roster expands to 52 entries using both uppercase and lowercase letters, including the classic early table plus later threats such as anhkheg, cockatrice, bone devil, green dragon, minotaur, shadow, and Asmodeus.",
	    "The status model adds dexterity, wisdom, constitution, carry weight, and pack volume alongside the familiar level, gold, hit points, armor, and experience fields.",
	    "Trading post levels let the player buy and sell objects for gold, changing dungeon economy beyond ordinary treasure pickup.",
	    "Magic pools support the D command to dip objects, adding a new map feature and item-risk interaction.",
	    "The item catalog is much larger: 21 potion types, 26 scroll types, 26 ring types, 26 stick types, 18 weapons, and 10 armor types.",
	    "The Amulet appears much deeper than early Rogue, with the source setting its earliest level to 35.",
	    NULL
	},
	{
	    {
		"Super-Rogue 9.0.1 guide",
		"Bundled guide",
		"variants/srogue90/rogue.nr",
		"variants/srogue90/rogue.nr",
		"BSD-style source distribution",
		"Long-form Super-Rogue tutorial and command guide shipped with the source archive."
	    },
	    {
		"Super-Rogue 9.0.1 license",
		"Bundled license",
		"variants/srogue90/LICENSE.TXT",
		"variants/srogue90/LICENSE.TXT",
		"BSD-style source distribution with name-use restrictions",
		"Redistribution terms for Super-Rogue and its Rogue-derived portions."
	    },
	    { NULL, NULL, NULL, NULL, NULL, NULL }
	}
    }
};

static const ROGUE_VARIANT_INFO *current_variant = &variants[0];
static const char *variant_error = NULL;
static bool variant_explicit = FALSE;

static bool
str_equal(const char *left, const char *right)
{
    if (left == NULL || right == NULL)
	return FALSE;
    return (bool)(strcmp(left, right) == 0);
}

int
rogue_variant_count(void)
{
    return (int)(sizeof(variants) / sizeof(variants[0]));
}

const ROGUE_VARIANT_INFO *
rogue_variant_at(int index)
{
    if (index < 0 || index >= rogue_variant_count())
	return NULL;
    return &variants[index];
}

const ROGUE_VARIANT_INFO *
rogue_variant_default(void)
{
    return &variants[0];
}

const ROGUE_VARIANT_INFO *
rogue_variant_current(void)
{
    return current_variant;
}

const ROGUE_VARIANT_INFO *
rogue_variant_find(const char *id)
{
    int i;

    for (i = 0; i < rogue_variant_count(); i++)
	if (str_equal(variants[i].id, id))
	    return &variants[i];
    return NULL;
}

bool
rogue_variant_select(const char *id)
{
    const ROGUE_VARIANT_INFO *found;

    found = rogue_variant_find(id);
    if (found == NULL)
    {
	variant_error = "Unknown Rogue variant.";
	return FALSE;
    }

    current_variant = found;
    variant_error = NULL;
    return TRUE;
}

bool
rogue_variant_is_current(const char *id)
{
    return str_equal(current_variant->id, id);
}

bool
rogue_variant_was_explicit(void)
{
    return variant_explicit;
}

const char *
rogue_variant_error(void)
{
    return variant_error;
}

bool
rogue_variant_init_args(int *argc, char **argv)
{
    int read_idx, write_idx;
    const char *id;

    write_idx = 1;
    for (read_idx = 1; read_idx < *argc; read_idx++)
    {
	if (strcmp(argv[read_idx], "--variant") == 0)
	{
	    if (read_idx + 1 >= *argc)
	    {
		variant_error = "--variant requires a variant id.";
		return FALSE;
	    }
	    id = argv[++read_idx];
	    if (!rogue_variant_select(id))
		return FALSE;
	    variant_explicit = TRUE;
	}
	else if (strncmp(argv[read_idx], "--variant=", 10) == 0)
	{
	    id = argv[read_idx] + 10;
	    if (!rogue_variant_select(id))
		return FALSE;
	    variant_explicit = TRUE;
	}
	else
	    argv[write_idx++] = argv[read_idx];
    }

    argv[write_idx] = NULL;
    *argc = write_idx;
    return TRUE;
}

void
rogue_variant_describe_cell(int y, int x, ROGUE_TILE_CELL *cell)
{
    if (rogue_variant_is_current("rogue52"))
    {
	rogue52_tile_describe_cell(y, x, cell);
	return;
    }
    if (rogue_variant_is_current("rogue36"))
    {
	rogue36_tile_describe_cell(y, x, cell);
	return;
    }
    if (rogue_variant_is_current("srogue90"))
    {
	srogue90_tile_describe_cell(y, x, cell);
	return;
    }

    rogue_tile_describe_cell(y, x, cell);
}

void
rogue_variant_status(ROGUE_VARIANT_STATUS *status)
{
    if (status == NULL)
	return;

    memset(status, 0, sizeof(*status));

    if (rogue_variant_is_current("rogue52"))
    {
	rogue52_variant_status(status);
	return;
    }
    if (rogue_variant_is_current("rogue36"))
    {
	rogue36_variant_status(status);
	return;
    }
    if (rogue_variant_is_current("srogue90"))
    {
	srogue90_variant_status(status);
	return;
    }

    status->dungeon_level = level;
    status->gold = purse;
    status->hp = pstats.s_hpt;
    status->max_hit_points = max_hp;
    status->strength = pstats.s_str;
    status->armor = (cur_armor != NULL ? cur_armor->o_arm : pstats.s_arm);
    status->exp_level = pstats.s_lvl;
    status->exp_points = pstats.s_exp;
    status->hungry_state = hungry_state;
    status->message = huh;
}

void
rogue_variant_hero_position(int *y, int *x)
{
    if (rogue_variant_is_current("rogue52"))
    {
	rogue52_variant_hero_position(y, x);
	return;
    }
    if (rogue_variant_is_current("rogue36"))
    {
	rogue36_variant_hero_position(y, x);
	return;
    }
    if (rogue_variant_is_current("srogue90"))
    {
	srogue90_variant_hero_position(y, x);
	return;
    }

    if (y != NULL)
	*y = hero.y;
    if (x != NULL)
	*x = hero.x;
}

int
rogue_variant_level_number(void)
{
    if (rogue_variant_is_current("rogue52"))
	return rogue52_variant_level_number();
    if (rogue_variant_is_current("rogue36"))
	return rogue36_variant_level_number();
    if (rogue_variant_is_current("srogue90"))
	return srogue90_variant_level_number();
    return level;
}

int
rogue_variant_map_rows(void)
{
    if (rogue_variant_is_current("rogue52"))
	return rogue52_variant_map_rows();
    if (rogue_variant_is_current("rogue36"))
	return rogue36_variant_map_rows();
    if (rogue_variant_is_current("srogue90"))
	return srogue90_variant_map_rows();
    return NUMLINES;
}

int
rogue_variant_map_cols(void)
{
    if (rogue_variant_is_current("rogue52"))
	return rogue52_variant_map_cols();
    if (rogue_variant_is_current("rogue36"))
	return rogue36_variant_map_cols();
    if (rogue_variant_is_current("srogue90"))
	return srogue90_variant_map_cols();
    return NUMCOLS;
}

bool
rogue_variant_cell_walkable(int y, int x)
{
    char ch;

    if (rogue_variant_is_current("rogue52"))
	return rogue52_variant_cell_walkable(y, x);
    if (rogue_variant_is_current("rogue36"))
	return rogue36_variant_cell_walkable(y, x);
    if (rogue_variant_is_current("srogue90"))
	return srogue90_variant_cell_walkable(y, x);

    if (y <= 0 || y >= NUMLINES - 1 || x < 0 || x >= NUMCOLS)
	return FALSE;

    ch = chat(y, x);
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
