#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include "rogue_platform.h"
#include "tilepack.h"
#include "generated/rogue_tile_mapping.h"

#define ROGUE_TILEPACK_MAX_ENTRIES 512

static ROGUE_TILEPACK_ENTRY entries[ROGUE_TILEPACK_MAX_ENTRIES];
static int entry_count = 0;
static char atlas_path[512] = "assets/rltiles/rltiles-2d.png";
static int atlas_columns = 30;
static int source_width = 32;
static int source_height = 32;
static char status_text[256] = "built-in generated tile mapping";
static char current_pack_id[64] = "generated";
static bool generated_fallback_safe = TRUE;
static bool loaded = FALSE;

static const char *
skip_ws(const char *p)
{
    while (*p != '\0' && isspace((unsigned char) *p))
	p++;
    return p;
}

static const char *
parse_json_string(const char *p, char *out, size_t out_size)
{
    char *w;

    p = skip_ws(p);
    if (*p != '"')
	return NULL;

    p++;
    w = out;
    while (*p != '\0' && *p != '"')
    {
	if (*p == '\\' && p[1] != '\0')
	    p++;
	if ((size_t) (w - out) + 1 < out_size)
	    *w++ = *p;
	p++;
    }

    if (*p != '"')
	return NULL;

    *w = '\0';
    return p + 1;
}

static bool
json_string_field(const char *json, const char *key, char *out, size_t out_size)
{
    char pattern[96];
    const char *p;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(json, pattern);
    if (p == NULL)
	return FALSE;

    p = strchr(p + strlen(pattern), ':');
    if (p == NULL)
	return FALSE;

    return parse_json_string(p + 1, out, out_size) != NULL;
}

static bool
json_int_field(const char *json, const char *key, int *out)
{
    char pattern[96];
    const char *p;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(json, pattern);
    if (p == NULL)
	return FALSE;

    p = strchr(p + strlen(pattern), ':');
    if (p == NULL)
	return FALSE;

    p = skip_ws(p + 1);
    if (*p == '\0')
	return FALSE;

    *out = atoi(p);
    return TRUE;
}

static bool
json_bool_field(const char *json, const char *key, bool *out)
{
    char pattern[96];
    const char *p;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(json, pattern);
    if (p == NULL)
	return FALSE;

    p = strchr(p + strlen(pattern), ':');
    if (p == NULL)
	return FALSE;

    p = skip_ws(p + 1);
    if (strncmp(p, "true", 4) == 0)
    {
	*out = TRUE;
	return TRUE;
    }
    if (strncmp(p, "false", 5) == 0)
    {
	*out = FALSE;
	return TRUE;
    }

    return FALSE;
}

static const char *
object_end(const char *p)
{
    int depth;
    bool in_string;
    bool escaped;

    depth = 0;
    in_string = FALSE;
    escaped = FALSE;

    while (*p != '\0')
    {
	if (in_string)
	{
	    if (escaped)
		escaped = FALSE;
	    else if (*p == '\\')
		escaped = TRUE;
	    else if (*p == '"')
		in_string = FALSE;
	}
	else
	{
	    if (*p == '"')
		in_string = TRUE;
	    else if (*p == '{')
		depth++;
	    else if (*p == '}')
	    {
		depth--;
		if (depth == 0)
		    return p + 1;
	    }
	}
	p++;
    }

    return NULL;
}

static void
dirname_of(const char *path, char *out, size_t out_size)
{
    char *slash;

    strncpy(out, path, out_size - 1);
    out[out_size - 1] = '\0';

    slash = strrchr(out, '/');
    if (slash == NULL)
	slash = strrchr(out, '\\');

    if (slash == NULL)
    {
	strncpy(out, ".", out_size - 1);
	out[out_size - 1] = '\0';
	return;
    }

    *slash = '\0';
}

static void
join_path(const char *dir, const char *name, char *out, size_t out_size)
{
    snprintf(out, out_size, "%s/%s", dir, name);
}

static bool
valid_pack_id(const char *pack_id)
{
    const char *p;

    if (pack_id == NULL || *pack_id == '\0')
	return FALSE;

    for (p = pack_id; *p != '\0'; p++)
	if (!isalnum((unsigned char) *p) && *p != '_' && *p != '-')
	    return FALSE;

    return TRUE;
}

static void
pack_tilepack_path(const char *pack_id, char *out, size_t out_size)
{
    snprintf(out, out_size, "tilepacks/%s/tilepack.json", pack_id);
}

static bool
tilepack_json_summary(const char *tilepack_path, char *label,
		      size_t label_size)
{
    char *json;
    char image_name[256];
    char mapping_name[256];
    char resolved_tilepack_path[512];
    const char *display_path;
    int width, height, columns;

    display_path = rogue_platform_asset_path(tilepack_path,
					     resolved_tilepack_path,
					     sizeof(resolved_tilepack_path));
    json = rogue_platform_read_text_file(tilepack_path);
    if (json == NULL)
	return FALSE;

    if (!json_string_field(json, "image", image_name, sizeof(image_name))
	|| !json_string_field(json, "mapping", mapping_name,
			      sizeof(mapping_name))
	|| !json_int_field(json, "tileWidth", &width)
	|| !json_int_field(json, "tileHeight", &height)
	|| !json_int_field(json, "columns", &columns)
	|| width <= 0 || height <= 0 || columns <= 0)
    {
	free(json);
	return FALSE;
    }

    if (!json_string_field(json, "name", label, label_size)
	|| label[0] == '\0')
    {
	strncpy(label, display_path, label_size - 1);
	label[label_size - 1] = '\0';
    }

    free(json);
    return TRUE;
}

static bool
add_choice(ROGUE_TILEPACK_CHOICE *choices, int *count, int max_choices,
	   const char *pack_id)
{
    ROGUE_TILEPACK_CHOICE *choice;
    char tilepack_path[512];
    char label[128];
    int i;

    if (choices == NULL || count == NULL || *count >= max_choices
	|| !valid_pack_id(pack_id))
	return FALSE;

    for (i = 0; i < *count; i++)
	if (strcmp(choices[i].id, pack_id) == 0)
	    return FALSE;

    pack_tilepack_path(pack_id, tilepack_path, sizeof(tilepack_path));
    if (!tilepack_json_summary(tilepack_path, label, sizeof(label)))
	return FALSE;

    choice = &choices[*count];
    strncpy(choice->id, pack_id, sizeof(choice->id) - 1);
    choice->id[sizeof(choice->id) - 1] = '\0';
    if (strcmp(pack_id, "active") == 0)
	snprintf(choice->label, sizeof(choice->label), "Active - %s", label);
    else if (strcmp(pack_id, "default") == 0)
	snprintf(choice->label, sizeof(choice->label), "Default - %s", label);
    else
	snprintf(choice->label, sizeof(choice->label), "%s", label);
    snprintf(choice->tilepack_path, sizeof(choice->tilepack_path), "%s",
	     tilepack_path);
    choice->current = (bool)(strcmp(current_pack_id, pack_id) == 0);
    (*count)++;
    return TRUE;
}

static void
reset_to_generated(void)
{
    entry_count = 0;
    strncpy(atlas_path, rogue_tile_atlas_path(), sizeof(atlas_path) - 1);
    atlas_path[sizeof(atlas_path) - 1] = '\0';
    atlas_columns = rogue_tile_atlas_columns();
    source_width = rogue_tile_atlas_source_width();
    source_height = rogue_tile_atlas_source_height();
    generated_fallback_safe = TRUE;
    strncpy(current_pack_id, "generated", sizeof(current_pack_id) - 1);
    current_pack_id[sizeof(current_pack_id) - 1] = '\0';
}

static bool
add_entry(const char *role, int index, const char *name)
{
    ROGUE_TILEPACK_ENTRY *entry;

    if (entry_count >= ROGUE_TILEPACK_MAX_ENTRIES)
	return FALSE;

    entry = &entries[entry_count++];
    strncpy(entry->role, role, sizeof(entry->role) - 1);
    entry->role[sizeof(entry->role) - 1] = '\0';
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0';
    entry->index = index;
    return TRUE;
}

static void
parse_mapping_roles(const char *json)
{
    const char *roles;
    const char *p;
    const char *end;
    char role[64];
    char name[128];
    int index;
    char object_text[1024];
    const char *object_stop;
    size_t object_len;

    roles = strstr(json, "\"roles\"");
    if (roles == NULL)
	return;

    p = strchr(roles, '{');
    if (p == NULL)
	return;
    end = object_end(p);
    if (end == NULL)
	return;
    p++;

    while (p < end && *p != '\0')
    {
	p = skip_ws(p);
	if (*p == ',')
	{
	    p++;
	    continue;
	}
	if (*p == '}')
	    break;

	p = parse_json_string(p, role, sizeof(role));
	if (p == NULL)
	    break;
	p = skip_ws(p);
	if (*p != ':')
	    break;
	p = skip_ws(p + 1);
	if (*p != '{')
	    break;

	object_stop = object_end(p);
	if (object_stop == NULL)
	    break;
	object_len = (size_t) (object_stop - p);
	if (object_len == 0 || object_len >= sizeof(object_text))
	    break;
	memcpy(object_text, p, object_len);
	object_text[object_len] = '\0';
	p += object_len;

	if (!json_int_field(object_text, "index", &index))
	    continue;
	if (!json_string_field(object_text, "name", name, sizeof(name)))
	{
	    strncpy(name, role, sizeof(name) - 1);
	    name[sizeof(name) - 1] = '\0';
	}

	add_entry(role, index, name);
    }
}

static bool
load_tilepack_file(const char *tilepack_path, const char *pack_id)
{
    char *tilepack_json;
    char *mapping_json;
    char dir[512];
    char image_name[256];
    char mapping_name[256];
    char pack_name[128];
    char mapping_path[512];
    bool allow_generated_fallback;

    tilepack_json = rogue_platform_read_text_file(tilepack_path);
    if (tilepack_json == NULL)
	return FALSE;

    if (!json_string_field(tilepack_json, "image", image_name, sizeof(image_name))
	|| !json_string_field(tilepack_json, "mapping", mapping_name,
			      sizeof(mapping_name))
	|| !json_int_field(tilepack_json, "tileWidth", &source_width)
	|| !json_int_field(tilepack_json, "tileHeight", &source_height)
	|| !json_int_field(tilepack_json, "columns", &atlas_columns)
	|| source_width <= 0 || source_height <= 0 || atlas_columns <= 0)
    {
	free(tilepack_json);
	return FALSE;
    }

    if (!json_string_field(tilepack_json, "name", pack_name,
			   sizeof(pack_name)))
	pack_name[0] = '\0';
    if (!json_bool_field(tilepack_json, "fallbackToGenerated",
			 &allow_generated_fallback))
	allow_generated_fallback = (bool)(strcmp(pack_name,
						 "Default RL Tiles") == 0);

    dirname_of(tilepack_path, dir, sizeof(dir));
    join_path(dir, image_name, atlas_path, sizeof(atlas_path));
    join_path(dir, mapping_name, mapping_path, sizeof(mapping_path));

    mapping_json = rogue_platform_read_text_file(mapping_path);
    if (mapping_json == NULL)
    {
	free(tilepack_json);
	return FALSE;
    }

    entry_count = 0;
    parse_mapping_roles(mapping_json);
    generated_fallback_safe = allow_generated_fallback;
    if (pack_id != NULL && *pack_id != '\0')
    {
	strncpy(current_pack_id, pack_id, sizeof(current_pack_id) - 1);
	current_pack_id[sizeof(current_pack_id) - 1] = '\0';
    }
    snprintf(status_text, sizeof(status_text), "loaded %s with %d role mappings",
	     tilepack_path, entry_count);
    free(mapping_json);
    free(tilepack_json);
    return entry_count > 0;
}

bool
rogue_tilepack_load(void)
{
    if (loaded)
	return TRUE;

    loaded = TRUE;
    reset_to_generated();

    if (load_tilepack_file("tilepacks/active/tilepack.json", "active"))
	return TRUE;
    if (load_tilepack_file("tilepacks/default/tilepack.json", "default"))
	return TRUE;

    snprintf(status_text, sizeof(status_text),
	     "using generated fallback tile mapping");
    return FALSE;
}

bool
rogue_tilepack_load_named(const char *pack_id)
{
    char tilepack_path[512];

    if (!valid_pack_id(pack_id))
	return FALSE;

    loaded = TRUE;
    reset_to_generated();
    pack_tilepack_path(pack_id, tilepack_path, sizeof(tilepack_path));
    if (load_tilepack_file(tilepack_path, pack_id))
	return TRUE;

    if (load_tilepack_file("tilepacks/default/tilepack.json", "default"))
    {
	snprintf(status_text, sizeof(status_text),
		 "failed %s, loaded default tilepack", tilepack_path);
	return FALSE;
    }

    snprintf(status_text, sizeof(status_text),
	     "failed %s, using generated fallback tile mapping", tilepack_path);
    return FALSE;
}

int
rogue_tilepack_list(ROGUE_TILEPACK_CHOICE *choices, int max_choices)
{
    DIR *dir;
    struct dirent *entry;
    char tilepacks_path[512];
    int count;

    if (choices == NULL || max_choices <= 0)
	return 0;

    rogue_tilepack_load();
    count = 0;
    add_choice(choices, &count, max_choices, "active");
    add_choice(choices, &count, max_choices, "default");

    dir = opendir(rogue_platform_asset_path("tilepacks", tilepacks_path,
					    sizeof(tilepacks_path)));
    if (dir == NULL)
	return count;

    while ((entry = readdir(dir)) != NULL)
    {
	if (strcmp(entry->d_name, ".") == 0
	    || strcmp(entry->d_name, "..") == 0
	    || strcmp(entry->d_name, "active") == 0
	    || strcmp(entry->d_name, "default") == 0)
	    continue;
	add_choice(choices, &count, max_choices, entry->d_name);
	if (count >= max_choices)
	    break;
    }

    closedir(dir);
    return count;
}

const char *
rogue_tilepack_atlas_path(void)
{
    rogue_tilepack_load();
    return atlas_path;
}

int
rogue_tilepack_columns(void)
{
    rogue_tilepack_load();
    return atlas_columns;
}

int
rogue_tilepack_source_width(void)
{
    rogue_tilepack_load();
    return source_width;
}

int
rogue_tilepack_source_height(void)
{
    rogue_tilepack_load();
    return source_height;
}

int
rogue_tilepack_lookup_index(const char *role, int fallback_index)
{
    int i;

    rogue_tilepack_load();
    if (role == NULL)
	return fallback_index;

    for (i = 0; i < entry_count; i++)
	if (strcmp(entries[i].role, role) == 0 && entries[i].index >= 0)
	    return entries[i].index;

    if (!generated_fallback_safe)
	return -1;

    return fallback_index;
}

const char *
rogue_tilepack_lookup_name(const char *role, const char *fallback_name)
{
    int i;

    rogue_tilepack_load();
    if (role == NULL)
	return fallback_name;

    for (i = 0; i < entry_count; i++)
	if (strcmp(entries[i].role, role) == 0)
	    return entries[i].name;

    return fallback_name;
}

const char *
rogue_tilepack_status(void)
{
    rogue_tilepack_load();
    return status_text;
}

const char *
rogue_tilepack_current_id(void)
{
    rogue_tilepack_load();
    return current_pack_id;
}
