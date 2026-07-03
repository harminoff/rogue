/*
 * Rogue tile presentation descriptors.
 *
 * This layer describes the current game state in semantic tile terms without
 * changing the curses renderer or gameplay rules.
 */

#ifndef ROGUE_TILES_H
#define ROGUE_TILES_H

#include <curses.h>

typedef enum rogue_tile_layer {
    ROGUE_TILE_EMPTY,
    ROGUE_TILE_TERRAIN,
    ROGUE_TILE_OBJECT,
    ROGUE_TILE_ACTOR
} ROGUE_TILE_LAYER;

typedef struct rogue_tile_cell {
    int y;
    int x;
    char glyph;
    ROGUE_TILE_LAYER layer;
    const char *role;
    const char *atlas_key;
    int atlas_index;
    bool has_underlay;
    char under_glyph;
    const char *under_role;
    const char *under_atlas_key;
    int under_atlas_index;
    const char *name;
    bool seen;
    bool visible;
} ROGUE_TILE_CELL;

void rogue_tile_describe_cell(int y, int x, ROGUE_TILE_CELL *cell);
void rogue52_tile_describe_cell(int y, int x, ROGUE_TILE_CELL *cell);
void rogue36_tile_describe_cell(int y, int x, ROGUE_TILE_CELL *cell);
const char *rogue_tile_layer_name(ROGUE_TILE_LAYER layer);

#endif
