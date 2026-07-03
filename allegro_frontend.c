/*
 * Allegro 5 tile frontend.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
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
    bool fullscreen;
    int windowed_width;
    int windowed_height;
} ROGUE_ALLEGRO_SETTINGS;

static ROGUE_ALLEGRO_SETTINGS settings = {
    ROGUE_DEFAULT_TILE_DRAW_SIZE,
    ROGUE_ALLEGRO_VIEW_TILES,
    FALSE,
    FALSE,
    FALSE,
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
static int message_log_count = 0;
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
    settings.blood_spatter_enabled = json_bool_field(
	text, "bloodSpatter", settings.blood_spatter_enabled);
    settings.shader_enabled = json_bool_field(
	text, "shaders", settings.shader_enabled);
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
	    "  \"bloodSpatter\": %s,\n"
	    "  \"shaders\": %s\n"
	    "}\n",
	    settings.side_panel_log_enabled ? "true" : "false",
	    settings.blood_spatter_enabled ? "true" : "false",
	    settings.shader_enabled ? "true" : "false");
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
	snprintf(line, sizeof(line), "b) Blood Spatter: %s",
		 settings.blood_spatter_enabled ? "On" : "Off");
	rogue_allegro_text_overlay_add(line);
	snprintf(line, sizeof(line), "c) Shaders: %s",
		 settings.shader_enabled ? "On" : "Off");
	rogue_allegro_text_overlay_add(line);
	rogue_allegro_text_overlay_add("");
	rogue_allegro_text_overlay_add("Visual-only settings. Gameplay rules stay unchanged.");

	selected = rogue_allegro_text_overlay_pick(
	    "Enter toggles, Esc closes");
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
		settings.blood_spatter_enabled =
		    !settings.blood_spatter_enabled;
		save_settings();
		break;
	    case 'c':
	    case 'C':
		settings.shader_enabled = !settings.shader_enabled;
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
draw_glyph_cell(int screen_x, int screen_y, ROGUE_TILE_CELL *cell)
{
    ALLEGRO_COLOR bg, fg;
    int dx, dy;
    char text[2];

    dx = render_origin_x + screen_x * ROGUE_TILE_DRAW_SIZE;
    dy = render_origin_y + screen_y * ROGUE_TILE_DRAW_SIZE;

    bg = al_map_rgb(10, 10, 12);
    fg = al_map_rgb(220, 220, 210);
    if (cell->layer == ROGUE_TILE_OBJECT)
	fg = al_map_rgb(240, 210, 80);
    else if (cell->layer == ROGUE_TILE_ACTOR)
	fg = al_map_rgb(120, 210, 255);
    else if (cell->glyph == '|' || cell->glyph == '-')
	fg = al_map_rgb(150, 150, 150);

    al_draw_filled_rectangle(dx, dy, dx + ROGUE_TILE_DRAW_SIZE,
			     dy + ROGUE_TILE_DRAW_SIZE, bg);

    if (cell->glyph != ' ')
    {
	text[0] = cell->glyph;
	text[1] = '\0';
	al_draw_text(font, fg, dx + ROGUE_TILE_DRAW_SIZE / 2,
		     dy + 1, ALLEGRO_ALIGN_CENTRE, text);
    }
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
    edge = ROGUE_TILE_DRAW_SIZE / 6;
    if (edge < 4)
	edge = 4;
    if (edge > ROGUE_TILE_DRAW_SIZE)
	edge = ROGUE_TILE_DRAW_SIZE;
    source_edge = source_w / 6;
    if (source_edge < 4)
	source_edge = 4;

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

    al_draw_text(font, al_map_rgb(230, 230, 220), w / 2,
		 first_line_y, ALLEGRO_ALIGN_CENTRE, line);
    al_draw_text(font, al_map_rgb(180, 200, 255), 8, second_line_y, 0, huh);
    if (prompt_active)
	al_draw_text(font, al_map_rgb(245, 226, 170), w - 8,
		     second_line_y, ALLEGRO_ALIGN_RIGHT, prompt_text);
    else
	al_draw_text(font, al_map_rgb(160, 160, 160), w - 8,
		     second_line_y, ALLEGRO_ALIGN_RIGHT,
		     "F10 tiles  F11 full  F12 settings");
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
    char visible[ROGUE_OVERLAY_LINE_LEN];
    ALLEGRO_COLOR bg, border, title, text, muted;

    panel_w = side_panel_width();
    if (panel_w <= 0)
	return;

    x = display_width() - panel_w;
    bg = al_map_rgb(8, 9, 13);
    border = al_map_rgb(60, 64, 82);
    title = al_map_rgb(245, 226, 170);
    text = al_map_rgb(210, 220, 230);
    muted = al_map_rgb(130, 135, 150);
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

    first = message_log_count - max_lines;
    if (first < 0)
	first = 0;
    y = 72;
    for (i = first; i < message_log_count; i++)
    {
	snprintf(visible, sizeof(visible), "%s", message_log[i]);
	if ((int) strlen(visible) > max_chars)
	{
	    visible[max_chars - 1] = '.';
	    visible[max_chars - 2] = '.';
	    visible[max_chars - 3] = '.';
	    visible[max_chars] = '\0';
	}
	al_draw_text(font, text, x + 18, y, 0, visible);
	y += line_height + 2;
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
	    snprintf(message_log[i - 1], sizeof(message_log[i - 1]), "%s",
		     message_log[i]);
	message_log_count = ROGUE_MESSAGE_LOG_LINES - 1;
    }

    snprintf(message_log[message_log_count],
	     sizeof(message_log[message_log_count]), "%s", message);
    message_log_count++;
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
}
