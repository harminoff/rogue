/*
 * RogueTiles variant registry.
 */

#ifndef ROGUE_VARIANT_H
#define ROGUE_VARIANT_H

#include <curses.h>
#include "tiles.h"

#define ROGUE_VARIANT_MAX_FEATURES 8
#define ROGUE_VARIANT_MAX_MANUALS 4

typedef struct rogue_variant_manual_ref {
    const char *title;
    const char *kind;
    const char *location;
    const char *readable_path;
    const char *rights;
    const char *notes;
} ROGUE_VARIANT_MANUAL_REF;

typedef struct rogue_variant_info {
    const char *id;
    const char *name;
    int release_year;
    const char *era;
    const char *lineage;
    const char *license;
    const char *distribution;
    const char *tile_support;
    const char *summary;
    const char *features[ROGUE_VARIANT_MAX_FEATURES];
    const ROGUE_VARIANT_MANUAL_REF manuals[ROGUE_VARIANT_MAX_MANUALS];
} ROGUE_VARIANT_INFO;

typedef struct rogue_variant_status {
    int dungeon_level;
    int gold;
    int hp;
    int max_hit_points;
    unsigned int strength;
    unsigned int strength_base;
    unsigned int dexterity;
    unsigned int dexterity_base;
    unsigned int wisdom;
    unsigned int wisdom_base;
    unsigned int constitution;
    unsigned int constitution_base;
    int armor;
    int exp_level;
    long exp_points;
    int carry_weight;
    int carry_capacity;
    int volume_percent;
    int has_extended_stats;
    int hungry_state;
    const char *message;
} ROGUE_VARIANT_STATUS;

typedef struct rogue_variant_action_context {
    int on_stairs;
    int on_object;
    int in_trading_post;
    int on_magic_pool;
} ROGUE_VARIANT_ACTION_CONTEXT;

bool rogue_variant_init_args(int *argc, char **argv);
const ROGUE_VARIANT_INFO *rogue_variant_current(void);
const ROGUE_VARIANT_INFO *rogue_variant_default(void);
const ROGUE_VARIANT_INFO *rogue_variant_find(const char *id);
const ROGUE_VARIANT_INFO *rogue_variant_at(int index);
int rogue_variant_count(void);
bool rogue_variant_select(const char *id);
bool rogue_variant_is_current(const char *id);
bool rogue_variant_was_explicit(void);
const char *rogue_variant_error(void);
void rogue_variant_describe_cell(int y, int x, ROGUE_TILE_CELL *cell);
void rogue_variant_status(ROGUE_VARIANT_STATUS *status);
void rogue_variant_action_context(ROGUE_VARIANT_ACTION_CONTEXT *context);
void rogue_variant_hero_position(int *y, int *x);
int rogue_variant_level_number(void);
int rogue_variant_map_rows(void);
int rogue_variant_map_cols(void);
bool rogue_variant_cell_walkable(int y, int x);

#endif
