#ifndef ROGUE_TILEPACK_H
#define ROGUE_TILEPACK_H

#include <curses.h>

typedef struct rogue_tilepack_entry {
    char role[64];
    char name[128];
    int index;
} ROGUE_TILEPACK_ENTRY;

#define ROGUE_TILEPACK_MAX_CHOICES 32

typedef struct rogue_tilepack_choice {
    char id[64];
    char label[128];
    char tilepack_path[512];
    bool current;
} ROGUE_TILEPACK_CHOICE;

bool rogue_tilepack_load(void);
bool rogue_tilepack_load_named(const char *pack_id);
int rogue_tilepack_list(ROGUE_TILEPACK_CHOICE *choices, int max_choices);
const char *rogue_tilepack_atlas_path(void);
int rogue_tilepack_columns(void);
int rogue_tilepack_source_width(void);
int rogue_tilepack_source_height(void);
int rogue_tilepack_lookup_index(const char *role, int fallback_index);
const char *rogue_tilepack_lookup_name(const char *role, const char *fallback_name);
const char *rogue_tilepack_status(void);
const char *rogue_tilepack_current_id(void);

#endif
