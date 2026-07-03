/*
 * Allegro 5 tile frontend.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <curses.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>
#include "rogue.h"
#include "tiles.h"
#include "overlay_picker.h"
#include "tilepack.h"

#define ROGUE_DEFAULT_TILE_DRAW_SIZE 32
#define ROGUE_STATUS_HEIGHT 96
#define ROGUE_MIN_TILE_DRAW_SIZE 16
#define ROGUE_MAX_TILE_DRAW_SIZE 64
#define ROGUE_TILE_ZOOM_STEP 4
#define ROGUE_DEFAULT_VIEW_COLS 40
#define ROGUE_MAX_VIEW_COLS NUMCOLS
#define ROGUE_MAX_VIEW_ROWS (NUMLINES - 1)
#define ROGUE_REPEAT_DELAY_SECONDS 0.30
#define ROGUE_REPEAT_RATE_SECONDS 0.13
#define ROGUE_FONT_SIZE 32
#define ROGUE_OVERLAY_MAX_LINES 160
#define ROGUE_OVERLAY_LINE_LEN 160
#define ROGUE_MESSAGE_LOG_LINES 64
#define ROGUE_SIDE_PANEL_MIN_WIDTH 280
#define ROGUE_SIDE_PANEL_MAX_WIDTH 420
#define ROGUE_BLOOD_SPLATS 96
#define ROGUE_BLOOD_DROPS_PER_HIT 5
#define ROGUE_MIN_WALL_THICKNESS 1
#define ROGUE_DEFAULT_WALL_THICKNESS 2
#define ROGUE_MAX_WALL_THICKNESS 3

typedef enum rogue_allegro_view {
    ROGUE_ALLEGRO_VIEW_TILES,
    ROGUE_ALLEGRO_VIEW_GLYPHS
} ROGUE_ALLEGRO_VIEW;

typedef struct rogue_allegro_settings {
    int tile_draw_size;
    ROGUE_ALLEGRO_VIEW view_mode;
    bool shader_enabled;
    bool blood_spatter_enabled;
    bool side_panel_log_enabled;
    bool stylized_log_enabled;
    bool stylized_bottom_bar_enabled;
    int wall_thickness;
    bool fullscreen;
    int windowed_width;
    int windowed_height;
} ROGUE_ALLEGRO_SETTINGS;

typedef struct rogue_blood_splat {
    int x;
    int y;
    int level;
    int radius;
    int alpha;
    int offset_x;
    int offset_y;
} ROGUE_BLOOD_SPLAT;

static ROGUE_ALLEGRO_SETTINGS settings = {
    ROGUE_DEFAULT_TILE_DRAW_SIZE,
    ROGUE_ALLEGRO_VIEW_TILES,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    ROGUE_DEFAULT_WALL_THICKNESS,
    FALSE,
    ROGUE_DEFAULT_VIEW_COLS * ROGUE_DEFAULT_TILE_DRAW_SIZE,
    ROGUE_MAX_VIEW_ROWS * ROGUE_DEFAULT_TILE_DRAW_SIZE + ROGUE_STATUS_HEIGHT
};

#define ROGUE_TILE_DRAW_SIZE (settings.tile_draw_size)

static ALLEGRO_DISPLAY *display = NULL;
static ALLEGRO_EVENT_QUEUE *queue = NULL;
static ALLEGRO_BITMAP *atlas = NULL;
static ALLEGRO_FONT *font = NULL;
static bool started = FALSE;
static bool smoke_mode = FALSE;
static int suppress_key_char_keycode = 0;
static int held_movement_keycode = 0;
static char held_movement = '\0';
static double held_movement_next_time = 0.0;
static int render_origin_x = 0;
static int render_origin_y = 0;
static char message_log[ROGUE_MESSAGE_LOG_LINES][ROGUE_OVERLAY_LINE_LEN];
static int message_damage_dealt[ROGUE_MESSAGE_LOG_LINES];
static int message_damage_taken[ROGUE_MESSAGE_LOG_LINES];
static int message_enemy_hp[ROGUE_MESSAGE_LOG_LINES];
static int message_enemy_max_hp[ROGUE_MESSAGE_LOG_LINES];
static int message_log_count = 0;
static int pending_damage_dealt = 0;
static int pending_damage_taken = 0;
static int pending_enemy_hp = 0;
static int pending_enemy_max_hp = 0;
static ROGUE_BLOOD_SPLAT blood_splats[ROGUE_BLOOD_SPLATS];
static int blood_splat_count = 0;
static unsigned int blood_rng = 0x6d2b79f5u;
static bool prompt_active = FALSE;
static char prompt_text[128];
static bool death_overlay_active = FALSE;
static char death_title[128];
static char death_killer[128];
static char death_gold[64];
static char death_prompt[128];
static bool text_overlay_active = FALSE;
static char text_overlay_title[64];
static char text_overlay_prompt[128];
static char text_overlay_lines[ROGUE_OVERLAY_MAX_LINES][ROGUE_OVERLAY_LINE_LEN];
static int text_overlay_line_count = 0;
static int text_overlay_selected = -1;
static int text_overlay_scroll = 0;
static bool text_overlay_selectable = FALSE;
static char settings_path[512] = "settings.json";

void rogue_allegro_text_overlay_begin(const char *title);
void rogue_allegro_text_overlay_add(const char *line);
char rogue_allegro_text_overlay_pick(const char *prompt);
void rogue_allegro_text_overlay_clear(void);
bool rogue_allegro_notice(const char *title, const char *message);
void rogue_allegro_render(void);

static void
allegro_start_error(const char *message)
{
    fprintf(stderr, "%s\n", message);
#ifdef _WIN32
    MessageBoxA(NULL, message, "RogueTiles", MB_OK | MB_ICONERROR);
#endif
}

static bool
is_movement_command(char ch)
{
    switch (ch)
    {
	case 'h':
	case 'j':
	case 'k':
	case 'l':
	case 'y':
	case 'u':
	case 'b':
	case 'n':
	case '.':
	    return TRUE;
	default:
	    return FALSE;
    }
}

static void
clear_held_movement(void)
{
    held_movement_keycode = 0;
    held_movement = '\0';
    held_movement_next_time = 0.0;
}

static bool
movement_key_is_down(int keycode)
{
    ALLEGRO_KEYBOARD_STATE state;

    if (keycode <= 0)
	return FALSE;

    al_get_keyboard_state(&state);
    return (bool) al_key_down(&state, keycode);
}

static void
begin_held_movement(int keycode, char mapped)
{
    held_movement_keycode = keycode;
    held_movement = mapped;
    held_movement_next_time = al_get_time() + ROGUE_REPEAT_DELAY_SECONDS;
}

static ALLEGRO_FONT *
load_ui_font(void)
{
    ALLEGRO_FONT *loaded;

    loaded = al_load_ttf_font("assets/fonts/monogram/monogram.ttf",
			      ROGUE_FONT_SIZE, ALLEGRO_TTF_MONOCHROME);
    if (loaded == NULL)
	loaded = al_load_ttf_font("../assets/fonts/monogram/monogram.ttf",
				  ROGUE_FONT_SIZE, ALLEGRO_TTF_MONOCHROME);
    if (loaded == NULL)
	loaded = al_create_builtin_font();

    return loaded;
}

static bool
json_bool_field(const char *json, const char *key, bool fallback)
{
    char pattern[64];
    const char *p;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(json, pattern);
    if (p == NULL)
	return fallback;
    p = strchr(p + strlen(pattern), ':');
    if (p == NULL)
	return fallback;
    p++;
    while (*p != '\0' && isspace((unsigned char) *p))
	p++;
    if (strncmp(p, "true", 4) == 0)
	return TRUE;
    if (strncmp(p, "false", 5) == 0)
	return FALSE;
    return fallback;
}

static int
json_int_field(const char *json, const char *key, int fallback,
	       int min_value, int max_value)
{
    char pattern[64];
    const char *p;
    char *end;
    long value;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(json, pattern);
    if (p == NULL)
	return fallback;
    p = strchr(p + strlen(pattern), ':');
    if (p == NULL)
	return fallback;
    p++;
    while (*p != '\0' && isspace((unsigned char) *p))
	p++;

    value = strtol(p, &end, 10);
    if (end == p)
	return fallback;
    if (value < min_value)
	return min_value;
    if (value > max_value)
	return max_value;
    return (int) value;
}

static void
init_settings_path(void)
{
#ifdef _WIN32
    char exe_path[512];
    char *slash;

    if (GetModuleFileNameA(NULL, exe_path, sizeof(exe_path)) > 0)
    {
	exe_path[sizeof(exe_path) - 1] = '\0';
	slash = strrchr(exe_path, '\\');
	if (slash == NULL)
	    slash = strrchr(exe_path, '/');
	if (slash != NULL)
	{
	    *slash = '\0';
	    snprintf(settings_path, sizeof(settings_path), "%s\\settings.json",
		     exe_path);
	}
    }
#endif
}

static void
load_settings(void)
{
    FILE *file;
    char text[2048];
    size_t read_count;

    file = fopen(settings_path, "rb");
    if (file == NULL)
	return;

    read_count = fread(text, 1, sizeof(text) - 1, file);
    fclose(file);
    text[read_count] = '\0';

    settings.side_panel_log_enabled = json_bool_field(
	text, "sidePanelLog", settings.side_panel_log_enabled);
    settings.stylized_log_enabled = json_bool_field(
	text, "stylizedLog", settings.stylized_log_enabled);
    settings.stylized_bottom_bar_enabled = json_bool_field(
	text, "stylizedBottomBar", settings.stylized_bottom_bar_enabled);
    settings.blood_spatter_enabled = json_bool_field(
	text, "bloodSpatter", settings.blood_spatter_enabled);
    settings.shader_enabled = json_bool_field(
	text, "shaders", settings.shader_enabled);
    settings.wall_thickness = json_int_field(
	text, "wallThickness", settings.wall_thickness,
	ROGUE_MIN_WALL_THICKNESS, ROGUE_MAX_WALL_THICKNESS);
}

static void
save_settings(void)
{
    FILE *file;

    file = fopen(settings_path, "wb");
    if (file == NULL)
	return;

    fprintf(file,
	    "{\n"
	    "  \"sidePanelLog\": %s,\n"
	    "  \"stylizedLog\": %s,\n"
	    "  \"stylizedBottomBar\": %s,\n"
	    "  \"bloodSpatter\": %s,\n"
	    "  \"shaders\": %s,\n"
	    "  \"wallThickness\": %d\n"
	    "}\n",
	    settings.side_panel_log_enabled ? "true" : "false",
	    settings.stylized_log_enabled ? "true" : "false",
	    settings.stylized_bottom_bar_enabled ? "true" : "false",
	    settings.blood_spatter_enabled ? "true" : "false",
	    settings.shader_enabled ? "true" : "false",
	    settings.wall_thickness);
    fclose(file);
}

static int
clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value)
	return min_value;
    if (value > max_value)
	return max_value;
    return value;
}

static int
display_width(void)
{
    if (display == NULL)
	return settings.windowed_width;
    return al_get_display_width(display);
}

static int
display_height(void)
{
    if (display == NULL)
	return settings.windowed_height;
    return al_get_display_height(display);
}

static int
side_panel_width(void)
{
    int width;
    int max_width;

    if (!settings.side_panel_log_enabled)
	return 0;

    max_width = display_width() - ROGUE_TILE_DRAW_SIZE;
    if (max_width <= 0)
	return 0;
    width = display_width() / 4;
    width = clamp_int(width, ROGUE_SIDE_PANEL_MIN_WIDTH,
		      ROGUE_SIDE_PANEL_MAX_WIDTH);
    if (width > max_width)
	width = max_width;
    return width;
}

static int
play_area_width(void)
{
    int width;

    width = display_width() - side_panel_width();
    if (width < ROGUE_TILE_DRAW_SIZE)
	width = ROGUE_TILE_DRAW_SIZE;
    return width;
}

static int
view_cols(void)
{
    int cols;

    cols = play_area_width() / ROGUE_TILE_DRAW_SIZE;
    return clamp_int(cols, 1, ROGUE_MAX_VIEW_COLS);
}

static int
view_rows(void)
{
    int available;
    int rows;

    available = display_height() - ROGUE_STATUS_HEIGHT;
    if (available < ROGUE_TILE_DRAW_SIZE)
	available = ROGUE_TILE_DRAW_SIZE;
    rows = available / ROGUE_TILE_DRAW_SIZE;
    return clamp_int(rows, 1, ROGUE_MAX_VIEW_ROWS);
}

static int
camera_left(int cols)
{
    int left;
    int max_left;

    left = hero.x - cols / 2;
    max_left = NUMCOLS - cols;

    if (left < 0)
	left = 0;
    if (left > max_left)
	left = max_left;

    return left;
}

static int
camera_top(int rows)
{
    int top;
    int max_top;

    top = hero.y - rows / 2;
    max_top = ROGUE_MAX_VIEW_ROWS - rows;

    if (top < 0)
	top = 0;
    if (top > max_top)
	top = max_top;
    if (top < 0)
	top = 0;

    return top;
}

static void
toggle_fullscreen(void)
{
    bool fullscreen;

    if (display == NULL)
	return;

    fullscreen = !settings.fullscreen;
    if (fullscreen)
    {
	settings.windowed_width = display_width();
	settings.windowed_height = display_height();
    }

    if (al_set_display_flag(display, ALLEGRO_FULLSCREEN_WINDOW, fullscreen))
    {
	settings.fullscreen = fullscreen;
	if (!fullscreen)
	    al_resize_display(display, settings.windowed_width,
			      settings.windowed_height);
    }
}

static void
set_zoom(int tile_draw_size)
{
    settings.tile_draw_size = clamp_int(tile_draw_size,
					ROGUE_MIN_TILE_DRAW_SIZE,
					ROGUE_MAX_TILE_DRAW_SIZE);
}

static bool
handle_view_key(int keycode)
{
    switch (keycode)
    {
	case ALLEGRO_KEY_F11:
	    toggle_fullscreen();
	    return TRUE;
	case ALLEGRO_KEY_EQUALS:
	case ALLEGRO_KEY_PAD_PLUS:
	    set_zoom(settings.tile_draw_size + ROGUE_TILE_ZOOM_STEP);
	    return TRUE;
	case ALLEGRO_KEY_MINUS:
	case ALLEGRO_KEY_PAD_MINUS:
	    set_zoom(settings.tile_draw_size - ROGUE_TILE_ZOOM_STEP);
	    return TRUE;
	case ALLEGRO_KEY_0:
	case ALLEGRO_KEY_PAD_0:
	    set_zoom(ROGUE_DEFAULT_TILE_DRAW_SIZE);
	    return TRUE;
	default:
	    return FALSE;
    }
}

static bool
load_current_atlas(void)
{
    const char *atlas_path;
    char parent_atlas_path[512];
    ALLEGRO_BITMAP *loaded;

    atlas_path = rogue_tilepack_atlas_path();
    loaded = al_load_bitmap(atlas_path);
    if (loaded == NULL)
    {
	snprintf(parent_atlas_path, sizeof(parent_atlas_path), "../%s",
		 atlas_path);
	loaded = al_load_bitmap(parent_atlas_path);
    }
    if (loaded == NULL)
    {
	snprintf(parent_atlas_path, sizeof(parent_atlas_path),
		 "Could not load tile atlas: %s", atlas_path);
	allegro_start_error(parent_atlas_path);
	return FALSE;
    }

    if (atlas != NULL)
	al_destroy_bitmap(atlas);
    atlas = loaded;
    return TRUE;
}

static const char *
wall_thickness_name(void)
{
    switch (settings.wall_thickness)
    {
	case ROGUE_MIN_WALL_THICKNESS:
	    return "Thin";
	case ROGUE_MAX_WALL_THICKNESS:
	    return "Thick";
	default:
	    return "Medium";
    }
}

static void
cycle_wall_thickness(void)
{
    settings.wall_thickness++;
    if (settings.wall_thickness > ROGUE_MAX_WALL_THICKNESS)
	settings.wall_thickness = ROGUE_MIN_WALL_THICKNESS;
}

static void
show_tilepack_menu(void)
{
    ROGUE_TILEPACK_CHOICE choices[ROGUE_TILEPACK_MAX_CHOICES];
    char line[ROGUE_OVERLAY_LINE_LEN];
    char choice_map[ROGUE_TILEPACK_MAX_CHOICES + 1];
    int count;
    int i;
    char key;
    char selected;
    bool loaded_pack;

    count = rogue_tilepack_list(choices, ROGUE_TILEPACK_MAX_CHOICES);

    rogue_allegro_text_overlay_begin("Tile Set");
    snprintf(line, sizeof(line), "a) Glyph Mode%s",
	     settings.view_mode == ROGUE_ALLEGRO_VIEW_GLYPHS
	     ? " [current]" : "");
    rogue_allegro_text_overlay_add(line);
    choice_map[0] = '\0';

    if (count > 25)
	count = 25;

    for (i = 0; i < count && i + 1 < (int) sizeof(choice_map); i++)
    {
	key = (char) ('b' + i);
	choice_map[i] = key;
	snprintf(line, sizeof(line), "%c) %s%s", key, choices[i].label,
		 choices[i].current
		 && settings.view_mode == ROGUE_ALLEGRO_VIEW_TILES
		 ? " [current]" : "");
	rogue_allegro_text_overlay_add(line);
    }
    choice_map[i] = '\0';

    selected = rogue_allegro_text_overlay_pick(
	"Enter applies, Esc closes");
    rogue_allegro_text_overlay_clear();

    if (selected == 'a' || selected == 'A')
    {
	settings.view_mode = ROGUE_ALLEGRO_VIEW_GLYPHS;
	return;
    }

    for (i = 0; i < count && choice_map[i] != '\0'; i++)
    {
	if (tolower((unsigned char) selected)
	    != tolower((unsigned char) choice_map[i]))
	    continue;

	loaded_pack = rogue_tilepack_load_named(choices[i].id);
	if (load_current_atlas())
	{
	    settings.view_mode = ROGUE_ALLEGRO_VIEW_TILES;
	    if (!loaded_pack)
		rogue_allegro_notice("Tile Set",
				     "Could not load that tile set; using fallback.");
	}
	return;
    }
}

static void
show_settings_menu(void)
{
    char line[ROGUE_OVERLAY_LINE_LEN];
    char selected;
    bool done;

    done = FALSE;
    while (!done)
    {
	rogue_allegro_text_overlay_begin("Settings");
	snprintf(line, sizeof(line), "a) Side Panel Log: %s",
		 settings.side_panel_log_enabled ? "On" : "Off");
	rogue_allegro_text_overlay_add(line);
	snprintf(line, sizeof(line), "b) Stylized Log: %s",
		 settings.stylized_log_enabled ? "On" : "Off");
	rogue_allegro_text_overlay_add(line);
	snprintf(line, sizeof(line), "c) Stylized Bottom Bar: %s",
		 settings.stylized_bottom_bar_enabled ? "On" : "Off");
	rogue_allegro_text_overlay_add(line);
	snprintf(line, sizeof(line), "d) Blood Spatter: %s",
		 settings.blood_spatter_enabled ? "On" : "Off");
	rogue_allegro_text_overlay_add(line);
	snprintf(line, sizeof(line), "e) Shaders: %s",
		 settings.shader_enabled ? "On" : "Off");
	rogue_allegro_text_overlay_add(line);
	snprintf(line, sizeof(line), "f) Wall Thickness: %s",
		 wall_thickness_name());
	rogue_allegro_text_overlay_add(line);
	rogue_allegro_text_overlay_add("");
	rogue_allegro_text_overlay_add("Visual-only settings. Gameplay rules stay unchanged.");

	selected = rogue_allegro_text_overlay_pick(
	    "Enter changes, Esc closes");
	rogue_allegro_text_overlay_clear();

	switch (selected)
	{
	    case 'a':
	    case 'A':
		settings.side_panel_log_enabled =
		    !settings.side_panel_log_enabled;
		save_settings();
		break;
	    case 'b':
	    case 'B':
		settings.stylized_log_enabled =
		    !settings.stylized_log_enabled;
		save_settings();
		break;
	    case 'c':
	    case 'C':
		settings.stylized_bottom_bar_enabled =
		    !settings.stylized_bottom_bar_enabled;
		save_settings();
		break;
	    case 'd':
	    case 'D':
		settings.blood_spatter_enabled =
		    !settings.blood_spatter_enabled;
		save_settings();
		break;
	    case 'e':
	    case 'E':
		settings.shader_enabled = !settings.shader_enabled;
		save_settings();
		break;
	    case 'f':
	    case 'F':
		cycle_wall_thickness();
		save_settings();
		break;
	    default:
		done = TRUE;
		break;
	}
	rogue_allegro_render();
    }
}

static char
map_special_key(int keycode)
{
    switch (keycode)
    {
	case ALLEGRO_KEY_UP:
	    return 'k';
	case ALLEGRO_KEY_DOWN:
	    return 'j';
	case ALLEGRO_KEY_LEFT:
	    return 'h';
	case ALLEGRO_KEY_RIGHT:
	    return 'l';
	case ALLEGRO_KEY_HOME:
	    return 'y';
	case ALLEGRO_KEY_PGUP:
	    return 'u';
	case ALLEGRO_KEY_END:
	    return 'b';
	case ALLEGRO_KEY_PGDN:
	    return 'n';
	case ALLEGRO_KEY_PAD_8:
	    return 'k';
	case ALLEGRO_KEY_PAD_2:
	    return 'j';
	case ALLEGRO_KEY_PAD_4:
	    return 'h';
	case ALLEGRO_KEY_PAD_6:
	    return 'l';
	case ALLEGRO_KEY_PAD_7:
	    return 'y';
	case ALLEGRO_KEY_PAD_9:
	    return 'u';
	case ALLEGRO_KEY_PAD_1:
	    return 'b';
	case ALLEGRO_KEY_PAD_3:
	    return 'n';
	case ALLEGRO_KEY_PAD_5:
	    return '.';
	case ALLEGRO_KEY_ESCAPE:
	    return ESCAPE;
	case ALLEGRO_KEY_ENTER:
	case ALLEGRO_KEY_PAD_ENTER:
	    return '\n';
	case ALLEGRO_KEY_BACKSPACE:
	    return '\b';
	case ALLEGRO_KEY_F1:
	    return '?';
	case ALLEGRO_KEY_F2:
	    return '/';
	case ALLEGRO_KEY_F3:
	    return 'a';
	case ALLEGRO_KEY_F4:
	    return CTRL('R');
	case ALLEGRO_KEY_F5:
	    return 'c';
	case ALLEGRO_KEY_F6:
	    return 'D';
	case ALLEGRO_KEY_F7:
	    return 'i';
	case ALLEGRO_KEY_F8:
	    return '^';
	default:
	    return '\0';
    }
}

static void
drain_keyboard_events(void)
{
    ALLEGRO_EVENT event;

    while (al_peek_next_event(queue, &event))
    {
	if (event.type != ALLEGRO_EVENT_KEY_DOWN
	    && event.type != ALLEGRO_EVENT_KEY_UP
	    && event.type != ALLEGRO_EVENT_KEY_CHAR)
	    break;
	al_drop_next_event(queue);
    }
}

static void
draw_glyph_foreground_cell(int screen_x, int screen_y, ROGUE_TILE_CELL *cell)
{
    ALLEGRO_COLOR fg;
    int dx, dy;
    char text[2];

    if (cell == NULL || cell->glyph == ' ')
	return;

    dx = render_origin_x + screen_x * ROGUE_TILE_DRAW_SIZE;
    dy = render_origin_y + screen_y * ROGUE_TILE_DRAW_SIZE;

    fg = al_map_rgb(220, 220, 210);
    if (cell->layer == ROGUE_TILE_OBJECT)
	fg = al_map_rgb(240, 210, 80);
    else if (cell->layer == ROGUE_TILE_ACTOR)
	fg = al_map_rgb(120, 210, 255);
    else if (cell->glyph == '|' || cell->glyph == '-')
	fg = al_map_rgb(150, 150, 150);

    text[0] = cell->glyph;
    text[1] = '\0';
    al_draw_text(font, fg, dx + ROGUE_TILE_DRAW_SIZE / 2, dy + 1,
		 ALLEGRO_ALIGN_CENTRE, text);
}

static void
draw_glyph_cell(int screen_x, int screen_y, ROGUE_TILE_CELL *cell)
{
    ALLEGRO_COLOR bg;
    int dx, dy;

    dx = render_origin_x + screen_x * ROGUE_TILE_DRAW_SIZE;
    dy = render_origin_y + screen_y * ROGUE_TILE_DRAW_SIZE;

    bg = al_map_rgb(10, 10, 12);
    al_draw_filled_rectangle(dx, dy, dx + ROGUE_TILE_DRAW_SIZE,
			     dy + ROGUE_TILE_DRAW_SIZE, bg);
    draw_glyph_foreground_cell(screen_x, screen_y, cell);
}

static void
draw_atlas_tile(int atlas_index, int dx, int dy)
{
    int sx, sy;
    int source_w, source_h;
    int columns;

    columns = rogue_tilepack_columns();
    source_w = rogue_tilepack_source_width();
    source_h = rogue_tilepack_source_height();
    sx = (atlas_index % columns) * source_w;
    sy = (atlas_index / columns) * source_h;
    al_draw_scaled_bitmap(atlas, sx, sy, source_w, source_h, dx, dy,
			  ROGUE_TILE_DRAW_SIZE, ROGUE_TILE_DRAW_SIZE, 0);
}

static int
resolved_cell_index(const ROGUE_TILE_CELL *cell)
{
    if (cell == NULL)
	return -1;
    return rogue_tilepack_lookup_index(cell->role, cell->atlas_index);
}

static int
resolved_underlay_index(const ROGUE_TILE_CELL *cell)
{
    if (cell == NULL)
	return -1;
    return rogue_tilepack_lookup_index(cell->under_role, cell->under_atlas_index);
}

static bool
is_wall_cell(const ROGUE_TILE_CELL *cell)
{
    return (bool)(cell != NULL
		  && cell->layer == ROGUE_TILE_TERRAIN
		  && (cell->glyph == '|' || cell->glyph == '-'));
}

static bool
is_wall_edge_neighbor(const ROGUE_TILE_CELL *cell)
{
    if (cell == NULL || !cell->seen || cell->layer == ROGUE_TILE_EMPTY)
	return FALSE;

    return !is_wall_cell(cell);
}

static int
wall_thickness_from_size(int size)
{
    int edge;

    switch (settings.wall_thickness)
    {
	case ROGUE_MIN_WALL_THICKNESS:
	    edge = size / 10;
	    break;
	case ROGUE_MAX_WALL_THICKNESS:
	    edge = size / 4;
	    break;
	default:
	    edge = size / 6;
	    break;
    }

    if (edge < 2)
	edge = 2;
    if (edge > size)
	edge = size;
    return edge;
}

static int
wall_thickness_pixels(void)
{
    return wall_thickness_from_size(ROGUE_TILE_DRAW_SIZE);
}

static int
wall_thickness_source_pixels(int source_w, int source_h)
{
    int size;

    size = (source_w < source_h) ? source_w : source_h;
    return wall_thickness_from_size(size);
}

static void
draw_atlas_tile_region(int atlas_index, int sx_offset, int sy_offset,
		       int source_w, int source_h, int dx, int dy,
		       int dest_w, int dest_h)
{
    int sx, sy;
    int columns;

    columns = rogue_tilepack_columns();
    sx = (atlas_index % columns) * rogue_tilepack_source_width() + sx_offset;
    sy = (atlas_index / columns) * rogue_tilepack_source_height() + sy_offset;
    al_draw_scaled_bitmap(atlas, sx, sy, source_w, source_h, dx, dy,
			  dest_w, dest_h, 0);
}

static void
draw_wall_edge_cell(int screen_x, int screen_y, ROGUE_TILE_CELL *cell,
		    ROGUE_TILE_CELL view[ROGUE_MAX_VIEW_ROWS][ROGUE_MAX_VIEW_COLS],
		    int rows, int cols)
{
    int dx, dy;
    int edge;
    int source_edge;
    int source_w, source_h;
    int wall_index;
    bool open_left, open_right, open_up, open_down;

    dx = render_origin_x + screen_x * ROGUE_TILE_DRAW_SIZE;
    dy = render_origin_y + screen_y * ROGUE_TILE_DRAW_SIZE;
    source_w = rogue_tilepack_source_width();
    source_h = rogue_tilepack_source_height();
    edge = wall_thickness_pixels();
    source_edge = wall_thickness_source_pixels(source_w, source_h);

    open_left = (bool)(screen_x > 0
		       && is_wall_edge_neighbor(&view[screen_y][screen_x - 1]));
    open_right = (bool)(screen_x + 1 < cols
			&& is_wall_edge_neighbor(&view[screen_y][screen_x + 1]));
    open_up = (bool)(screen_y > 0
		     && is_wall_edge_neighbor(&view[screen_y - 1][screen_x]));
    open_down = (bool)(screen_y + 1 < rows
		       && is_wall_edge_neighbor(&view[screen_y + 1][screen_x]));

    al_draw_filled_rectangle(dx, dy, dx + ROGUE_TILE_DRAW_SIZE,
			     dy + ROGUE_TILE_DRAW_SIZE, al_map_rgb(0, 0, 0));

    wall_index = resolved_cell_index(cell);
    if (wall_index < 0 || atlas == NULL)
    {
	draw_glyph_cell(screen_x, screen_y, cell);
	return;
    }

    if (open_left)
	draw_atlas_tile_region(wall_index, 0, 0, source_edge,
			       source_h, dx, dy, edge,
			       ROGUE_TILE_DRAW_SIZE);
    if (open_right)
	draw_atlas_tile_region(wall_index,
			       source_w - source_edge, 0,
			       source_edge, source_h,
			       dx + ROGUE_TILE_DRAW_SIZE - edge, dy,
			       edge, ROGUE_TILE_DRAW_SIZE);
    if (open_up)
	draw_atlas_tile_region(wall_index, 0, 0,
			       source_w, source_edge,
			       dx, dy, ROGUE_TILE_DRAW_SIZE, edge);
    if (open_down)
	draw_atlas_tile_region(wall_index, 0,
			       source_h - source_edge,
			       source_w, source_edge,
			       dx, dy + ROGUE_TILE_DRAW_SIZE - edge,
			       ROGUE_TILE_DRAW_SIZE, edge);

}

static void
draw_tile_cell(int screen_x, int screen_y, ROGUE_TILE_CELL *cell)
{
    int dx, dy;
    int atlas_index;
    int underlay_index;
    ALLEGRO_COLOR fallback;

    dx = render_origin_x + screen_x * ROGUE_TILE_DRAW_SIZE;
    dy = render_origin_y + screen_y * ROGUE_TILE_DRAW_SIZE;

    atlas_index = resolved_cell_index(cell);
    underlay_index = resolved_underlay_index(cell);

    if (cell->has_underlay && underlay_index >= 0 && atlas != NULL)
	draw_atlas_tile(underlay_index, dx, dy);

    if (cell->layer == ROGUE_TILE_EMPTY)
    {
	al_draw_filled_rectangle(dx, dy, dx + ROGUE_TILE_DRAW_SIZE,
				 dy + ROGUE_TILE_DRAW_SIZE,
				 al_map_rgb(0, 0, 0));
	return;
    }

    if (atlas_index < 0 || atlas == NULL)
    {
	fallback = al_map_rgb(100, 20, 60);
	al_draw_filled_rectangle(dx, dy, dx + ROGUE_TILE_DRAW_SIZE,
				 dy + ROGUE_TILE_DRAW_SIZE, fallback);
	draw_glyph_cell(screen_x, screen_y, cell);
	return;
    }

    draw_atlas_tile(atlas_index, dx, dy);
}

static void
draw_actor_foreground_cell(int screen_x, int screen_y, ROGUE_TILE_CELL *cell)
{
    int dx, dy;
    int atlas_index;

    if (cell == NULL || cell->layer != ROGUE_TILE_ACTOR)
	return;

    if (settings.view_mode == ROGUE_ALLEGRO_VIEW_GLYPHS)
    {
	draw_glyph_foreground_cell(screen_x, screen_y, cell);
	return;
    }

    atlas_index = resolved_cell_index(cell);
    if (atlas_index < 0 || atlas == NULL)
    {
	draw_glyph_foreground_cell(screen_x, screen_y, cell);
	return;
    }

    dx = render_origin_x + screen_x * ROGUE_TILE_DRAW_SIZE;
    dy = render_origin_y + screen_y * ROGUE_TILE_DRAW_SIZE;
    draw_atlas_tile(atlas_index, dx, dy);
}

static int
status_piece_width(const char *label, const char *value)
{
    return al_get_text_width(font, label)
	   + al_get_text_width(font, value)
	   + al_get_text_width(font, "  ");
}

static int
draw_status_piece(const char *label, const char *value, int x, int y,
		  ALLEGRO_COLOR value_color)
{
    ALLEGRO_COLOR label_color;

    label_color = al_map_rgb(130, 150, 170);
    al_draw_text(font, label_color, x, y, 0, label);
    x += al_get_text_width(font, label);
    al_draw_text(font, value_color, x, y, 0, value);
    x += al_get_text_width(font, value);
    x += al_get_text_width(font, "  ");
    return x;
}

static void
draw_stylized_status_line(int width, int y, int armor,
			  const char *hungry_name, const char *fallback)
{
    char level_text[24];
    char gold_text[24];
    char hp_text[32];
    char str_text[24];
    char arm_text[24];
    char exp_text[32];
    int total_width;
    int x;
    ALLEGRO_COLOR hp_color;

    snprintf(level_text, sizeof(level_text), "%d", level);
    snprintf(gold_text, sizeof(gold_text), "%d", purse);
    snprintf(hp_text, sizeof(hp_text), "%d(%d)", pstats.s_hpt, max_hp);
    snprintf(str_text, sizeof(str_text), "%u", pstats.s_str);
    snprintf(arm_text, sizeof(arm_text), "%d", armor);
    snprintf(exp_text, sizeof(exp_text), "%d/%d", pstats.s_lvl, pstats.s_exp);

    total_width = status_piece_width("Level ", level_text)
		  + status_piece_width("Gold ", gold_text)
		  + status_piece_width("HP ", hp_text)
		  + status_piece_width("ST ", str_text)
		  + status_piece_width("Arm ", arm_text)
		  + status_piece_width("Exp ", exp_text);
    if (hungry_name != NULL && hungry_name[0] != '\0')
	total_width += status_piece_width("", hungry_name);

    if (total_width > width - 24)
    {
	al_draw_text(font, al_map_rgb(230, 230, 220), width / 2, y,
		     ALLEGRO_ALIGN_CENTRE, fallback);
	return;
    }

    hp_color = al_map_rgb(116, 220, 148);
    if (pstats.s_hpt * 4 <= max_hp)
	hp_color = al_map_rgb(238, 82, 82);
    else if (pstats.s_hpt * 2 <= max_hp)
	hp_color = al_map_rgb(238, 205, 112);

    x = (width - total_width) / 2;
    x = draw_status_piece("Level ", level_text, x, y,
			  al_map_rgb(126, 176, 238));
    x = draw_status_piece("Gold ", gold_text, x, y,
			  al_map_rgb(238, 205, 112));
    x = draw_status_piece("HP ", hp_text, x, y, hp_color);
    x = draw_status_piece("ST ", str_text, x, y,
			  al_map_rgb(228, 154, 83));
    x = draw_status_piece("Arm ", arm_text, x, y,
			  al_map_rgb(174, 190, 210));
    x = draw_status_piece("Exp ", exp_text, x, y,
			  al_map_rgb(174, 154, 238));
    if (hungry_name != NULL && hungry_name[0] != '\0')
	draw_status_piece("", hungry_name, x, y, al_map_rgb(228, 154, 83));
}

static void
draw_status(void)
{
    int y;
    int w;
    int h;
    int armor;
    char line[256];
    int line_height;
    int first_line_y;
    int second_line_y;
    static char *state_name[] = { "", "Hungry", "Weak", "Faint" };

    w = play_area_width();
    h = display_height();
    y = h - ROGUE_STATUS_HEIGHT;
    if (y < 0)
	y = 0;
    armor = (cur_armor != NULL ? cur_armor->o_arm : pstats.s_arm);
    line_height = al_get_font_line_height(font);
    first_line_y = y + 8;
    second_line_y = first_line_y + line_height + 6;

    al_draw_filled_rectangle(0, y, w, h,
			     al_map_rgb(5, 5, 8));
    al_draw_line(0, y, w, y, al_map_rgb(80, 80, 90), 1);

    snprintf(line, sizeof(line),
	     "Level:%d  Gold:%d  HP:%d(%d)  ST:%u  Arm:%d  Exp:%d/%d %s",
	     level, purse, pstats.s_hpt, max_hp, pstats.s_str, armor,
	     pstats.s_lvl, pstats.s_exp, state_name[hungry_state]);

    if (settings.stylized_bottom_bar_enabled)
	draw_stylized_status_line(w, first_line_y, armor,
				  state_name[hungry_state], line);
    else
	al_draw_text(font, al_map_rgb(230, 230, 220), w / 2,
		     first_line_y, ALLEGRO_ALIGN_CENTRE, line);
    if (!settings.side_panel_log_enabled)
	al_draw_text(font, al_map_rgb(180, 200, 255), 8, second_line_y,
		     0, huh);
    if (prompt_active)
	al_draw_text(font, al_map_rgb(245, 226, 170), w - 8,
		     second_line_y, ALLEGRO_ALIGN_RIGHT, prompt_text);
    else
	al_draw_text(font, al_map_rgb(160, 160, 160), w - 8,
		     second_line_y, ALLEGRO_ALIGN_RIGHT,
		     "F10 tiles  F11 full  F12 settings");
}

static int
ascii_lower_char(int ch)
{
    if (ch >= 'A' && ch <= 'Z')
	return ch + ('a' - 'A');
    return ch;
}

static bool
contains_text(const char *text, const char *needle)
{
    const char *p;
    const char *n;

    if (text == NULL || needle == NULL || *needle == '\0')
	return FALSE;

    for (p = text; *p != '\0'; p++)
    {
	n = needle;
	while (*n != '\0'
	       && p[n - needle] != '\0'
	       && ascii_lower_char((unsigned char) p[n - needle])
		  == ascii_lower_char((unsigned char) *n))
	    n++;
	if (*n == '\0')
	    return TRUE;
    }

    return FALSE;
}

static int
blood_random(int limit)
{
    if (limit <= 0)
	return 0;

    blood_rng = blood_rng * 1664525u + 1013904223u;
    return (int) ((blood_rng >> 16) % (unsigned int) limit);
}

static bool
message_is_blood_trigger(const char *message)
{
    if (message == NULL)
	return FALSE;

    if (contains_text(message, "miss")
	|| contains_text(message, "can't")
	|| contains_text(message, "cannot")
	|| contains_text(message, "no damage"))
	return FALSE;

    return contains_text(message, "hit")
	|| contains_text(message, "defeated")
	|| contains_text(message, "killed")
	|| contains_text(message, "wounded");
}

static bool
blood_tile_is_walkable(int y, int x)
{
    char ch;

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

static bool
find_blood_splat_cell(int *out_y, int *out_x)
{
    int attempt;
    int radius;
    int y;
    int x;

    for (attempt = 0; attempt < 16; attempt++)
    {
	radius = (attempt < 8) ? 1 : 2;
	y = hero.y + blood_random(radius * 2 + 1) - radius;
	x = hero.x + blood_random(radius * 2 + 1) - radius;
	if (!blood_tile_is_walkable(y, x))
	    continue;

	*out_y = y;
	*out_x = x;
	return TRUE;
    }

    if (blood_tile_is_walkable(hero.y, hero.x))
    {
	*out_y = hero.y;
	*out_x = hero.x;
	return TRUE;
    }

    return FALSE;
}

static void
clear_blood_splats(void)
{
    blood_splat_count = 0;
}

static void
add_blood_splat(int y, int x)
{
    ROGUE_BLOOD_SPLAT *splat;
    int slot;

    if (!blood_tile_is_walkable(y, x))
	return;

    if (blood_splat_count < ROGUE_BLOOD_SPLATS)
	slot = blood_splat_count++;
    else
	slot = blood_random(ROGUE_BLOOD_SPLATS);

    splat = &blood_splats[slot];
    splat->x = x;
    splat->y = y;
    splat->level = level;
    splat->radius = 2 + blood_random(4);
    splat->alpha = 130 + blood_random(86);
    splat->offset_x = 8 + blood_random(17);
    splat->offset_y = 8 + blood_random(17);
}

static void
spawn_blood_spatter(void)
{
    int i;
    int y;
    int x;

    for (i = 0; i < ROGUE_BLOOD_DROPS_PER_HIT; i++)
	if (find_blood_splat_cell(&y, &x))
	    add_blood_splat(y, x);
}

static void
draw_blood_splats(int left, int top, int rows, int cols)
{
    int i;
    int screen_x;
    int screen_y;
    int px;
    int py;
    int radius;
    ALLEGRO_COLOR color;
    ROGUE_BLOOD_SPLAT *splat;

    if (!settings.blood_spatter_enabled || blood_splat_count <= 0)
	return;

    for (i = 0; i < blood_splat_count; i++)
    {
	splat = &blood_splats[i];
	if (splat->level != level)
	    continue;

	screen_x = splat->x - left;
	screen_y = splat->y - top;
	if (screen_x < 0 || screen_x >= cols
	    || screen_y < 0 || screen_y >= rows)
	    continue;

	px = render_origin_x + screen_x * ROGUE_TILE_DRAW_SIZE
	     + (splat->offset_x * ROGUE_TILE_DRAW_SIZE) / 32;
	py = render_origin_y + screen_y * ROGUE_TILE_DRAW_SIZE
	     + (splat->offset_y * ROGUE_TILE_DRAW_SIZE) / 32;
	radius = (splat->radius * ROGUE_TILE_DRAW_SIZE) / 32;
	if (radius < 1)
	    radius = 1;
	color = al_map_rgba(118, 11, 18, splat->alpha);
	al_draw_filled_circle(px, py, radius, color);
	if (radius > 2)
	    al_draw_filled_circle(px + radius, py - radius / 2,
				  radius / 2, al_map_rgba(70, 4, 9,
							   splat->alpha));
    }
}

static ALLEGRO_COLOR
log_message_color(const char *message)
{
    if (!settings.stylized_log_enabled)
	return al_map_rgb(210, 220, 230);

    if (contains_text(message, "defeated")
	|| contains_text(message, "killed")
	|| contains_text(message, "hit"))
	return al_map_rgb(238, 122, 102);
    if (contains_text(message, "miss")
	|| contains_text(message, "nothing")
	|| contains_text(message, "illegal"))
	return al_map_rgb(150, 156, 166);
    if (contains_text(message, "gold")
	|| contains_text(message, "found")
	|| contains_text(message, "pick"))
	return al_map_rgb(238, 205, 112);
    if (contains_text(message, "hungry")
	|| contains_text(message, "weak")
	|| contains_text(message, "faint"))
	return al_map_rgb(228, 154, 83);
    if (contains_text(message, "welcome")
	|| contains_text(message, "level"))
	return al_map_rgb(126, 176, 238);

    return al_map_rgb(210, 220, 230);
}

static int
next_wrap_len(const char *text, int start, int max_chars)
{
    int len;
    int end;
    int split;
    int i;

    len = (int) strlen(text);
    while (start < len && isspace((unsigned char) text[start]))
	start++;
    if (start >= len)
	return 0;

    end = start + max_chars;
    if (end > len)
	end = len;

    for (i = start; i < end; i++)
	if (text[i] == '.')
	    return i - start + 1;

    if (end >= len)
	return len - start;

    split = end;
    while (split > start && !isspace((unsigned char) text[split]))
	split--;
    if (split <= start)
	split = end;

    return split - start;
}

static int
count_wrapped_lines(const char *text, int max_chars)
{
    int lines;
    int start;
    int len;
    int text_len;

    if (text == NULL || *text == '\0')
	return 1;

    lines = 0;
    start = 0;
    text_len = (int) strlen(text);
    while (start < text_len)
    {
	while (start < text_len && isspace((unsigned char) text[start]))
	    start++;
	if (start >= text_len)
	    break;
	len = next_wrap_len(text, start, max_chars);
	if (len <= 0)
	    break;
	lines++;
	start += len;
    }

    return lines > 0 ? lines : 1;
}

static int
count_log_entry_lines(int index, int max_chars)
{
    int lines;

    lines = count_wrapped_lines(message_log[index], max_chars);
    if (message_damage_dealt[index] > 0)
	lines++;
    if (message_damage_taken[index] > 0)
	lines++;
    if (message_enemy_max_hp[index] > 0)
	lines++;

    return lines;
}

static void
draw_wrapped_log_message(const char *message, int x, int *y,
			 int max_chars, int line_height,
			 ALLEGRO_COLOR text)
{
    int start;
    int len;
    int text_len;
    char line[ROGUE_OVERLAY_LINE_LEN];

    if (message == NULL || *message == '\0')
	return;

    start = 0;
    text_len = (int) strlen(message);
    while (start < text_len)
    {
	while (start < text_len && isspace((unsigned char) message[start]))
	    start++;
	if (start >= text_len)
	    break;
	len = next_wrap_len(message, start, max_chars);
	if (len <= 0)
	    break;
	if (len >= (int) sizeof(line))
	    len = (int) sizeof(line) - 1;
	memcpy(line, message + start, (size_t) len);
	line[len] = '\0';
	al_draw_text(font, text, x, *y, 0, line);
	*y += line_height + 1;
	start += len;
    }
}

static void
draw_damage_dealt_line(int index, int x, int *y, int line_height)
{
    char line[64];

    if (message_damage_dealt[index] > 0)
    {
	snprintf(line, sizeof(line), "Damage dealt: %d",
		 message_damage_dealt[index]);
	al_draw_text(font, al_map_rgb(238, 122, 102), x, *y, 0, line);
	*y += line_height + 1;
    }
}

static void
draw_damage_taken_line(int index, int x, int *y, int line_height)
{
    char line[64];

    if (message_damage_taken[index] > 0)
    {
	snprintf(line, sizeof(line), "Damage taken: %d",
		 message_damage_taken[index]);
	al_draw_text(font, al_map_rgb(238, 82, 82), x, *y, 0, line);
	*y += line_height + 1;
    }
}

static void
draw_enemy_hp_line(int index, int x, int *y, int line_height)
{
    char line[64];

    if (message_enemy_max_hp[index] > 0)
    {
	snprintf(line, sizeof(line), "Enemy HP: %d/%d",
		 message_enemy_hp[index], message_enemy_max_hp[index]);
	al_draw_text(font, al_map_rgb(214, 190, 124), x, *y, 0, line);
	*y += line_height + 1;
    }
}

static void
draw_log_damage_lines(int index, int x, int *y, int line_height)
{
    draw_damage_dealt_line(index, x, y, line_height);
    draw_enemy_hp_line(index, x, y, line_height);
    draw_damage_taken_line(index, x, y, line_height);
}

static void
draw_wrapped_log_segment(const char *message, int start, int len,
			 int x, int *y, int max_chars, int line_height,
			 ALLEGRO_COLOR text)
{
    char segment[ROGUE_OVERLAY_LINE_LEN];

    if (len <= 0)
	return;
    if (len >= (int) sizeof(segment))
	len = (int) sizeof(segment) - 1;
    memcpy(segment, message + start, (size_t) len);
    segment[len] = '\0';
    draw_wrapped_log_message(segment, x, y, max_chars, line_height, text);
}

static void
draw_combat_log_entry(int index, int x, int *y, int max_chars,
		      int line_height)
{
    const char *message;
    const char *split;
    int first_len;
    int second_start;
    int message_len;
    ALLEGRO_COLOR color;

    message = message_log[index];
    color = log_message_color(message);
    if (message_damage_dealt[index] <= 0 || message_damage_taken[index] <= 0)
    {
	draw_wrapped_log_message(message, x, y, max_chars, line_height, color);
	draw_log_damage_lines(index, x, y, line_height);
	return;
    }

    split = strchr(message, '.');
    if (split == NULL || split[1] == '\0')
    {
	draw_wrapped_log_message(message, x, y, max_chars, line_height, color);
	draw_log_damage_lines(index, x, y, line_height);
	return;
    }

    first_len = (int) (split - message) + 1;
    draw_wrapped_log_segment(message, 0, first_len, x, y, max_chars,
			     line_height, color);
    draw_damage_dealt_line(index, x, y, line_height);
    draw_enemy_hp_line(index, x, y, line_height);

    second_start = first_len;
    message_len = (int) strlen(message);
    while (second_start < message_len
	   && isspace((unsigned char) message[second_start]))
	second_start++;
    draw_wrapped_log_segment(message, second_start,
			     message_len - second_start, x, y, max_chars,
			     line_height, color);
    draw_damage_taken_line(index, x, y, line_height);
}

static void
draw_side_panel(void)
{
    int panel_w;
    int x;
    int y;
    int i;
    int first;
    int line_height;
    int max_lines;
    int char_width;
    int max_chars;
    int max_text_y;
    int entry_lines;
    int used_lines;
    ALLEGRO_COLOR bg, border, title, text, muted, divider;

    panel_w = side_panel_width();
    if (panel_w <= 0)
	return;

    x = display_width() - panel_w;
    bg = al_map_rgb(8, 9, 13);
    border = al_map_rgb(60, 64, 82);
    title = al_map_rgb(245, 226, 170);
    text = al_map_rgb(210, 220, 230);
    muted = al_map_rgb(130, 135, 150);
    divider = al_map_rgb(36, 40, 54);
    line_height = al_get_font_line_height(font);
    char_width = al_get_text_width(font, "M");
    if (char_width <= 0)
	char_width = 12;
    max_chars = (panel_w - 36) / char_width;
    if (max_chars < 8)
	max_chars = 8;
    max_lines = (display_height() - 96) / (line_height + 2);
    if (max_lines < 1)
	max_lines = 1;

    al_draw_filled_rectangle(x, 0, display_width(), display_height(), bg);
    al_draw_line(x, 0, x, display_height(), border, 1);
    al_draw_text(font, title, x + 18, 18, 0, "Log");
    al_draw_line(x + 16, 56, display_width() - 16, 56, border, 1);

    used_lines = 0;
    first = message_log_count;
    for (i = message_log_count - 1; i >= 0; i--)
    {
	entry_lines = count_log_entry_lines(i, max_chars);
	if (used_lines > 0)
	    entry_lines++;
	if (used_lines + entry_lines > max_lines)
	    break;
	used_lines += entry_lines;
	first = i;
    }
    if (first < 0)
	first = 0;
    if (first > message_log_count)
	first = message_log_count;

    y = 72;
    max_text_y = display_height() - 16;
    for (i = first; i < message_log_count; i++)
    {
	if (i > first)
	{
	    al_draw_line(x + 18, y + 2, display_width() - 18, y + 2,
			 divider, 1);
	    y += 9;
	}
	draw_combat_log_entry(i, x + 18, &y, max_chars, line_height);
	y += 4;
	if (y > max_text_y)
	    break;
    }

    if (message_log_count == 0)
	al_draw_text(font, muted, x + 18, y, 0, "No messages yet");
}

static void
draw_text_overlay(void)
{
    int x, y, w, h;
    int window_w, window_h;
    int i;
    int line_index;
    int line_height;
    int visible_lines;
    int text_y;
    char more_text[64];
    ALLEGRO_COLOR bg, border, title, text, muted, selected_bg;

    if (!text_overlay_active)
	return;

    window_w = display_width();
    window_h = display_height();
    line_height = al_get_font_line_height(font);
    visible_lines = text_overlay_line_count;
    if (visible_lines > 14)
	visible_lines = 14;
    if (visible_lines < 1)
	visible_lines = 1;
    if (text_overlay_selectable)
    {
	text_overlay_selected = rogue_picker_clamp_selection(
	    text_overlay_selected, text_overlay_line_count);
	if (text_overlay_selected >= 0)
	{
	    if (text_overlay_selected < text_overlay_scroll)
		text_overlay_scroll = text_overlay_selected;
	    if (text_overlay_selected >= text_overlay_scroll + visible_lines)
		text_overlay_scroll = text_overlay_selected - visible_lines + 1;
	}
	if (text_overlay_scroll < 0)
	    text_overlay_scroll = 0;
	if (text_overlay_scroll > text_overlay_line_count - visible_lines)
	    text_overlay_scroll = text_overlay_line_count - visible_lines;
	if (text_overlay_scroll < 0)
	    text_overlay_scroll = 0;
    }
    else
    {
	if (text_overlay_scroll < 0)
	    text_overlay_scroll = 0;
	if (text_overlay_scroll > text_overlay_line_count - visible_lines)
	    text_overlay_scroll = text_overlay_line_count - visible_lines;
	if (text_overlay_scroll < 0)
	    text_overlay_scroll = 0;
    }

    w = window_w - 160;
    if (w < 520)
	w = window_w - 48;
    if (w > window_w - 48)
	w = window_w - 48;
    h = 96 + visible_lines * (line_height + 2);
    x = (window_w - w) / 2;
    y = (window_h - h) / 2;

    bg = al_map_rgba(8, 9, 13, 244);
    border = al_map_rgb(120, 126, 150);
    title = al_map_rgb(245, 226, 170);
    text = al_map_rgb(230, 230, 220);
    muted = al_map_rgb(170, 170, 165);
    selected_bg = al_map_rgb(54, 58, 76);

    al_draw_filled_rectangle(x, y, x + w, y + h, bg);
    al_draw_rectangle(x + 0.5, y + 0.5, x + w - 0.5, y + h - 0.5,
		      border, 2);
    al_draw_text(font, title, x + 24, y + 18, 0, text_overlay_title);
    al_draw_line(x + 24, y + 58, x + w - 24, y + 58,
		 al_map_rgb(70, 72, 88), 1);

    text_y = y + 72;
    for (i = 0; i < visible_lines; i++)
    {
	line_index = text_overlay_scroll + i;
	if (line_index >= text_overlay_line_count)
	    break;
	if (text_overlay_selectable && line_index == text_overlay_selected)
	    al_draw_filled_rectangle(x + 16, text_y - 2, x + w - 16,
				     text_y + line_height + 1, selected_bg);
	al_draw_text(font, text, x + 24, text_y, 0,
		     text_overlay_lines[line_index]);
	text_y += line_height + 2;
    }
    if (text_overlay_line_count > visible_lines)
    {
	snprintf(more_text, sizeof(more_text), "%d-%d of %d",
		 text_overlay_scroll + 1, text_overlay_scroll + visible_lines,
		 text_overlay_line_count);
	al_draw_text(font, muted, x + 24, text_y, 0, more_text);
    }

    al_draw_text(font, muted, x + w - 24, y + h - line_height - 16,
		 ALLEGRO_ALIGN_RIGHT, text_overlay_prompt);
}

static void
draw_death_overlay(void)
{
    int x, y, w, h;
    int window_w, window_h;
    int line_height;
    int text_y;
    ALLEGRO_COLOR bg, border, title, text, muted;

    if (!death_overlay_active)
	return;

    window_w = display_width();
    window_h = display_height();
    w = 560;
    if (w > window_w - 48)
	w = window_w - 48;
    line_height = al_get_font_line_height(font);
    h = 260;
    x = (window_w - w) / 2;
    y = (window_h - h) / 2;

    bg = al_map_rgb(12, 10, 10);
    border = al_map_rgb(150, 36, 36);
    title = al_map_rgb(245, 226, 170);
    text = al_map_rgb(230, 230, 220);
    muted = al_map_rgb(170, 170, 165);

    al_draw_filled_rectangle(x, y, x + w, y + h, bg);
    al_draw_rectangle(x + 0.5, y + 0.5, x + w - 0.5, y + h - 0.5,
		      border, 2);
    al_draw_line(x + 24, y + 72, x + w - 24, y + 72,
		 al_map_rgb(80, 60, 55), 1);

    al_draw_text(font, title, x + w / 2, y + 24,
		 ALLEGRO_ALIGN_CENTRE, death_title);
    text_y = y + 88;
    al_draw_text(font, text, x + w / 2, text_y,
		 ALLEGRO_ALIGN_CENTRE, whoami);
    text_y += line_height + 4;
    al_draw_text(font, text, x + w / 2, text_y,
		 ALLEGRO_ALIGN_CENTRE, death_killer);
    text_y += line_height + 4;
    al_draw_text(font, muted, x + w / 2, text_y,
		 ALLEGRO_ALIGN_CENTRE, death_gold);
    al_draw_text(font, title, x + w / 2, y + h - line_height - 24,
		 ALLEGRO_ALIGN_CENTRE, death_prompt);
}

bool
rogue_allegro_start(bool smoke)
{
    smoke_mode = smoke;

    if (started)
	return TRUE;

    init_settings_path();
    load_settings();

    if (!al_init())
    {
	allegro_start_error("Allegro initialization failed.");
	return FALSE;
    }

    if (!al_install_keyboard())
    {
	allegro_start_error("Allegro keyboard initialization failed.");
	return FALSE;
    }

    al_init_image_addon();
    al_init_font_addon();
    if (!al_init_ttf_addon())
    {
	allegro_start_error("Allegro TTF initialization failed.");
	return FALSE;
    }
    al_init_primitives_addon();

    al_set_new_display_option(ALLEGRO_VSYNC, 1, ALLEGRO_SUGGEST);
    al_set_new_display_flags(ALLEGRO_RESIZABLE);
    display = al_create_display(settings.windowed_width,
				settings.windowed_height);
    if (display == NULL)
    {
	allegro_start_error("Could not create Allegro display.");
	return FALSE;
    }
    al_set_window_title(display, "RogueTiles");

    queue = al_create_event_queue();
    if (queue == NULL)
    {
	allegro_start_error("Could not create Allegro event queue.");
	return FALSE;
    }

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());

    rogue_tilepack_load();
    if (!load_current_atlas())
	return FALSE;

    font = load_ui_font();
    if (font == NULL)
    {
	allegro_start_error("Could not create Allegro font.");
	return FALSE;
    }

    started = TRUE;
    return TRUE;
}

void
rogue_allegro_render(void)
{
    int screen_y, screen_x;
    int world_x;
    int world_y;
    int left;
    int top;
    int rows;
    int cols;
    ROGUE_TILE_CELL view[ROGUE_MAX_VIEW_ROWS][ROGUE_MAX_VIEW_COLS];
    ROGUE_TILE_CELL *cell;

    if (!started)
	return;

    rows = view_rows();
    cols = view_cols();
    left = camera_left(cols);
    top = camera_top(rows);
    render_origin_x = (play_area_width() - cols * ROGUE_TILE_DRAW_SIZE) / 2;
    if (render_origin_x < 0)
	render_origin_x = 0;
    render_origin_y = ((display_height() - ROGUE_STATUS_HEIGHT)
		       - rows * ROGUE_TILE_DRAW_SIZE) / 2;
    if (render_origin_y < 0)
	render_origin_y = 0;
    for (screen_y = 0; screen_y < rows; screen_y++)
	for (screen_x = 0; screen_x < cols; screen_x++)
	{
	    world_x = left + screen_x;
	    world_y = top + screen_y;
	    rogue_tile_describe_cell(world_y, world_x,
				     &view[screen_y][screen_x]);
	}

    al_clear_to_color(al_map_rgb(0, 0, 0));
    al_hold_bitmap_drawing(TRUE);
    for (screen_y = 0; screen_y < rows; screen_y++)
	for (screen_x = 0; screen_x < cols; screen_x++)
	{
	    cell = &view[screen_y][screen_x];
	    if (settings.view_mode == ROGUE_ALLEGRO_VIEW_TILES)
	    {
		if (is_wall_cell(cell))
		    draw_wall_edge_cell(screen_x, screen_y, cell, view,
					rows, cols);
		else
		    draw_tile_cell(screen_x, screen_y, cell);
	    }
	    else
		draw_glyph_cell(screen_x, screen_y, cell);
	}
    al_hold_bitmap_drawing(FALSE);

    draw_blood_splats(left, top, rows, cols);
    for (screen_y = 0; screen_y < rows; screen_y++)
	for (screen_x = 0; screen_x < cols; screen_x++)
	    draw_actor_foreground_cell(screen_x, screen_y,
				       &view[screen_y][screen_x]);
    draw_status();
    draw_side_panel();
    draw_text_overlay();
    draw_death_overlay();
    al_flip_display();

    if (smoke_mode)
	al_rest(0.2);
}

char
rogue_allegro_readchar(void)
{
    ALLEGRO_EVENT event;
    char mapped;
    double now;
    double timeout;
    bool got_event;

    if (!started)
	return (char) md_readchar();

    for (;;)
    {
	timeout = -1.0;
	if (held_movement != '\0')
	{
	    if (!movement_key_is_down(held_movement_keycode))
		clear_held_movement();
	    else
	    {
		now = al_get_time();
		if (now >= held_movement_next_time)
		{
		    held_movement_next_time = now + ROGUE_REPEAT_RATE_SECONDS;
		    return held_movement;
		}
		timeout = held_movement_next_time - now;
	    }
	}

	rogue_allegro_render();
	if (timeout >= 0.0)
	    got_event = (bool) al_wait_for_event_timed(queue, &event,
						      (float) timeout);
	else
	{
	    al_wait_for_event(queue, &event);
	    got_event = TRUE;
	}

	if (!got_event)
	    continue;

	if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
	    return 'Q';
	if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE)
	{
	    al_acknowledge_resize(display);
	    rogue_allegro_render();
	    continue;
	}

	if (event.type == ALLEGRO_EVENT_KEY_DOWN)
	{
	    if (event.keyboard.keycode == ALLEGRO_KEY_F12)
	    {
		show_settings_menu();
		suppress_key_char_keycode = event.keyboard.keycode;
		rogue_allegro_render();
		continue;
	    }
	    if (event.keyboard.keycode == ALLEGRO_KEY_F10)
	    {
		show_tilepack_menu();
		suppress_key_char_keycode = event.keyboard.keycode;
		rogue_allegro_render();
		continue;
	    }
	    if (handle_view_key(event.keyboard.keycode))
	    {
		suppress_key_char_keycode = event.keyboard.keycode;
		rogue_allegro_render();
		continue;
	    }

	    mapped = map_special_key(event.keyboard.keycode);
	    if (mapped != '\0')
	    {
		suppress_key_char_keycode = event.keyboard.keycode;
		if (is_movement_command(mapped))
		{
		    if (event.keyboard.repeat)
			continue;
		    begin_held_movement(event.keyboard.keycode, mapped);
		}
		return mapped;
	    }
	}

	if (event.type == ALLEGRO_EVENT_KEY_UP)
	{
	    if (event.keyboard.keycode == held_movement_keycode)
		clear_held_movement();
	    continue;
	}

	if (event.type == ALLEGRO_EVENT_KEY_CHAR)
	{
	    if (event.keyboard.keycode == held_movement_keycode
		&& !movement_key_is_down(event.keyboard.keycode))
	    {
		clear_held_movement();
		continue;
	    }
	    if (event.keyboard.keycode == suppress_key_char_keycode
		|| event.keyboard.keycode == held_movement_keycode)
	    {
		suppress_key_char_keycode = 0;
		continue;
	    }
	    if (event.keyboard.unichar > 0 && event.keyboard.unichar < 128)
	    {
		mapped = (char) event.keyboard.unichar;
		if (is_movement_command(mapped))
		{
		    if (event.keyboard.repeat)
			continue;
		    begin_held_movement(event.keyboard.keycode, mapped);
		}
		return mapped;
	    }
	}
    }
}

void
rogue_allegro_show_prompt(const char *prompt)
{
    if (prompt == NULL || *prompt == '\0')
	prompt = "Press Space to continue";

    snprintf(prompt_text, sizeof(prompt_text), "%s", prompt);
    prompt_active = TRUE;
    rogue_allegro_render();
}

void
rogue_allegro_clear_prompt(void)
{
    prompt_active = FALSE;
    prompt_text[0] = '\0';
    rogue_allegro_render();
}

void
rogue_allegro_record_message(const char *message)
{
    int i;

    if (message == NULL || *message == '\0')
	return;

    if (message_log_count >= ROGUE_MESSAGE_LOG_LINES)
    {
	for (i = 1; i < ROGUE_MESSAGE_LOG_LINES; i++)
	{
	    snprintf(message_log[i - 1], sizeof(message_log[i - 1]), "%s",
		     message_log[i]);
	    message_damage_dealt[i - 1] = message_damage_dealt[i];
	    message_damage_taken[i - 1] = message_damage_taken[i];
	    message_enemy_hp[i - 1] = message_enemy_hp[i];
	    message_enemy_max_hp[i - 1] = message_enemy_max_hp[i];
	}
	message_log_count = ROGUE_MESSAGE_LOG_LINES - 1;
    }

    snprintf(message_log[message_log_count],
	     sizeof(message_log[message_log_count]), "%s", message);
    message_damage_dealt[message_log_count] = pending_damage_dealt;
    message_damage_taken[message_log_count] = pending_damage_taken;
    message_enemy_hp[message_log_count] = pending_enemy_hp;
    message_enemy_max_hp[message_log_count] = pending_enemy_max_hp;
    message_log_count++;
    pending_damage_dealt = 0;
    pending_damage_taken = 0;
    pending_enemy_hp = 0;
    pending_enemy_max_hp = 0;

    if (contains_text(message, "welcome to level"))
	clear_blood_splats();
    else if (settings.blood_spatter_enabled
	     && message_is_blood_trigger(message))
	spawn_blood_spatter();

    rogue_allegro_render();
}

void
rogue_allegro_record_damage(int dealt, int taken, int enemy_hp,
			    int enemy_max_hp)
{
    if (dealt > 0)
	pending_damage_dealt += dealt;
    if (taken > 0)
	pending_damage_taken += taken;
    if (enemy_max_hp > 0)
    {
	pending_enemy_hp = clamp_int(enemy_hp, 0, enemy_max_hp);
	pending_enemy_max_hp = enemy_max_hp;
    }
}

void
rogue_allegro_text_overlay_begin(const char *title)
{
    if (title == NULL || *title == '\0')
	title = "Rogue";

    snprintf(text_overlay_title, sizeof(text_overlay_title), "%s", title);
    snprintf(text_overlay_prompt, sizeof(text_overlay_prompt),
	     "Space or Esc closes");
    text_overlay_line_count = 0;
    text_overlay_active = FALSE;
    text_overlay_selected = -1;
    text_overlay_scroll = 0;
    text_overlay_selectable = FALSE;
}

void
rogue_allegro_text_overlay_add(const char *line)
{
    if (line == NULL)
	line = "";
    if (text_overlay_line_count >= ROGUE_OVERLAY_MAX_LINES)
	return;

    snprintf(text_overlay_lines[text_overlay_line_count],
	     sizeof(text_overlay_lines[text_overlay_line_count]), "%s", line);
    text_overlay_line_count++;
}

char
rogue_allegro_text_overlay_show(const char *prompt)
{
    ALLEGRO_EVENT event;
    char ch;
    int visible_lines;
    int page;

    if (prompt != NULL && *prompt != '\0')
	snprintf(text_overlay_prompt, sizeof(text_overlay_prompt), "%s", prompt);
    text_overlay_selectable = FALSE;
    text_overlay_selected = -1;
    text_overlay_scroll = 0;
    text_overlay_active = TRUE;
    rogue_allegro_render();

    ch = '\0';
    while (ch == '\0')
    {
	visible_lines = text_overlay_line_count;
	if (visible_lines > 14)
	    visible_lines = 14;
	if (visible_lines < 1)
	    visible_lines = 1;
	page = visible_lines - 1;
	if (page < 1)
	    page = 1;

	al_wait_for_event(queue, &event);
	if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
	{
	    ch = 'Q';
	    break;
	}
	if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE)
	{
	    al_acknowledge_resize(display);
	    rogue_allegro_render();
	    continue;
	}
	if (event.type != ALLEGRO_EVENT_KEY_DOWN)
	    continue;

	switch (event.keyboard.keycode)
	{
	    default:
		if (handle_view_key(event.keyboard.keycode))
		    suppress_key_char_keycode = event.keyboard.keycode;
		break;
	    case ALLEGRO_KEY_UP:
	    case ALLEGRO_KEY_PAD_8:
		text_overlay_scroll--;
		break;
	    case ALLEGRO_KEY_DOWN:
	    case ALLEGRO_KEY_PAD_2:
		text_overlay_scroll++;
		break;
	    case ALLEGRO_KEY_PGUP:
	    case ALLEGRO_KEY_PAD_9:
		text_overlay_scroll -= page;
		break;
	    case ALLEGRO_KEY_PGDN:
	    case ALLEGRO_KEY_PAD_3:
		text_overlay_scroll += page;
		break;
	    case ALLEGRO_KEY_HOME:
		text_overlay_scroll = 0;
		break;
	    case ALLEGRO_KEY_END:
		text_overlay_scroll = text_overlay_line_count - visible_lines;
		break;
	    case ALLEGRO_KEY_SPACE:
		ch = ' ';
		break;
	    case ALLEGRO_KEY_ESCAPE:
		ch = ESCAPE;
		break;
	    case ALLEGRO_KEY_ENTER:
	    case ALLEGRO_KEY_PAD_ENTER:
		ch = '\n';
		break;
	}
	rogue_allegro_render();
    }

    text_overlay_active = FALSE;
    drain_keyboard_events();
    rogue_allegro_render();
    return ch;
}

char
rogue_allegro_text_overlay_pick(const char *prompt)
{
    ALLEGRO_EVENT event;
    const char *lines[ROGUE_OVERLAY_MAX_LINES];
    int i;
    int match;
    char chosen;
    char selected_key;
    char typed;

    if (!started)
	return '\0';

    if (prompt != NULL && *prompt != '\0')
	snprintf(text_overlay_prompt, sizeof(text_overlay_prompt), "%s", prompt);

    for (i = 0; i < text_overlay_line_count; i++)
	lines[i] = text_overlay_lines[i];

    clear_held_movement();
    suppress_key_char_keycode = 0;
    text_overlay_selectable = TRUE;
    text_overlay_selected = rogue_picker_first_keyed_line(
	lines, text_overlay_line_count);
    if (text_overlay_selected < 0)
	text_overlay_selected = rogue_picker_clamp_selection(
	    0, text_overlay_line_count);
    text_overlay_scroll = 0;
    text_overlay_active = TRUE;
    chosen = '\0';
    rogue_allegro_render();

    while (chosen == '\0')
    {
	al_wait_for_event(queue, &event);

	if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
	{
	    chosen = 'Q';
	    break;
	}
	if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE)
	{
	    al_acknowledge_resize(display);
	    rogue_allegro_render();
	    continue;
	}

	if (event.type == ALLEGRO_EVENT_KEY_DOWN)
	{
	    switch (event.keyboard.keycode)
	    {
		default:
		    if (handle_view_key(event.keyboard.keycode))
			suppress_key_char_keycode = event.keyboard.keycode;
		    break;
		case ALLEGRO_KEY_ESCAPE:
		    chosen = ESCAPE;
		    break;
		case ALLEGRO_KEY_SPACE:
		    chosen = ' ';
		    break;
		case ALLEGRO_KEY_UP:
		case ALLEGRO_KEY_PAD_8:
		    text_overlay_selected = rogue_picker_move_selection(
			text_overlay_selected, text_overlay_line_count, -1);
		    break;
		case ALLEGRO_KEY_DOWN:
		case ALLEGRO_KEY_PAD_2:
		    text_overlay_selected = rogue_picker_move_selection(
			text_overlay_selected, text_overlay_line_count, 1);
		    break;
		case ALLEGRO_KEY_HOME:
		    text_overlay_selected = rogue_picker_clamp_selection(
			0, text_overlay_line_count);
		    break;
		case ALLEGRO_KEY_END:
		    text_overlay_selected = rogue_picker_clamp_selection(
			text_overlay_line_count - 1, text_overlay_line_count);
		    break;
		case ALLEGRO_KEY_ENTER:
		case ALLEGRO_KEY_PAD_ENTER:
		    if (text_overlay_selected >= 0)
		    {
			selected_key = rogue_picker_line_key(
			    text_overlay_lines[text_overlay_selected]);
			if (selected_key != '\0')
			    chosen = selected_key;
		    }
		    break;
	    }
	    rogue_allegro_render();
	    continue;
	}

	if (event.type == ALLEGRO_EVENT_KEY_CHAR)
	{
	    if (event.keyboard.keycode == suppress_key_char_keycode)
	    {
		suppress_key_char_keycode = 0;
		continue;
	    }
	    if (event.keyboard.unichar > 0 && event.keyboard.unichar < 128)
	    {
		typed = (char) event.keyboard.unichar;
		if (typed == ' ')
		{
		    chosen = ' ';
		    rogue_allegro_render();
		    continue;
		}
		match = rogue_picker_find_key(typed, lines,
					      text_overlay_line_count);
		if (match >= 0)
		{
		    text_overlay_selected = match;
		    chosen = rogue_picker_line_key(text_overlay_lines[match]);
		}
	    }
	    rogue_allegro_render();
	}
    }

    text_overlay_selectable = FALSE;
    text_overlay_active = FALSE;
    drain_keyboard_events();
    clear_held_movement();
    rogue_allegro_render();
    return chosen;
}

void
rogue_allegro_text_overlay_clear(void)
{
    text_overlay_active = FALSE;
    text_overlay_line_count = 0;
    text_overlay_title[0] = '\0';
    text_overlay_prompt[0] = '\0';
    text_overlay_selected = -1;
    text_overlay_scroll = 0;
    text_overlay_selectable = FALSE;
}

bool
rogue_allegro_text_input(const char *title, const char *prompt,
			 const char *initial, char *out, int out_size)
{
    ALLEGRO_EVENT event;
    char input[MAXSTR];
    char input_display[ROGUE_OVERLAY_LINE_LEN];
    int len;
    bool done;
    bool accepted;

    if (!started || out == NULL || out_size <= 0)
	return FALSE;

    if (title == NULL || *title == '\0')
	title = "Input";
    if (prompt == NULL)
	prompt = "";
    if (initial == NULL)
	initial = "";

    snprintf(input, sizeof(input), "%s", initial);
    input[sizeof(input) - 1] = '\0';
    len = (int) strlen(input);
    done = FALSE;
    accepted = FALSE;

    clear_held_movement();
    suppress_key_char_keycode = 0;

    while (!done)
    {
	rogue_allegro_text_overlay_begin(title);
	rogue_allegro_text_overlay_add(prompt);
	snprintf(input_display, sizeof(input_display), "> %s_", input);
	rogue_allegro_text_overlay_add(input_display);
	snprintf(text_overlay_prompt, sizeof(text_overlay_prompt),
		 "Enter accepts, Esc cancels, Backspace deletes");
	text_overlay_active = TRUE;
	rogue_allegro_render();

	al_wait_for_event(queue, &event);
	if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
	    break;
	if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE)
	{
	    al_acknowledge_resize(display);
	    rogue_allegro_render();
	    continue;
	}
	if (event.type == ALLEGRO_EVENT_KEY_DOWN)
	{
	    switch (event.keyboard.keycode)
	    {
		default:
		    if (handle_view_key(event.keyboard.keycode))
			suppress_key_char_keycode = event.keyboard.keycode;
		    break;
		case ALLEGRO_KEY_ESCAPE:
		    done = TRUE;
		    break;
		case ALLEGRO_KEY_ENTER:
		case ALLEGRO_KEY_PAD_ENTER:
		    accepted = TRUE;
		    done = TRUE;
		    break;
		case ALLEGRO_KEY_BACKSPACE:
		    if (len > 0)
			input[--len] = '\0';
		    break;
	    }
	    continue;
	}
	if (event.type == ALLEGRO_EVENT_KEY_CHAR
	    && event.keyboard.unichar > 0
	    && event.keyboard.unichar < 128)
	{
	    if (event.keyboard.keycode == suppress_key_char_keycode)
	    {
		suppress_key_char_keycode = 0;
		continue;
	    }
	    if (isprint((unsigned char) event.keyboard.unichar)
		&& len + 1 < (int) sizeof(input)
		&& len + 1 < out_size)
	    {
		input[len++] = (char) event.keyboard.unichar;
		input[len] = '\0';
	    }
	}
    }

    text_overlay_active = FALSE;
    drain_keyboard_events();
    if (accepted)
    {
	snprintf(out, out_size, "%s", input);
	out[out_size - 1] = '\0';
    }
    rogue_allegro_render();
    return accepted;
}

bool
rogue_allegro_confirm(const char *title, const char *prompt)
{
    char choice;
    char prompt_line[ROGUE_OVERLAY_LINE_LEN];

    if (!started)
	return FALSE;

    if (title == NULL || *title == '\0')
	title = "Confirm";
    if (prompt == NULL)
	prompt = "Are you sure?";

    rogue_allegro_text_overlay_begin(title);
    snprintf(prompt_line, sizeof(prompt_line), "%s", prompt);
    rogue_allegro_text_overlay_add(prompt_line);
    rogue_allegro_text_overlay_add("y) Yes");
    rogue_allegro_text_overlay_add("n) No");
    choice = rogue_allegro_text_overlay_pick(
	"Y accepts, N/Esc/Space cancels");
    rogue_allegro_text_overlay_clear();

    return (bool)(choice == 'y' || choice == 'Y');
}

bool
rogue_allegro_notice(const char *title, const char *message)
{
    if (!started)
	return FALSE;

    if (title == NULL || *title == '\0')
	title = "Rogue";
    if (message == NULL)
	message = "";

    rogue_allegro_text_overlay_begin(title);
    rogue_allegro_text_overlay_add(message);
    rogue_allegro_text_overlay_show("Space closes");
    rogue_allegro_text_overlay_clear();
    return TRUE;
}

void
rogue_allegro_show_death(const char *killer, int gold, bool has_amulet)
{
    if (killer == NULL)
	killer = "something unknown";

    snprintf(death_title, sizeof(death_title), "REST IN PEACE");
    snprintf(death_killer, sizeof(death_killer), "Killed by %s", killer);
    snprintf(death_gold, sizeof(death_gold), "%d gold%s", gold,
	     has_amulet ? " and the Amulet of Yendor" : "");
    snprintf(death_prompt, sizeof(death_prompt), "Press Enter to continue");

    text_overlay_active = FALSE;
    death_overlay_active = TRUE;
    rogue_allegro_render();
}

void
rogue_allegro_wait_for_return(const char *prompt)
{
    char ch;

    if (prompt != NULL && *prompt != '\0')
	snprintf(death_prompt, sizeof(death_prompt), "%s", prompt);

    if (!death_overlay_active)
    {
	snprintf(death_title, sizeof(death_title), "Rogue");
	snprintf(death_killer, sizeof(death_killer),
		 "Score screen is ready in the terminal");
	snprintf(death_gold, sizeof(death_gold),
		 "Use Enter here to continue");
	if (death_prompt[0] == '\0')
	    snprintf(death_prompt, sizeof(death_prompt), "Press Enter to continue");
	death_overlay_active = TRUE;
    }

    if (death_overlay_active)
	rogue_allegro_render();

    do
    {
	ch = rogue_allegro_readchar();
    } while (ch != '\n' && ch != '\r' && ch != 'Q');
}

void
rogue_allegro_shutdown(void)
{
    if (font != NULL)
	al_destroy_font(font);
    if (atlas != NULL)
	al_destroy_bitmap(atlas);
    if (queue != NULL)
	al_destroy_event_queue(queue);
    if (display != NULL)
	al_destroy_display(display);

    font = NULL;
    atlas = NULL;
    queue = NULL;
    display = NULL;
    started = FALSE;
    held_movement_keycode = 0;
    held_movement = '\0';
    held_movement_next_time = 0.0;
    prompt_active = FALSE;
    death_overlay_active = FALSE;
    text_overlay_active = FALSE;
    text_overlay_line_count = 0;
    message_log_count = 0;
    pending_damage_dealt = 0;
    pending_damage_taken = 0;
    pending_enemy_hp = 0;
    pending_enemy_max_hp = 0;
    blood_splat_count = 0;
}
