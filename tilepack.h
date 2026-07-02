#ifndef ROGUE_TILEPACK_H
#define ROGUE_TILEPACK_H

#include <curses.h>

typedef struct rogue_tilepack_entry {
    char role[64];
    char name[128];
    int index;
} ROGUE_TILEPACK_ENTRY;

bool rogue_tilepack_load(void);
const char *rogue_tilepack_atlas_path(void);
int rogue_tilepack_columns(void);
int rogue_tilepack_source_width(void);
int rogue_tilepack_source_height(void);
int rogue_tilepack_lookup_index(const char *role, int fallback_index);
const char *rogue_tilepack_lookup_name(const char *role, const char *fallback_name);
const char *rogue_tilepack_status(void);

#endif
