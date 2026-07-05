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
#include "variant.h"

#define ROGUE_DEFAULT_TILE_DRAW_SIZE 32
#define ROGUE_STATUS_HEIGHT 96
#define ROGUE_MIN_TILE_DRAW_SIZE 16
#define ROGUE_MAX_TILE_DRAW_SIZE 64
#define ROGUE_TILE_ZOOM_STEP 4
#define ROGUE_DEFAULT_VIEW_COLS 40
#define ROGUE_MAX_VIEW_COLS 80
#define ROGUE_MAX_VIEW_ROWS 32
#define ROGUE_REPEAT_DELAY_SECONDS 0.30
#define ROGUE_REPEAT_RATE_SECONDS 0.13
#define ROGUE_FONT_SIZE 32
#define ROGUE_DETAIL_FONT_SIZE 32
#define ROGUE_SMALL_FONT_SIZE 16
#define ROGUE_OVERLAY_MAX_LINES 1536
#define ROGUE_OVERLAY_LINE_LEN 160
#define TEXT_OVERLAY_WRAP_CHARS 92
#define ROGUE_MANUAL_MAX_CHAPTERS 48
#define ROGUE_OVERLAY_SCROLL_DELAY_SECONDS 0.18
#define ROGUE_OVERLAY_SCROLL_RATE_SECONDS 0.045
#define ROGUE_MESSAGE_LOG_LINES 64
#define ROGUE_SIDE_PANEL_MIN_WIDTH 280
#define ROGUE_SIDE_PANEL_MAX_WIDTH 420
#define ROGUE_BLOOD_SPLATS 96
#define ROGUE_BLOOD_DROPS_PER_HIT 5
#define ROGUE_GLOOM_STRENGTH 0.62f
#define ROGUE_GLOOM_RADIUS 0.34f
#define ROGUE_DAMAGE_FLASH_SECONDS 0.22
#define ROGUE_DAMAGE_FLASH_ALPHA 0.44f
#define ROGUE_LOW_HP_PULSE_SECONDS 1.15
#define ROGUE_LOW_HP_PULSE_MIN_ALPHA 0.10f
#define ROGUE_LOW_HP_PULSE_MAX_ALPHA 0.30f
#define ROGUE_PIXEL_SHARPEN_STRENGTH 0.38f
#define ROGUE_POSTERIZE_LEVELS 6.0f
#define ROGUE_MIN_WALL_THICKNESS 1
#define ROGUE_DEFAULT_WALL_THICKNESS 2
#define ROGUE_THICK_WALL_THICKNESS 3
#define ROGUE_FULL_WALL_THICKNESS 4
#define ROGUE_MAX_WALL_THICKNESS 4

typedef enum rogue_allegro_view {
    ROGUE_ALLEGRO_VIEW_TILES,
    ROGUE_ALLEGRO_VIEW_GLYPHS
} ROGUE_ALLEGRO_VIEW;

typedef enum rogue_crt_effect_mode {
    ROGUE_CRT_OFF = 0,
    ROGUE_CRT_SUBTLE = 1,
    ROGUE_CRT_BALANCED = 2,
    ROGUE_CRT_DRAMATIC = 3
} ROGUE_CRT_EFFECT_MODE;

typedef struct rogue_allegro_settings {
    int tile_draw_size;
    ROGUE_ALLEGRO_VIEW view_mode;
    bool dungeon_gloom_enabled;
    bool damage_flash_enabled;
    bool low_hp_pulse_enabled;
    bool pixel_sharpen_enabled;
    bool posterize_enabled;
    ROGUE_CRT_EFFECT_MODE crt_effect_mode;
    bool blood_spatter_enabled;
    bool enemy_health_overlay_enabled;
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
    ROGUE_CRT_OFF,
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
static ALLEGRO_BITMAP *scene_bitmap = NULL;
static ALLEGRO_BITMAP *scene_source_bitmap = NULL;
static ALLEGRO_SHADER *gloom_shader = NULL;
static ALLEGRO_SHADER *postprocess_shader = NULL;
static ALLEGRO_FONT *font = NULL;
static ALLEGRO_FONT *detail_font = NULL;
static ALLEGRO_FONT *small_font = NULL;
static bool started = FALSE;
static bool smoke_mode = FALSE;
static bool shader_smoke_mode = FALSE;
static bool gloom_shader_failed = FALSE;
static bool postprocess_shader_failed = FALSE;
static unsigned char *atlas_tile_visible = NULL;
static int atlas_tile_visible_count = 0;
static int scene_bitmap_width = 0;
static int scene_bitmap_height = 0;
static int suppress_key_char_keycode = 0;
static int held_movement_keycode = 0;
static char held_movement = '\0';
static double held_movement_next_time = 0.0;
static double damage_flash_until = 0.0;
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
static bool text_overlay_fill_vertical = FALSE;
static int manual_chapter_lines[ROGUE_MANUAL_MAX_CHAPTERS];
static char manual_chapter_titles[ROGUE_MANUAL_MAX_CHAPTERS][ROGUE_OVERLAY_LINE_LEN];
static int manual_chapter_count = 0;
static char settings_path[512] = "settings.json";

void rogue_allegro_text_overlay_begin(const char *title);
void rogue_allegro_text_overlay_add(const char *line);
char rogue_allegro_text_overlay_show(const char *prompt);
char rogue_allegro_text_overlay_pick(const char *prompt);
void rogue_allegro_text_overlay_clear(void);
bool rogue_allegro_notice(const char *title, const char *message);
void rogue_allegro_render(void);
static int display_width(void);
static int display_height(void);
static int channel_clamp(float value);
static unsigned char posterize_channel(unsigned char value);
static void read_locked_rgba(ALLEGRO_LOCKED_REGION *region, int x, int y,
			     unsigned char *r, unsigned char *g,
			     unsigned char *b, unsigned char *a);
static void write_locked_rgba(ALLEGRO_LOCKED_REGION *region, int x, int y,
			      unsigned char r, unsigned char g,
			      unsigned char b, unsigned char a);
static void reset_manual_chapters(void);
static bool manual_line_is_heading(const char *line);
static void clean_manual_text(char *text);
static void record_manual_chapter(const char *title);
static char show_manual_chapter_picker(void);
static void show_manuals_menu_for_variant(const ROGUE_VARIANT_INFO *current);
static void drain_keyboard_events(void);
static void save_shader_smoke_bitmap(const char *filename,
				     ALLEGRO_BITMAP *bitmap);

void
rogue_allegro_enable_shader_smoke(void)
{
    shader_smoke_mode = TRUE;
}

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

static ALLEGRO_FONT *
load_detail_ui_font(void)
{
    ALLEGRO_FONT *loaded;

    loaded = al_load_ttf_font("assets/fonts/monogram/monogram.ttf",
			      ROGUE_DETAIL_FONT_SIZE,
			      ALLEGRO_TTF_MONOCHROME);
    if (loaded == NULL)
	loaded = al_load_ttf_font("../assets/fonts/monogram/monogram.ttf",
				  ROGUE_DETAIL_FONT_SIZE,
				  ALLEGRO_TTF_MONOCHROME);
    if (loaded == NULL)
	loaded = font;

    return loaded;
}

static ALLEGRO_FONT *
load_small_ui_font(void)
{
    ALLEGRO_FONT *loaded;

    loaded = al_load_ttf_font("assets/fonts/monogram/monogram.ttf",
			      ROGUE_SMALL_FONT_SIZE,
			      ALLEGRO_TTF_MONOCHROME);
    if (loaded == NULL)
	loaded = al_load_ttf_font("../assets/fonts/monogram/monogram.ttf",
				  ROGUE_SMALL_FONT_SIZE,
				  ALLEGRO_TTF_MONOCHROME);
    if (loaded == NULL)
	loaded = font;

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
    settings.enemy_health_overlay_enabled = json_bool_field(
	text, "enemyHealthOverlay", settings.enemy_health_overlay_enabled);
    settings.dungeon_gloom_enabled = json_bool_field(
	text, "dungeonGloom",
	json_bool_field(text, "shaders", settings.dungeon_gloom_enabled));
    settings.damage_flash_enabled = json_bool_field(
	text, "damageFlash", settings.damage_flash_enabled);
    settings.low_hp_pulse_enabled = json_bool_field(
	text, "lowHpPulse", settings.low_hp_pulse_enabled);
    settings.pixel_sharpen_enabled = json_bool_field(
	text, "pixelSharpen", settings.pixel_sharpen_enabled);
    settings.posterize_enabled = json_bool_field(
	text, "posterize", settings.posterize_enabled);
    settings.crt_effect_mode = (ROGUE_CRT_EFFECT_MODE)json_int_field(
	text, "crtEffect", settings.crt_effect_mode,
	ROGUE_CRT_OFF, ROGUE_CRT_DRAMATIC);
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
	    "  \"enemyHealthOverlay\": %s,\n"
	    "  \"dungeonGloom\": %s,\n"
	    "  \"damageFlash\": %s,\n"
	    "  \"lowHpPulse\": %s,\n"
	    "  \"pixelSharpen\": %s,\n"
	    "  \"posterize\": %s,\n"
	    "  \"crtEffect\": %d,\n"
	    "  \"wallThickness\": %d\n"
	    "}\n",
	    settings.side_panel_log_enabled ? "true" : "false",
	    settings.stylized_log_enabled ? "true" : "false",
	    settings.stylized_bottom_bar_enabled ? "true" : "false",
	    settings.blood_spatter_enabled ? "true" : "false",
	    settings.enemy_health_overlay_enabled ? "true" : "false",
	    settings.dungeon_gloom_enabled ? "true" : "false",
	    settings.damage_flash_enabled ? "true" : "false",
	    settings.low_hp_pulse_enabled ? "true" : "false",
	    settings.pixel_sharpen_enabled ? "true" : "false",
	    settings.posterize_enabled ? "true" : "false",
	    settings.crt_effect_mode,
	    settings.wall_thickness);
    fclose(file);
}

static void
variant_wrap_append(char *dst, int dst_size, const char *left,
		    const char *middle, const char *right)
{
    snprintf(dst, dst_size, "%s%s%s", left, middle, right);
    dst[dst_size - 1] = '\0';
}

static int
draw_variant_wrapped_text(ALLEGRO_FONT *draw_font, ALLEGRO_COLOR color,
			  int x, int y, int max_width, int line_height,
			  const char *first_prefix,
			  const char *next_prefix, const char *text,
			  bool draw)
{
    char line[1024], candidate[1024], word[256];
    const char *p;
    const char *prefix;
    int len, line_has_text;

    if (draw_font == NULL || text == NULL)
	return y;

    prefix = first_prefix != NULL ? first_prefix : "";
    next_prefix = next_prefix != NULL ? next_prefix : prefix;
    snprintf(line, sizeof(line), "%s", prefix);
    line[sizeof(line) - 1] = '\0';
    line_has_text = FALSE;
    p = text;

    while (*p != '\0')
    {
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
	    p++;
	if (*p == '\0')
	    break;

	len = 0;
	while (p[len] != '\0' && p[len] != ' ' && p[len] != '\t'
	    && p[len] != '\r' && p[len] != '\n')
	{
	    if (len < (int) sizeof(word) - 1)
		word[len] = p[len];
	    len++;
	}
	word[len < (int) sizeof(word) ? len : (int) sizeof(word) - 1] = '\0';
	p += len;

	variant_wrap_append(candidate, sizeof(candidate), line,
			    line_has_text ? " " : "", word);
	if (line_has_text
	    && al_get_text_width(draw_font, candidate) > max_width)
	{
	    if (draw)
		al_draw_text(draw_font, color, x, y, 0, line);
	    y += line_height;
	    snprintf(line, sizeof(line), "%s%s", next_prefix, word);
	    line[sizeof(line) - 1] = '\0';
	}
	else
	{
	    snprintf(line, sizeof(line), "%s", candidate);
	    line[sizeof(line) - 1] = '\0';
	}
	line_has_text = TRUE;
    }

    if (line_has_text || prefix[0] != '\0')
    {
	if (draw)
	    al_draw_text(draw_font, color, x, y, 0, line);
	y += line_height;
    }

    return y;
}

static const ROGUE_VARIANT_INFO *
variant_picker_at(int display_index)
{
    const ROGUE_VARIANT_INFO *left;
    const ROGUE_VARIANT_INFO *right;
    int i, j, rank, count;

    count = rogue_variant_count();
    if (display_index < 0 || display_index >= count)
	return NULL;

    for (i = 0; i < count; i++)
    {
	left = rogue_variant_at(i);
	if (left == NULL)
	    continue;
	rank = 0;
	for (j = 0; j < count; j++)
	{
	    right = rogue_variant_at(j);
	    if (right == NULL)
		continue;
	    if (right->release_year < left->release_year
		|| (right->release_year == left->release_year && j < i))
		rank++;
	}
	if (rank == display_index)
	    return left;
    }

    return rogue_variant_at(display_index);
}

static int
variant_picker_index_of(const ROGUE_VARIANT_INFO *variant)
{
    int i;

    if (variant == NULL)
	return 0;
    for (i = 0; i < rogue_variant_count(); i++)
	if (variant_picker_at(i) == variant)
	    return i;
    return 0;
}

static int
variant_picker_draw_detail(const ROGUE_VARIANT_INFO *current,
			   ALLEGRO_FONT *body_font, int x, int y,
			   int detail_width, int detail_line_height,
			   ALLEGRO_COLOR muted, ALLEGRO_COLOR text,
			   bool draw)
{
    int i;

    y = draw_variant_wrapped_text(body_font, muted, x, y, detail_width,
				  detail_line_height, "Released: ",
				  "          ", current->era, draw);
    y = draw_variant_wrapped_text(body_font, muted, x, y, detail_width,
				  detail_line_height, "Lineage: ",
				  "         ", current->lineage, draw);
    y = draw_variant_wrapped_text(body_font, muted, x, y, detail_width,
				  detail_line_height, "License: ",
				  "         ", current->license, draw);
    y = draw_variant_wrapped_text(body_font, muted, x, y, detail_width,
				  detail_line_height, "Rules: ",
				  "       ", current->tile_support, draw);
    if (current->manuals[0].title != NULL)
    {
	y += 6;
	y = draw_variant_wrapped_text(body_font, muted, x, y, detail_width,
				      detail_line_height, "Manuals: ",
				      "         ",
				      "Press F1 to read bundled manuals and guides.",
				      draw);
	for (i = 0; i < ROGUE_VARIANT_MAX_MANUALS
	    && current->manuals[i].title != NULL; i++)
	{
	    y = draw_variant_wrapped_text(body_font, text, x, y,
					  detail_width, detail_line_height,
					  "- ", "  ",
					  current->manuals[i].title, draw);
	}
    }
    y += 6;
    y = draw_variant_wrapped_text(body_font, text, x, y, detail_width,
				  detail_line_height, "", "",
				  current->summary, draw);
    y += 8;

    for (i = 0; i < ROGUE_VARIANT_MAX_FEATURES
	&& current->features[i] != NULL; i++)
    {
	y = draw_variant_wrapped_text(body_font, text, x, y, detail_width,
				      detail_line_height, "- ", "  ",
				      current->features[i], draw);
    }

    return y;
}

static void
draw_variant_picker(int selected, int detail_scroll, int *max_detail_scroll)
{
    int i, y, x, panel_x, panel_y, panel_w, panel_h;
    int line_height, detail_line_height, detail_width;
    int left_w, detail_top, detail_bottom, detail_h, detail_end;
    ALLEGRO_FONT *body_font;
    const ROGUE_VARIANT_INFO *variant;
    const ROGUE_VARIANT_INFO *current;
    ALLEGRO_COLOR bg, border, title, text, muted, selected_bg, accent;

    current = variant_picker_at(selected);
    if (current == NULL)
	current = rogue_variant_default();

    bg = al_map_rgb(6, 7, 10);
    border = al_map_rgb(105, 112, 140);
    title = al_map_rgb(245, 226, 170);
    text = al_map_rgb(230, 232, 224);
    muted = al_map_rgb(156, 162, 178);
    selected_bg = al_map_rgb(44, 48, 66);
    accent = al_map_rgb(134, 190, 255);
    line_height = al_get_font_line_height(font);
    body_font = detail_font != NULL ? detail_font : font;
    detail_line_height = al_get_font_line_height(body_font);

    al_set_target_backbuffer(display);
    al_clear_to_color(bg);

    panel_w = display_width() - 100;
    if (panel_w > 1180)
	panel_w = 1180;
    if (panel_w < 560)
	panel_w = display_width() - 40;
    panel_h = display_height() - 120;
    if (panel_h < 430)
	panel_h = display_height() - 40;
    panel_x = (display_width() - panel_w) / 2;
    panel_y = (display_height() - panel_h) / 2;

    al_draw_rectangle(panel_x + 0.5, panel_y + 0.5,
		      panel_x + panel_w - 0.5, panel_y + panel_h - 0.5,
		      border, 2);
    al_draw_text(font, title, panel_x + 24, panel_y + 22, 0,
		 "Choose Rogue Version");
    al_draw_text(font, muted, panel_x + 24, panel_y + 62, 0,
		 "Arrow keys select. Enter starts. F1 reads manuals.");
    al_draw_line(panel_x + 24, panel_y + 100, panel_x + panel_w - 24,
		 panel_y + 100, al_map_rgb(62, 66, 84), 1);

    y = panel_y + 124;
    left_w = panel_w / 3;
    if (left_w < 230)
	left_w = 230;
    if (left_w > 340)
	left_w = 340;
    for (i = 0; i < rogue_variant_count(); i++)
    {
	variant = variant_picker_at(i);
	if (variant == NULL)
	    continue;
	if (i == selected)
	    al_draw_filled_rectangle(panel_x + 22, y - 4,
				     panel_x + left_w - 18,
				     y + line_height + 8, selected_bg);
	al_draw_textf(font, i == selected ? accent : text,
		      panel_x + 34, y, 0, "%d. %s", i + 1, variant->name);
	y += line_height + 12;
    }

    al_draw_line(panel_x + left_w, panel_y + 112,
		 panel_x + left_w, panel_y + panel_h - 54,
		 al_map_rgb(62, 66, 84), 1);

    x = panel_x + left_w + 24;
    y = panel_y + 124;
    detail_width = panel_x + panel_w - 24 - x;
    al_draw_text(font, title, x, y, 0, current->name);
    detail_top = y + detail_line_height + 8;
    detail_bottom = panel_y + panel_h - 56;
    detail_h = detail_bottom - detail_top;
    detail_end = variant_picker_draw_detail(current, body_font, x, detail_top,
					    detail_width, detail_line_height,
					    muted, text, FALSE);
    if (max_detail_scroll != NULL)
    {
	*max_detail_scroll = detail_end - detail_top - detail_h;
	if (*max_detail_scroll < 0)
	    *max_detail_scroll = 0;
    }
    if (detail_scroll < 0)
	detail_scroll = 0;
    if (max_detail_scroll != NULL && detail_scroll > *max_detail_scroll)
	detail_scroll = *max_detail_scroll;

    al_set_clipping_rectangle(x, detail_top, detail_width, detail_h);
    variant_picker_draw_detail(current, body_font, x,
			       detail_top - detail_scroll, detail_width,
			       detail_line_height, muted, text, TRUE);
    al_set_clipping_rectangle(0, 0, display_width(), display_height());

    al_draw_text(small_font, muted, panel_x + panel_w - 24,
		 panel_y + panel_h - 38, ALLEGRO_ALIGN_RIGHT,
		 max_detail_scroll != NULL && *max_detail_scroll > 0
		 ? "PgUp/PgDn scroll details. Esc keeps the default"
		 : "Esc keeps the default");
    al_flip_display();
}

const char *
rogue_allegro_choose_variant(void)
{
    ALLEGRO_EVENT event;
    int selected;
    int detail_scroll;
    int max_detail_scroll;
    int page_step;
    const ROGUE_VARIANT_INFO *variant;
    const ROGUE_VARIANT_INFO *selected_variant;

    if (!started)
	return rogue_variant_current()->id;

    selected = variant_picker_index_of(rogue_variant_current());
    detail_scroll = 0;
    max_detail_scroll = 0;
    for (;;)
    {
	draw_variant_picker(selected, detail_scroll, &max_detail_scroll);
	if (detail_scroll > max_detail_scroll)
	    detail_scroll = max_detail_scroll;
	al_wait_for_event(queue, &event);

	if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
	    return NULL;
	if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE)
	{
	    al_acknowledge_resize(display);
	    continue;
	}
	if (event.type != ALLEGRO_EVENT_KEY_DOWN)
	    continue;

	if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
	    return rogue_variant_default()->id;
	if (event.keyboard.keycode == ALLEGRO_KEY_F1)
	{
	    selected_variant = variant_picker_at(selected);
	    show_manuals_menu_for_variant(selected_variant);
	    continue;
	}
	if (event.keyboard.keycode == ALLEGRO_KEY_ENTER
	    || event.keyboard.keycode == ALLEGRO_KEY_PAD_ENTER)
	{
	    variant = variant_picker_at(selected);
	    return variant == NULL ? rogue_variant_default()->id : variant->id;
	}
	if (event.keyboard.keycode == ALLEGRO_KEY_UP)
	{
	    selected--;
	    if (selected < 0)
		selected = rogue_variant_count() - 1;
	    detail_scroll = 0;
	}
	else if (event.keyboard.keycode == ALLEGRO_KEY_DOWN)
	{
	    selected++;
	    if (selected >= rogue_variant_count())
		selected = 0;
	    detail_scroll = 0;
	}
	else if (event.keyboard.keycode == ALLEGRO_KEY_PGUP)
	{
	    page_step = display_height() / 3;
	    if (page_step < 120)
		page_step = 120;
	    detail_scroll -= page_step;
	    if (detail_scroll < 0)
		detail_scroll = 0;
	}
	else if (event.keyboard.keycode == ALLEGRO_KEY_PGDN)
	{
	    page_step = display_height() / 3;
	    if (page_step < 120)
		page_step = 120;
	    detail_scroll += page_step;
	    if (detail_scroll > max_detail_scroll)
		detail_scroll = max_detail_scroll;
	}
	else if (event.keyboard.keycode >= ALLEGRO_KEY_1
		 && event.keyboard.keycode < ALLEGRO_KEY_1
		    + rogue_variant_count())
	{
	    selected = event.keyboard.keycode - ALLEGRO_KEY_1;
	    detail_scroll = 0;
	}
    }
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
    int max_cols;

    cols = play_area_width() / ROGUE_TILE_DRAW_SIZE;
    max_cols = rogue_variant_map_cols();
    if (max_cols > ROGUE_MAX_VIEW_COLS)
	max_cols = ROGUE_MAX_VIEW_COLS;
    return clamp_int(cols, 1, max_cols);
}

static int
view_rows(void)
{
    int available;
    int rows;
    int max_rows;

    available = display_height() - ROGUE_STATUS_HEIGHT;
    if (available < ROGUE_TILE_DRAW_SIZE)
	available = ROGUE_TILE_DRAW_SIZE;
    rows = available / ROGUE_TILE_DRAW_SIZE;
    max_rows = rogue_variant_map_rows();
    if (max_rows > ROGUE_MAX_VIEW_ROWS)
	max_rows = ROGUE_MAX_VIEW_ROWS;
    return clamp_int(rows, 1, max_rows);
}

static int
camera_left(int cols)
{
    int left;
    int max_left;
    int hero_x;
    int map_cols;

    rogue_variant_hero_position(NULL, &hero_x);
    map_cols = rogue_variant_map_cols();
    left = hero_x - cols / 2;
    max_left = map_cols - cols;

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
    int hero_y;
    int map_rows;

    rogue_variant_hero_position(&hero_y, NULL);
    map_rows = rogue_variant_map_rows();
    top = hero_y - rows / 2;
    max_top = map_rows - rows;

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

static void
clear_atlas_visibility_cache(void)
{
    if (atlas_tile_visible != NULL)
	free(atlas_tile_visible);
    atlas_tile_visible = NULL;
    atlas_tile_visible_count = 0;
}

static bool
atlas_index_has_visible_pixels(int atlas_index)
{
    int columns;
    int source_w, source_h;
    int image_w, image_h;
    int sx, sy;
    int x, y;
    int visible_pixels;
    int alpha_pixels;
    unsigned char r, g, b, a;
    ALLEGRO_COLOR pixel;

    if (atlas == NULL || atlas_index < 0)
	return FALSE;

    columns = rogue_tilepack_columns();
    source_w = rogue_tilepack_source_width();
    source_h = rogue_tilepack_source_height();
    image_w = al_get_bitmap_width(atlas);
    image_h = al_get_bitmap_height(atlas);
    if (columns <= 0 || source_w <= 0 || source_h <= 0)
	return FALSE;

    sx = (atlas_index % columns) * source_w;
    sy = (atlas_index / columns) * source_h;
    if (sx < 0 || sy < 0 || sx + source_w > image_w || sy + source_h > image_h)
	return FALSE;

    visible_pixels = 0;
    alpha_pixels = 0;
    for (y = sy; y < sy + source_h; y += 2)
	for (x = sx; x < sx + source_w; x += 2)
	{
	    pixel = al_get_pixel(atlas, x, y);
	    al_unmap_rgba(pixel, &r, &g, &b, &a);
	    if (a > 24)
	    {
		alpha_pixels++;
		if ((int) r + (int) g + (int) b > 80
		    || r > 56 || g > 56 || b > 56)
		{
		    visible_pixels++;
		    if (visible_pixels >= 4)
			return TRUE;
		}
	    }
	}

    return (bool)(alpha_pixels > 0 && visible_pixels > 0);
}

static void
rebuild_atlas_visibility_cache(void)
{
    int columns;
    int rows;
    int image_h;
    int source_h;

    clear_atlas_visibility_cache();
    if (atlas == NULL)
	return;

    columns = rogue_tilepack_columns();
    source_h = rogue_tilepack_source_height();
    image_h = al_get_bitmap_height(atlas);
    if (columns <= 0 || source_h <= 0)
	return;

    rows = image_h / source_h;
    atlas_tile_visible_count = columns * rows;
    if (atlas_tile_visible_count <= 0)
	return;

    atlas_tile_visible = (unsigned char *) calloc(
	(size_t) atlas_tile_visible_count, sizeof(*atlas_tile_visible));
    if (atlas_tile_visible == NULL)
    {
	atlas_tile_visible_count = 0;
	return;
    }
    memset(atlas_tile_visible, 255, (size_t) atlas_tile_visible_count);
}

static bool
atlas_tile_is_visible(int atlas_index)
{
    if (atlas_index < 0)
	return FALSE;
    if (atlas_tile_visible == NULL || atlas_tile_visible_count <= 0)
	return TRUE;
    if (atlas_index >= atlas_tile_visible_count)
	return FALSE;
    if (atlas_tile_visible[atlas_index] == 255)
	atlas_tile_visible[atlas_index] =
	    atlas_index_has_visible_pixels(atlas_index) ? 1 : 0;
    return (bool)(atlas_tile_visible[atlas_index] != 0);
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
    rebuild_atlas_visibility_cache();
    return TRUE;
}

static const char *
wall_thickness_name(void)
{
    switch (settings.wall_thickness)
    {
	case ROGUE_MIN_WALL_THICKNESS:
	    return "Thin";
	case ROGUE_THICK_WALL_THICKNESS:
	    return "Thick";
	case ROGUE_FULL_WALL_THICKNESS:
	    return "Full";
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

static const char *
crt_effect_label(ROGUE_CRT_EFFECT_MODE mode)
{
    switch (mode)
    {
	case ROGUE_CRT_SUBTLE:
	    return "Subtle";
	case ROGUE_CRT_BALANCED:
	    return "Balanced";
	case ROGUE_CRT_DRAMATIC:
	    return "Dramatic";
	default:
	    return "Off";
    }
}

static void
cycle_crt_effect_mode(void)
{
    switch (settings.crt_effect_mode)
    {
	case ROGUE_CRT_OFF:
	    settings.crt_effect_mode = ROGUE_CRT_SUBTLE;
	    break;
	case ROGUE_CRT_SUBTLE:
	    settings.crt_effect_mode = ROGUE_CRT_BALANCED;
	    break;
	case ROGUE_CRT_BALANCED:
	    settings.crt_effect_mode = ROGUE_CRT_DRAMATIC;
	    break;
	default:
	    settings.crt_effect_mode = ROGUE_CRT_OFF;
	    break;
    }
}

static bool
postprocess_enabled(void)
{
    return (bool)(settings.pixel_sharpen_enabled
		  || settings.posterize_enabled);
}

static bool
scene_effects_need_bitmap(void)
{
    return (bool)(settings.dungeon_gloom_enabled || postprocess_enabled());
}

static const char *postprocess_pixel_shader_source =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "uniform sampler2D u_scene_texture;\n"
    "uniform bool u_pixel_sharpen_enabled;\n"
    "uniform bool u_posterize_enabled;\n"
    "uniform float u_pixel_sharpen_strength;\n"
    "uniform float u_posterize_levels;\n"
    "uniform vec2 u_scene_texel_size;\n"
    "void main()\n"
    "{\n"
    "    vec2 uv = vec2(gl_FragCoord.x * u_scene_texel_size.x,\n"
    "                   1.0 - gl_FragCoord.y * u_scene_texel_size.y);\n"
    "    uv = clamp(uv, vec2(0.0, 0.0), vec2(1.0, 1.0));\n"
    "    vec4 color = texture2D(u_scene_texture, uv);\n"
    "    if (u_pixel_sharpen_enabled)\n"
    "    {\n"
    "        vec3 north = texture2D(u_scene_texture, uv + vec2(0.0, -u_scene_texel_size.y)).rgb;\n"
    "        vec3 south = texture2D(u_scene_texture, uv + vec2(0.0, u_scene_texel_size.y)).rgb;\n"
    "        vec3 east = texture2D(u_scene_texture, uv + vec2(u_scene_texel_size.x, 0.0)).rgb;\n"
    "        vec3 west = texture2D(u_scene_texture, uv + vec2(-u_scene_texel_size.x, 0.0)).rgb;\n"
    "        vec3 blur = (north + south + east + west) * 0.25;\n"
    "        color.rgb = clamp(color.rgb + (color.rgb - blur) * u_pixel_sharpen_strength, 0.0, 1.0);\n"
    "    }\n"
    "    if (u_posterize_enabled)\n"
    "        color.rgb = floor(color.rgb * u_posterize_levels + 0.5) / u_posterize_levels;\n"
    "    gl_FragColor = color;\n"
    "}\n";

static const char *gloom_pixel_shader_source =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "uniform float u_gloom_strength;\n"
    "uniform float u_gloom_radius;\n"
    "uniform vec2 u_screen_size;\n"
    "void main()\n"
    "{\n"
    "    vec2 uv = gl_FragCoord.xy / u_screen_size;\n"
    "    vec2 p = uv - vec2(0.5, 0.5);\n"
    "    p.x *= u_screen_size.x / u_screen_size.y;\n"
    "    float dist = length(p);\n"
    "    float gloom = smoothstep(u_gloom_radius, 0.78, dist);\n"
    "    gl_FragColor = vec4(0.0, 0.0, 0.0, gloom * u_gloom_strength);\n"
    "}\n";

static bool
ensure_scene_bitmap(void)
{
    int w, h;

    if (display == NULL)
	return FALSE;

    w = display_width();
    h = display_height();
    if (w <= 0 || h <= 0)
	return FALSE;

    if (scene_bitmap != NULL
	&& scene_source_bitmap != NULL
	&& scene_bitmap_width == w
	&& scene_bitmap_height == h)
	return TRUE;

    if (scene_bitmap != NULL)
	al_destroy_bitmap(scene_bitmap);
    if (scene_source_bitmap != NULL)
	al_destroy_bitmap(scene_source_bitmap);

    scene_bitmap = al_create_bitmap(w, h);
    scene_source_bitmap = al_create_bitmap(w, h);
    if (scene_bitmap == NULL || scene_source_bitmap == NULL)
    {
	if (scene_bitmap != NULL)
	    al_destroy_bitmap(scene_bitmap);
	if (scene_source_bitmap != NULL)
	    al_destroy_bitmap(scene_source_bitmap);
	scene_bitmap = NULL;
	scene_source_bitmap = NULL;
	scene_bitmap_width = 0;
	scene_bitmap_height = 0;
	return FALSE;
    }

    scene_bitmap_width = w;
    scene_bitmap_height = h;
    return TRUE;
}

static void
copy_scene_to_source_bitmap(void)
{
    if (scene_bitmap == NULL || scene_source_bitmap == NULL)
	return;

    al_use_shader(NULL);
    al_set_target_bitmap(scene_source_bitmap);
    al_clear_to_color(al_map_rgb(0, 0, 0));
    al_draw_bitmap(scene_bitmap, 0, 0, 0);
    save_shader_smoke_bitmap("rogue_scene_source_shader.png",
			     scene_source_bitmap);
}

static void
shader_smoke_path(char *out, size_t out_size, const char *filename)
{
    char base[512];
    char *slash;

    if (out_size == 0)
	return;

    snprintf(base, sizeof(base), "%s", settings_path);
    slash = strrchr(base, '\\');
    if (slash == NULL)
	slash = strrchr(base, '/');
    if (slash != NULL)
    {
	*slash = '\0';
	snprintf(out, out_size, "%s\\%s", base, filename);
	return;
    }

    snprintf(out, out_size, "%s", filename);
}

static void
save_shader_smoke_bitmap(const char *filename, ALLEGRO_BITMAP *bitmap)
{
    char path[512];

    if (!shader_smoke_mode || bitmap == NULL)
	return;

    shader_smoke_path(path, sizeof(path), filename);
    if (!al_save_bitmap(path, bitmap))
	fprintf(stderr, "Could not save shader smoke bitmap: %s\n", path);
}

static bool
ensure_gloom_shader(void)
{
    ALLEGRO_SHADER *shader;
    ALLEGRO_SHADER_PLATFORM platform;
    const char *vertex_source;
    const char *log;

    if (gloom_shader != NULL)
	return TRUE;
    if (gloom_shader_failed)
	return FALSE;

    shader = al_create_shader(ALLEGRO_SHADER_GLSL);
    if (shader == NULL)
    {
	gloom_shader_failed = TRUE;
	return FALSE;
    }

    platform = al_get_shader_platform(shader);
    vertex_source = al_get_default_shader_source(platform,
						 ALLEGRO_VERTEX_SHADER);
    if (vertex_source == NULL
	|| !al_attach_shader_source(shader, ALLEGRO_VERTEX_SHADER,
				    vertex_source)
	|| !al_attach_shader_source(shader, ALLEGRO_PIXEL_SHADER,
				    gloom_pixel_shader_source)
	|| !al_build_shader(shader))
    {
	log = al_get_shader_log(shader);
	if (log != NULL && *log != '\0')
	    fprintf(stderr, "Gloom shader disabled: %s\n", log);
	al_destroy_shader(shader);
	gloom_shader_failed = TRUE;
	return FALSE;
    }

    gloom_shader = shader;
    return TRUE;
}

static bool
ensure_postprocess_shader(void)
{
    ALLEGRO_SHADER *shader;
    ALLEGRO_SHADER_PLATFORM platform;
    const char *vertex_source;
    const char *log;

    if (postprocess_shader != NULL)
	return TRUE;
    if (postprocess_shader_failed)
	return FALSE;

    shader = al_create_shader(ALLEGRO_SHADER_GLSL);
    if (shader == NULL)
    {
	postprocess_shader_failed = TRUE;
	return FALSE;
    }

    platform = al_get_shader_platform(shader);
    vertex_source = al_get_default_shader_source(platform,
						 ALLEGRO_VERTEX_SHADER);
    if (vertex_source == NULL
	|| !al_attach_shader_source(shader, ALLEGRO_VERTEX_SHADER,
				    vertex_source)
	|| !al_attach_shader_source(shader, ALLEGRO_PIXEL_SHADER,
				    postprocess_pixel_shader_source)
	|| !al_build_shader(shader))
    {
	log = al_get_shader_log(shader);
	if (log != NULL && *log != '\0')
	    fprintf(stderr, "Postprocess shader disabled: %s\n", log);
	al_destroy_shader(shader);
	postprocess_shader_failed = TRUE;
	return FALSE;
    }

    postprocess_shader = shader;
    return TRUE;
}

static bool
draw_scene_with_postprocess_shader(void)
{
    int x, y;
    unsigned char r, g, b, a;
    unsigned char north_r, north_g, north_b, ignored_a;
    unsigned char south_r, south_g, south_b;
    unsigned char east_r, east_g, east_b;
    unsigned char west_r, west_g, west_b;
    ALLEGRO_LOCKED_REGION *source_lock;
    ALLEGRO_LOCKED_REGION *target_lock;

    if (!postprocess_enabled())
	return FALSE;
    if (scene_bitmap == NULL || scene_source_bitmap == NULL)
	return FALSE;

    al_use_shader(NULL);
    source_lock = al_lock_bitmap(scene_source_bitmap,
				 ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,
				 ALLEGRO_LOCK_READONLY);
    if (source_lock == NULL)
	return FALSE;
    target_lock = al_lock_bitmap(scene_bitmap,
				 ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,
				 ALLEGRO_LOCK_WRITEONLY);
    if (target_lock == NULL)
    {
	al_unlock_bitmap(scene_source_bitmap);
	return FALSE;
    }

    for (y = 0; y < scene_bitmap_height; y++)
	for (x = 0; x < scene_bitmap_width; x++)
	{
	    read_locked_rgba(source_lock, x, y, &r, &g, &b, &a);
	    if (settings.pixel_sharpen_enabled)
	    {
		read_locked_rgba(source_lock, x, y > 0 ? y - 1 : y,
				 &north_r, &north_g, &north_b, &ignored_a);
		read_locked_rgba(source_lock, x,
				 y + 1 < scene_bitmap_height ? y + 1 : y,
				 &south_r, &south_g, &south_b, &ignored_a);
		read_locked_rgba(source_lock,
				 x + 1 < scene_bitmap_width ? x + 1 : x,
				 y, &east_r, &east_g, &east_b, &ignored_a);
		read_locked_rgba(source_lock, x > 0 ? x - 1 : x, y,
				 &west_r, &west_g, &west_b, &ignored_a);
		r = (unsigned char)channel_clamp(
		    (float)r + ((float)r - ((float)north_r
					   + (float)south_r
					   + (float)east_r
					   + (float)west_r) * 0.25f)
		    * ROGUE_PIXEL_SHARPEN_STRENGTH);
		g = (unsigned char)channel_clamp(
		    (float)g + ((float)g - ((float)north_g
					   + (float)south_g
					   + (float)east_g
					   + (float)west_g) * 0.25f)
		    * ROGUE_PIXEL_SHARPEN_STRENGTH);
		b = (unsigned char)channel_clamp(
		    (float)b + ((float)b - ((float)north_b
					   + (float)south_b
					   + (float)east_b
					   + (float)west_b) * 0.25f)
		    * ROGUE_PIXEL_SHARPEN_STRENGTH);
	    }
	    if (settings.posterize_enabled)
	    {
		r = posterize_channel(r);
		g = posterize_channel(g);
		b = posterize_channel(b);
	    }
	    write_locked_rgba(target_lock, x, y, r, g, b, a);
	}

    al_unlock_bitmap(scene_source_bitmap);
    al_unlock_bitmap(scene_bitmap);
    al_set_target_backbuffer(display);
    al_draw_bitmap(scene_bitmap, 0, 0, 0);
    return TRUE;
}

static int
channel_clamp(float value)
{
    if (value < 0.0f)
	return 0;
    if (value > 255.0f)
	return 255;
    return (int)(value + 0.5f);
}

static unsigned char
posterize_channel(unsigned char value)
{
    int step;
    int posterized;

    step = (int)((float)value * ROGUE_POSTERIZE_LEVELS / 255.0f + 0.5f);
    posterized =
	(int)((float)step * 255.0f / ROGUE_POSTERIZE_LEVELS + 0.5f);
    if (posterized < 0)
	return 0;
    if (posterized > 255)
	return 255;
    return (unsigned char)posterized;
}

static void
read_locked_rgba(ALLEGRO_LOCKED_REGION *region, int x, int y,
		 unsigned char *r, unsigned char *g,
		 unsigned char *b, unsigned char *a)
{
    unsigned char *pixel;

    pixel = (unsigned char *)region->data + (y * region->pitch)
	    + (x * region->pixel_size);
    *r = pixel[0];
    *g = pixel[1];
    *b = pixel[2];
    *a = pixel[3];
}

static void
write_locked_rgba(ALLEGRO_LOCKED_REGION *region, int x, int y,
		  unsigned char r, unsigned char g,
		  unsigned char b, unsigned char a)
{
    unsigned char *pixel;

    pixel = (unsigned char *)region->data + (y * region->pitch)
	    + (x * region->pixel_size);
    pixel[0] = r;
    pixel[1] = g;
    pixel[2] = b;
    pixel[3] = a;
}

static void
draw_scene_with_gloom_shader(void)
{
    bool shader_uniforms_ready;
    float screen_size[2];
    int old_blender_op;
    int old_blender_src;
    int old_blender_dst;

    if (scene_bitmap == NULL)
	return;

    al_set_target_backbuffer(display);
    al_clear_to_color(al_map_rgb(0, 0, 0));
    if (!draw_scene_with_postprocess_shader())
	al_draw_bitmap(scene_bitmap, 0, 0, 0);
    save_shader_smoke_bitmap("rogue_scene_after_postprocess.png",
			     al_get_backbuffer(display));

    if (settings.dungeon_gloom_enabled && ensure_gloom_shader())
    {
	if (al_use_shader(gloom_shader))
	{
	    screen_size[0] = (float)display_width();
	    screen_size[1] = (float)display_height();
	    shader_uniforms_ready =
		(bool)(al_set_shader_float("u_gloom_strength",
					   ROGUE_GLOOM_STRENGTH)
		       && al_set_shader_float("u_gloom_radius",
					      ROGUE_GLOOM_RADIUS)
		       && al_set_shader_float_vector("u_screen_size", 2,
						     screen_size, 1));
	    if (shader_uniforms_ready)
	    {
		al_get_blender(&old_blender_op, &old_blender_src,
			       &old_blender_dst);
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA,
			       ALLEGRO_INVERSE_ALPHA);
		al_draw_filled_rectangle(0, 0, (float)display_width(),
					 (float)display_height(),
					 al_map_rgba_f(1, 1, 1, 1));
		al_set_blender(old_blender_op, old_blender_src,
			       old_blender_dst);
		al_use_shader(NULL);
		save_shader_smoke_bitmap("rogue_scene_after_shader.png",
					 al_get_backbuffer(display));
		return;
	    }

	    al_use_shader(NULL);
	}
    }

    save_shader_smoke_bitmap("rogue_scene_after_shader.png",
			     al_get_backbuffer(display));
}

static void
draw_alpha_overlay(ALLEGRO_COLOR color)
{
    int old_blender_op;
    int old_blender_src;
    int old_blender_dst;

    al_get_blender(&old_blender_op, &old_blender_src, &old_blender_dst);
    al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
    al_draw_filled_rectangle(0, 0, (float)play_area_width(),
			     (float)(display_height() - ROGUE_STATUS_HEIGHT),
			     color);
    al_set_blender(old_blender_op, old_blender_src, old_blender_dst);
}

static void
draw_damage_flash_overlay(void)
{
    double remaining;
    float alpha;

    if (!settings.damage_flash_enabled)
	return;

    remaining = damage_flash_until - al_get_time();
    if (remaining <= 0.0)
	return;

    alpha = (float)(remaining / ROGUE_DAMAGE_FLASH_SECONDS);
    if (alpha > 1.0f)
	alpha = 1.0f;
    draw_alpha_overlay(al_map_rgba_f(1.0f, 0.08f, 0.02f,
				     alpha * ROGUE_DAMAGE_FLASH_ALPHA));
}

static float
low_hp_pulse_alpha(void)
{
    double now;
    double cycles;
    double phase;
    double ramp;
    ROGUE_VARIANT_STATUS status;

    if (!settings.low_hp_pulse_enabled)
	return 0.0f;
    rogue_variant_status(&status);
    if (status.max_hit_points <= 0 || status.hp <= 0)
	return 0.0f;
    if (status.hp * 4 > status.max_hit_points)
	return 0.0f;

    now = al_get_time();
    cycles = (double)((int)(now / ROGUE_LOW_HP_PULSE_SECONDS));
    phase = now - cycles * ROGUE_LOW_HP_PULSE_SECONDS;
    if (phase < ROGUE_LOW_HP_PULSE_SECONDS * 0.5)
	ramp = phase / (ROGUE_LOW_HP_PULSE_SECONDS * 0.5);
    else
	ramp = 1.0 - ((phase - ROGUE_LOW_HP_PULSE_SECONDS * 0.5)
		      / (ROGUE_LOW_HP_PULSE_SECONDS * 0.5));

    return (float)(ROGUE_LOW_HP_PULSE_MIN_ALPHA
		   + ramp * (ROGUE_LOW_HP_PULSE_MAX_ALPHA
			     - ROGUE_LOW_HP_PULSE_MIN_ALPHA));
}

static void
draw_low_hp_pulse_overlay(void)
{
    float alpha;

    alpha = low_hp_pulse_alpha();
    if (alpha <= 0.0f)
	return;

    draw_alpha_overlay(al_map_rgba_f(0.75f, 0.0f, 0.0f, alpha));
}

static void
draw_visual_effect_overlays(void)
{
    draw_low_hp_pulse_overlay();
    draw_damage_flash_overlay();
}

static void
show_shader_settings_menu(void)
{
    char line[ROGUE_OVERLAY_LINE_LEN];
    char selected;
    bool done;

    done = FALSE;
    while (!done)
    {
	rogue_allegro_text_overlay_begin("Shader Settings");
	snprintf(line, sizeof(line), "a) Dungeon Gloom: %s",
		 settings.dungeon_gloom_enabled ? "On" : "Off");
	rogue_allegro_text_overlay_add(line);
	snprintf(line, sizeof(line), "b) Damage Flash: %s",
		 settings.damage_flash_enabled ? "On" : "Off");
	rogue_allegro_text_overlay_add(line);
	snprintf(line, sizeof(line), "c) Low HP Pulse: %s",
		 settings.low_hp_pulse_enabled ? "On" : "Off");
	rogue_allegro_text_overlay_add(line);
	snprintf(line, sizeof(line), "d) Pixel Sharpen: %s",
		 settings.pixel_sharpen_enabled ? "On" : "Off");
	rogue_allegro_text_overlay_add(line);
	snprintf(line, sizeof(line), "e) Posterize: %s",
		 settings.posterize_enabled ? "On" : "Off");
	rogue_allegro_text_overlay_add(line);
	snprintf(line, sizeof(line), "f) CRT Effect: %s",
		 crt_effect_label(settings.crt_effect_mode));
	rogue_allegro_text_overlay_add(line);
	rogue_allegro_text_overlay_add("");
	rogue_allegro_text_overlay_add("Shader effects are independent and visual-only.");

	selected = rogue_allegro_text_overlay_pick(
	    "Enter toggles, Esc closes");
	rogue_allegro_text_overlay_clear();

	switch (selected)
	{
	    case 'a':
	    case 'A':
		settings.dungeon_gloom_enabled =
		    !settings.dungeon_gloom_enabled;
		save_settings();
		break;
	    case 'b':
	    case 'B':
		settings.damage_flash_enabled =
		    !settings.damage_flash_enabled;
		save_settings();
		break;
	    case 'c':
	    case 'C':
		settings.low_hp_pulse_enabled =
		    !settings.low_hp_pulse_enabled;
		save_settings();
		break;
	    case 'd':
	    case 'D':
		settings.pixel_sharpen_enabled =
		    !settings.pixel_sharpen_enabled;
		save_settings();
		break;
	    case 'e':
	    case 'E':
		settings.posterize_enabled =
		    !settings.posterize_enabled;
		save_settings();
		break;
	    case 'f':
	    case 'F':
		cycle_crt_effect_mode();
		save_settings();
		break;
	    default:
		done = TRUE;
		break;
	}
	rogue_allegro_render();
    }
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

static char *
trim_manual_text(char *text)
{
    char *end;

    while (*text != '\0' && isspace((unsigned char)*text))
	text++;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1]))
	*--end = '\0';
    return text;
}

static char *
unquote_manual_text(char *text)
{
    int len;

    text = trim_manual_text(text);
    len = (int)strlen(text);
    if (len >= 2 && text[0] == '"' && text[len - 1] == '"')
    {
	text[len - 1] = '\0';
	return text + 1;
    }
    return text;
}

static void
reset_manual_chapters(void)
{
    manual_chapter_count = 0;
    manual_chapter_titles[0][0] = '\0';
}

static bool
manual_line_is_heading(const char *line)
{
    const char *p;

    if (line == NULL || !isdigit((unsigned char)line[0]))
	return FALSE;

    p = line;
    for (;;)
    {
	if (!isdigit((unsigned char)*p))
	    return FALSE;
	while (isdigit((unsigned char)*p))
	    p++;
	if (*p != '.')
	    return FALSE;
	p++;
	if (!isdigit((unsigned char)*p))
	    break;
    }

    return (bool)(isspace((unsigned char)*p) && p[1] != '\0');
}

static char *
manual_macro_text(char *line, const char *macro)
{
    int len;
    char *text;

    len = (int)strlen(macro);
    if (strncmp(line, macro, (size_t)len) != 0)
	return NULL;
    text = line + len;
    while (*text != '\0' && isspace((unsigned char)*text))
	text++;
    return text;
}

static char *
manual_heading_text(char *line)
{
    char *text;

    text = manual_macro_text(line, ".H");
    if (text == NULL)
	return NULL;
    while (*text != '\0' && isspace((unsigned char)*text))
	text++;
    while (isdigit((unsigned char)*text))
	text++;
    while (*text != '\0' && isspace((unsigned char)*text))
	text++;
    return text;
}

static void
manual_format_macro_text(char *source, char *out, int out_size)
{
    char *p;
    int written;
    bool copied;

    if (out_size <= 0)
	return;
    out[0] = '\0';
    if (source == NULL)
	return;

    p = trim_manual_text(source);
    written = 0;
    copied = FALSE;
    while (*p != '\0' && written < out_size - 1)
    {
	while (*p != '\0' && isspace((unsigned char)*p))
	    p++;
	if (*p == '"')
	{
	    p++;
	    if (copied && written < out_size - 1)
		out[written++] = ' ';
	    while (*p != '\0' && *p != '"' && written < out_size - 1)
		out[written++] = *p++;
	    if (*p == '"')
		p++;
	    copied = TRUE;
	}
	else
	{
	    if (copied && written < out_size - 1)
		out[written++] = ' ';
	    while (*p != '\0' && !isspace((unsigned char)*p)
		   && written < out_size - 1)
		out[written++] = *p++;
	    copied = TRUE;
	}
    }
    out[written] = '\0';
    clean_manual_text(out);
}

static void
record_manual_chapter(const char *title)
{
    if (title == NULL || *title == '\0')
	return;
    if (manual_chapter_count >= ROGUE_MANUAL_MAX_CHAPTERS)
	return;

    manual_chapter_lines[manual_chapter_count] = text_overlay_line_count;
    snprintf(manual_chapter_titles[manual_chapter_count],
	     sizeof(manual_chapter_titles[manual_chapter_count]), "%s", title);
    manual_chapter_count++;
}

static void
append_manual_blank_line(void)
{
    if (text_overlay_line_count <= 0)
	return;
    if (text_overlay_lines[text_overlay_line_count - 1][0] == '\0')
	return;
    rogue_allegro_text_overlay_add("");
}

static char
show_manual_chapter_picker(void)
{
    static char saved_lines[ROGUE_OVERLAY_MAX_LINES][ROGUE_OVERLAY_LINE_LEN];
    char saved_title[sizeof(text_overlay_title)];
    char saved_prompt[sizeof(text_overlay_prompt)];
    char line[ROGUE_OVERLAY_LINE_LEN];
    int saved_line_count;
    int saved_selected;
    int saved_scroll;
    int i;
    bool saved_active;
    bool saved_selectable;
    bool saved_fill_vertical;
    char selected;

    if (manual_chapter_count <= 0)
	return '\0';

    saved_line_count = text_overlay_line_count;
    saved_selected = text_overlay_selected;
    saved_scroll = text_overlay_scroll;
    saved_active = text_overlay_active;
    saved_selectable = text_overlay_selectable;
    saved_fill_vertical = text_overlay_fill_vertical;
    snprintf(saved_title, sizeof(saved_title), "%s", text_overlay_title);
    snprintf(saved_prompt, sizeof(saved_prompt), "%s", text_overlay_prompt);
    for (i = 0; i < saved_line_count; i++)
	snprintf(saved_lines[i], sizeof(saved_lines[i]), "%s",
		 text_overlay_lines[i]);

    rogue_allegro_text_overlay_begin("Chapters");
    for (i = 0; i < manual_chapter_count && i < 26; i++)
    {
	snprintf(line, sizeof(line), "%c) %s", 'a' + i,
		 manual_chapter_titles[i]);
	rogue_allegro_text_overlay_add(line);
    }
    drain_keyboard_events();
    selected = rogue_allegro_text_overlay_pick(
	"Enter jumps, Esc returns to guide");

    snprintf(text_overlay_title, sizeof(text_overlay_title), "%s",
	     saved_title);
    snprintf(text_overlay_prompt, sizeof(text_overlay_prompt), "%s",
	     saved_prompt);
    text_overlay_line_count = saved_line_count;
    text_overlay_selected = saved_selected;
    text_overlay_scroll = saved_scroll;
    text_overlay_active = saved_active;
    text_overlay_selectable = saved_selectable;
    text_overlay_fill_vertical = saved_fill_vertical;
    for (i = 0; i < saved_line_count; i++)
	snprintf(text_overlay_lines[i], sizeof(text_overlay_lines[i]), "%s",
		 saved_lines[i]);

    return selected;
}

static void
clean_manual_text(char *text)
{
    char *read;
    char *write;

    read = text;
    write = text;
    while (*read != '\0')
    {
	if (read[0] == '\\' && read[1] == '-')
	{
	    *write++ = '-';
	    read += 2;
	}
	else if (read[0] == '\\' && read[1] == 'f' && read[2] != '\0')
	    read += 3;
	else if (*read == '\f')
	    read++;
	else if (*read == '\r' || *read == '\n')
	    read++;
	else
	    *write++ = *read++;
    }
    *write = '\0';
}

static void
flush_manual_paragraph(char *paragraph)
{
    char *text;

    text = trim_manual_text(paragraph);
    if (*text != '\0')
	rogue_allegro_text_overlay_add(text);
    paragraph[0] = '\0';
}

static bool
manual_line_is_artifact(const char *line)
{
    const char *p;
    bool has_text;

    if (line == NULL)
	return TRUE;
    if (strstr(line, "USD:33-") != NULL)
	return TRUE;
    if (strstr(line, "A Guide to the Dungeons of Doom") != NULL
	&& strstr(line, "USD:33") != NULL)
	return TRUE;
    if (strstr(line, "UNIX is a trademark") != NULL)
	return TRUE;
    if (strstr(line, "As opposed to pseudo English") != NULL)
	return TRUE;
    if (strstr(line, "minimum screen size") != NULL)
	return TRUE;

    has_text = FALSE;
    for (p = line; *p != '\0'; p++)
    {
	if (*p != '_' && !isspace((unsigned char)*p))
	    return FALSE;
	if (*p == '_')
	    has_text = TRUE;
    }
    return has_text;
}

static void
append_manual_paragraph_text(char *paragraph, int paragraph_size,
			     const char *source)
{
    char text[512];
    int paragraph_len;
    char *trimmed;

    snprintf(text, sizeof(text), "%s", source);
    text[sizeof(text) - 1] = '\0';
    clean_manual_text(text);
    trimmed = trim_manual_text(text);
    if (*trimmed == '\0' || manual_line_is_artifact(trimmed))
	return;

    paragraph_len = (int)strlen(paragraph);
    if (paragraph_len > 0
	&& paragraph_len + (int)strlen(trimmed) + 2 >= paragraph_size)
    {
	flush_manual_paragraph(paragraph);
	paragraph_len = 0;
    }

    if (paragraph_len > 0 && paragraph[paragraph_len - 1] == '-')
    {
	paragraph[paragraph_len - 1] = '\0';
	strncat(paragraph, trimmed,
		(size_t)(paragraph_size - (int)strlen(paragraph) - 1));
    }
    else
    {
	if (paragraph_len > 0)
	    strncat(paragraph, " ",
		    (size_t)(paragraph_size - paragraph_len - 1));
	strncat(paragraph, trimmed,
		(size_t)(paragraph_size - (int)strlen(paragraph) - 1));
    }
}

static void
append_manual_literal_line(char *line)
{
    char formatted[512];
    char *text;

    if (strncmp(line, ".B ", 3) == 0
	|| strncmp(line, ".I ", 3) == 0)
    {
	manual_format_macro_text(line + 3, formatted, (int)sizeof(formatted));
	text = trim_manual_text(formatted);
	if (*text != '\0')
	    rogue_allegro_text_overlay_add(text);
	return;
    }

    clean_manual_text(line);
    text = line;
    while (*text != '\0' && (*text == '\r' || *text == '\n'))
	text++;
    if (*text == '\0')
    {
	append_manual_blank_line();
	return;
    }
    if (manual_line_is_artifact(text))
	return;
    rogue_allegro_text_overlay_add(text);
}

static void
append_manual_file_line(const char *source, char *paragraph,
			int paragraph_size, bool *literal_block)
{
    char line[512];
    char formatted[512];
    char *text;

    snprintf(line, sizeof(line), "%s", source);
    line[sizeof(line) - 1] = '\0';

    if (literal_block != NULL && *literal_block)
    {
	if (strncmp(line, ".DE", 3) == 0)
	{
	    *literal_block = FALSE;
	    append_manual_blank_line();
	    return;
	}
	append_manual_literal_line(line);
	return;
    }

    if (line[0] == '.' && line[1] == '\\' && line[2] == '"')
	return;
    if (line[0] == '.')
    {
	if (strncmp(line, ".SH ", 4) == 0)
	{
	    flush_manual_paragraph(paragraph);
	    manual_format_macro_text(line + 4, formatted,
				     (int)sizeof(formatted));
	    text = formatted;
	    append_manual_blank_line();
	    record_manual_chapter(text);
	    rogue_allegro_text_overlay_add(text);
	}
	else if ((text = manual_heading_text(line)) != NULL)
	{
	    flush_manual_paragraph(paragraph);
	    manual_format_macro_text(text, formatted,
				     (int)sizeof(formatted));
	    text = formatted;
	    append_manual_blank_line();
	    record_manual_chapter(text);
	    rogue_allegro_text_overlay_add(text);
	}
	else if (strncmp(line, ".PP", 3) == 0
		 || strncmp(line, ".P", 2) == 0
		 || strncmp(line, ".SP", 3) == 0
		 || strncmp(line, ".br", 3) == 0
		 || strncmp(line, ".bp", 3) == 0
		 || strncmp(line, ".FS", 3) == 0
		 || strncmp(line, ".FE", 3) == 0)
	{
	    flush_manual_paragraph(paragraph);
	    append_manual_blank_line();
	}
	else if (strncmp(line, ".DS", 3) == 0)
	{
	    flush_manual_paragraph(paragraph);
	    append_manual_blank_line();
	    if (literal_block != NULL)
		*literal_block = TRUE;
	}
	else if (strncmp(line, ".DE", 3) == 0)
	{
	    flush_manual_paragraph(paragraph);
	    append_manual_blank_line();
	}
	else if (strncmp(line, ".B ", 3) == 0
		 || strncmp(line, ".I ", 3) == 0)
	{
	    manual_format_macro_text(line + 3, formatted,
				     (int)sizeof(formatted));
	    text = formatted;
	    append_manual_paragraph_text(paragraph, paragraph_size, text);
	}
	return;
    }

    clean_manual_text(line);
    text = trim_manual_text(line);
    if (*text == '\0')
    {
	flush_manual_paragraph(paragraph);
	append_manual_blank_line();
	return;
    }
    if (manual_line_is_artifact(text))
	return;
    if (manual_line_is_heading(text))
    {
	flush_manual_paragraph(paragraph);
	append_manual_blank_line();
	record_manual_chapter(text);
	rogue_allegro_text_overlay_add(text);
	return;
    }
    append_manual_paragraph_text(paragraph, paragraph_size, text);
}

static FILE *
open_manual_file(const char *path, char *opened_path, int opened_path_size)
{
    FILE *file;
    char candidate[512];

    if (path == NULL || *path == '\0')
	return NULL;

    snprintf(candidate, sizeof(candidate), "%s", path);
    candidate[sizeof(candidate) - 1] = '\0';
    file = fopen(candidate, "r");
    if (file != NULL)
    {
	snprintf(opened_path, opened_path_size, "%s", candidate);
	return file;
    }

    snprintf(candidate, sizeof(candidate), "..\\%s", path);
    candidate[sizeof(candidate) - 1] = '\0';
    file = fopen(candidate, "r");
    if (file != NULL)
    {
	snprintf(opened_path, opened_path_size, "%s", candidate);
	return file;
    }

    return NULL;
}

static bool
manual_has_local_text(const ROGUE_VARIANT_MANUAL_REF *manual)
{
    return (bool)(manual != NULL
		  && manual->readable_path != NULL
		  && manual->readable_path[0] != '\0');
}

static bool
load_manual_text_overlay(const ROGUE_VARIANT_MANUAL_REF *manual)
{
    FILE *file;
    char opened_path[512];
    char line[512];
    char paragraph[1024];
    int loaded_lines;
    bool literal_block;

    reset_manual_chapters();
    if (!manual_has_local_text(manual))
	return FALSE;

    file = open_manual_file(manual->readable_path, opened_path,
			    (int)sizeof(opened_path));
    if (file == NULL)
	return FALSE;

    loaded_lines = 0;
    paragraph[0] = '\0';
    literal_block = FALSE;
    while (fgets(line, sizeof(line), file) != NULL
	   && text_overlay_line_count < ROGUE_OVERLAY_MAX_LINES - 2)
    {
	append_manual_file_line(line, paragraph, (int)sizeof(paragraph),
				&literal_block);
	loaded_lines++;
    }
    flush_manual_paragraph(paragraph);
    fclose(file);

    if (text_overlay_line_count >= ROGUE_OVERLAY_MAX_LINES - 2)
	rogue_allegro_text_overlay_add("[Guide truncated: overlay line limit reached]");

    return (bool)(loaded_lines > 0);
}

static void
show_manual_reader(const ROGUE_VARIANT_MANUAL_REF *manual)
{
    char line[ROGUE_OVERLAY_LINE_LEN];

    if (manual == NULL || manual->title == NULL)
	return;

    rogue_allegro_text_overlay_begin(manual->title);
    text_overlay_fill_vertical = TRUE;
    if (!load_manual_text_overlay(manual))
    {
	rogue_allegro_text_overlay_add("This reference is not bundled as readable in-game text.");
	rogue_allegro_text_overlay_add("");
	snprintf(line, sizeof(line), "Kind: %s", manual->kind);
	rogue_allegro_text_overlay_add(line);
	snprintf(line, sizeof(line), "Location: %s", manual->location);
	rogue_allegro_text_overlay_add(line);
	snprintf(line, sizeof(line), "Rights: %s", manual->rights);
	rogue_allegro_text_overlay_add(line);
	snprintf(line, sizeof(line), "Notes: %s", manual->notes);
	rogue_allegro_text_overlay_add(line);
    }
    if (manual_chapter_count > 0)
	rogue_allegro_text_overlay_show(
	    "Arrows/Page scroll, C Chapters, Space closes");
    else
	rogue_allegro_text_overlay_show("Arrows/Page scroll, Space closes");
    rogue_allegro_text_overlay_clear();
}

static void
show_manuals_menu_for_variant(const ROGUE_VARIANT_INFO *current)
{
    char line[ROGUE_OVERLAY_LINE_LEN];
    char selected;
    int i;
    int index;

    if (current == NULL)
	return;

    for (;;)
    {
	rogue_allegro_text_overlay_begin("Manuals & Guides");
	snprintf(line, sizeof(line), "%s references", current->name);
	rogue_allegro_text_overlay_add(line);
	rogue_allegro_text_overlay_add("");

	for (i = 0; i < ROGUE_VARIANT_MAX_MANUALS
	    && current->manuals[i].title != NULL; i++)
	{
	    snprintf(line, sizeof(line), "%c) %s - %s%s",
		     'a' + i, current->manuals[i].title,
		     current->manuals[i].kind,
		     manual_has_local_text(&current->manuals[i])
		     ? "" : " (link only)");
	    rogue_allegro_text_overlay_add(line);
	}

	selected = rogue_allegro_text_overlay_pick(
	    "Select a guide, Esc closes");
	rogue_allegro_text_overlay_clear();
	if (selected == ESCAPE || selected == 'Q' || selected == '\0')
	    break;
	index = selected - 'a';
	if (index < 0 || index >= ROGUE_VARIANT_MAX_MANUALS
	    || current->manuals[index].title == NULL)
	    break;
	show_manual_reader(&current->manuals[index]);
    }
}

static void
show_manuals_menu(void)
{
    const ROGUE_VARIANT_INFO *current;

    current = rogue_variant_current();
    show_manuals_menu_for_variant(current);
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
	rogue_allegro_text_overlay_add("e) Shader Settings...");
	snprintf(line, sizeof(line), "f) Wall Thickness: %s",
		 wall_thickness_name());
	rogue_allegro_text_overlay_add(line);
	snprintf(line, sizeof(line), "g) Enemy Health Overlay: %s",
		 settings.enemy_health_overlay_enabled ? "On" : "Off");
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
		show_shader_settings_menu();
		break;
	    case 'f':
	    case 'F':
		cycle_wall_thickness();
		save_settings();
		break;
	    case 'g':
	    case 'G':
		settings.enemy_health_overlay_enabled =
		    !settings.enemy_health_overlay_enabled;
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
	case ROGUE_THICK_WALL_THICKNESS:
	    edge = size / 4;
	    break;
	case ROGUE_FULL_WALL_THICKNESS:
	    return size;
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

    if (atlas_index < 0 || atlas == NULL
	|| (cell->layer == ROGUE_TILE_ACTOR
	    && !atlas_tile_is_visible(atlas_index)))
    {
	if (cell->has_underlay)
	    draw_glyph_foreground_cell(screen_x, screen_y, cell);
	else
	{
	    fallback = al_map_rgb(100, 20, 60);
	    al_draw_filled_rectangle(dx, dy, dx + ROGUE_TILE_DRAW_SIZE,
				     dy + ROGUE_TILE_DRAW_SIZE, fallback);
	    draw_glyph_cell(screen_x, screen_y, cell);
	}
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
    if (atlas_index < 0 || atlas == NULL || !atlas_tile_is_visible(atlas_index))
    {
	draw_glyph_foreground_cell(screen_x, screen_y, cell);
	return;
    }

    dx = render_origin_x + screen_x * ROGUE_TILE_DRAW_SIZE;
    dy = render_origin_y + screen_y * ROGUE_TILE_DRAW_SIZE;
    draw_atlas_tile(atlas_index, dx, dy);
}

static void
draw_enemy_health_overlay_cell(int screen_x, int screen_y,
			       ROGUE_TILE_CELL *cell)
{
    THING *monster;
    ALLEGRO_FONT *label_font;
    ALLEGRO_COLOR back_color;
    ALLEGRO_COLOR border_color;
    ALLEGRO_COLOR fill_color;
    ALLEGRO_COLOR text_color;
    char label[64];
    int hp;
    int maxhp;
    int dx;
    int dy;
    int panel_w;
    int panel_h;
    int panel_x;
    int panel_y;
    int bar_x;
    int bar_y;
    int bar_w;
    int bar_h;
    int fill_w;
    int text_w;
    int play_w;

    if (!settings.enemy_health_overlay_enabled)
	return;
    if (cell == NULL || cell->layer != ROGUE_TILE_ACTOR || !cell->visible)
	return;
    if (cell->glyph < 'A' || cell->glyph > 'Z')
	return;

    monster = moat(cell->y, cell->x);
    if (monster == NULL || !see_monst(monster))
	return;
    if (monster->t_disguise != monster->t_type)
	return;

    hp = monster->t_stats.s_hpt;
    maxhp = monster->t_stats.s_maxhp;
    if (maxhp <= 0)
	return;
    hp = clamp_int(hp, 0, maxhp);

    label_font = (small_font != NULL) ? small_font : font;
    snprintf(label, sizeof(label), "HP %d/%d  L%d  Arm %d", hp, maxhp,
	     monster->t_stats.s_lvl, monster->t_stats.s_arm);
    text_w = al_get_text_width(label_font, label);
    bar_w = ROGUE_TILE_DRAW_SIZE;
    if (bar_w < 28)
	bar_w = 28;
    panel_w = text_w + 10;
    if (panel_w < bar_w + 8)
	panel_w = bar_w + 8;
    panel_h = ROGUE_SMALL_FONT_SIZE + 12;

    dx = render_origin_x + screen_x * ROGUE_TILE_DRAW_SIZE;
    dy = render_origin_y + screen_y * ROGUE_TILE_DRAW_SIZE;
    panel_x = dx + ROGUE_TILE_DRAW_SIZE / 2 - panel_w / 2;
    panel_y = dy - panel_h - 2;
    play_w = play_area_width();
    if (panel_x < 2)
	panel_x = 2;
    if (panel_x + panel_w > play_w - 2)
	panel_x = play_w - panel_w - 2;
    if (panel_x < 2)
	panel_x = 2;
    if (panel_y < 2)
	panel_y = dy + ROGUE_TILE_DRAW_SIZE + 2;

    bar_x = panel_x + (panel_w - bar_w) / 2;
    bar_y = panel_y + 3;
    bar_h = 4;
    fill_w = (bar_w * hp) / maxhp;

    back_color = al_map_rgba(8, 10, 14, 210);
    border_color = al_map_rgba(198, 214, 235, 185);
    fill_color = al_map_rgb(100, 220, 120);
    if (hp * 4 <= maxhp)
	fill_color = al_map_rgb(230, 72, 72);
    else if (hp * 2 <= maxhp)
	fill_color = al_map_rgb(232, 190, 76);
    text_color = al_map_rgb(238, 242, 236);

    al_draw_filled_rectangle(panel_x, panel_y, panel_x + panel_w,
			     panel_y + panel_h, back_color);
    al_draw_rectangle(panel_x + 0.5f, panel_y + 0.5f,
		      panel_x + panel_w - 0.5f,
		      panel_y + panel_h - 0.5f, border_color, 1.0f);
    al_draw_filled_rectangle(bar_x, bar_y, bar_x + bar_w,
			     bar_y + bar_h, al_map_rgb(42, 50, 52));
    al_draw_filled_rectangle(bar_x, bar_y, bar_x + fill_w,
			     bar_y + bar_h, fill_color);
    al_draw_text(label_font, text_color, panel_x + panel_w / 2,
		 panel_y + 7, ALLEGRO_ALIGN_CENTRE, label);
}

static void
draw_enemy_health_overlays(
    ROGUE_TILE_CELL view[ROGUE_MAX_VIEW_ROWS][ROGUE_MAX_VIEW_COLS],
    int rows, int cols)
{
    int screen_y;
    int screen_x;

    if (!settings.enemy_health_overlay_enabled)
	return;

    for (screen_y = 0; screen_y < rows; screen_y++)
	for (screen_x = 0; screen_x < cols; screen_x++)
	    draw_enemy_health_overlay_cell(screen_x, screen_y,
					   &view[screen_y][screen_x]);
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
			  const ROGUE_VARIANT_STATUS *status,
			  const char *hungry_name, const char *fallback)
{
    char level_text[24];
    char gold_text[24];
    char hp_text[32];
    char str_text[24];
    char arm_text[24];
    char exp_text[32];
    char vol_text[24];
    int total_width;
    int x;
    ALLEGRO_COLOR hp_color;

    snprintf(level_text, sizeof(level_text), "%d", status->dungeon_level);
    snprintf(gold_text, sizeof(gold_text), "%d", status->gold);
    snprintf(hp_text, sizeof(hp_text), "%d(%d)", status->hp,
	     status->max_hit_points);
    snprintf(str_text, sizeof(str_text), "%u", status->strength);
    snprintf(arm_text, sizeof(arm_text), "%d", armor);
    snprintf(exp_text, sizeof(exp_text), "%d/%ld", status->exp_level,
	     status->exp_points);
    snprintf(vol_text, sizeof(vol_text), "%d%%", status->volume_percent);

    total_width = status_piece_width("Level ", level_text)
		  + status_piece_width("Gold ", gold_text)
		  + status_piece_width("HP ", hp_text)
		  + status_piece_width("Arm ", arm_text)
		  + status_piece_width("Exp ", exp_text);
    if (status->has_extended_stats)
	total_width += status_piece_width("Vol ", vol_text);
    else
	total_width += status_piece_width("ST ", str_text);
    if (hungry_name != NULL && hungry_name[0] != '\0')
	total_width += status_piece_width("", hungry_name);

    if (total_width > width - 24)
    {
	al_draw_text(font, al_map_rgb(230, 230, 220), width / 2, y,
		     ALLEGRO_ALIGN_CENTRE, fallback);
	return;
    }

    hp_color = al_map_rgb(116, 220, 148);
    if (status->hp * 4 <= status->max_hit_points)
	hp_color = al_map_rgb(238, 82, 82);
    else if (status->hp * 2 <= status->max_hit_points)
	hp_color = al_map_rgb(238, 205, 112);

    x = (width - total_width) / 2;
    x = draw_status_piece("Level ", level_text, x, y,
			  al_map_rgb(126, 176, 238));
    x = draw_status_piece("Gold ", gold_text, x, y,
			  al_map_rgb(238, 205, 112));
    x = draw_status_piece("HP ", hp_text, x, y, hp_color);
    if (!status->has_extended_stats)
	x = draw_status_piece("ST ", str_text, x, y,
			      al_map_rgb(228, 154, 83));
    x = draw_status_piece("Arm ", arm_text, x, y,
			  al_map_rgb(174, 190, 210));
    x = draw_status_piece("Exp ", exp_text, x, y,
			  al_map_rgb(174, 154, 238));
    if (status->has_extended_stats)
	x = draw_status_piece("Vol ", vol_text, x, y,
			      al_map_rgb(130, 205, 205));
    if (hungry_name != NULL && hungry_name[0] != '\0')
	draw_status_piece("", hungry_name, x, y, al_map_rgb(228, 154, 83));
}

static void
format_ability_pair(char *out, size_t out_size, unsigned int effective,
		    unsigned int base)
{
    snprintf(out, out_size, "%u(%u)", effective, base);
}

static void
draw_status(void)
{
    int y;
    int w;
    int h;
    int armor;
    char line[256];
    char second_status_line[256];
    int line_height;
    int first_line_y;
    int second_line_y;
    int footer_line_y;
    static char *state_name[] = { "", "Hungry", "Weak", "Faint" };
    ROGUE_VARIANT_STATUS status;

    rogue_variant_status(&status);
    w = play_area_width();
    h = display_height();
    y = h - ROGUE_STATUS_HEIGHT;
    if (y < 0)
	y = 0;
    armor = status.armor;
    line_height = al_get_font_line_height(font);
    first_line_y = y + 8;
    second_line_y = first_line_y + line_height + 6;
    footer_line_y = second_line_y;
    second_status_line[0] = '\0';

    al_draw_filled_rectangle(0, y, w, h,
			     al_map_rgb(5, 5, 8));
    al_draw_line(0, y, w, y, al_map_rgb(80, 80, 90), 1);

    if (status.has_extended_stats)
    {
	char str_text[24];
	char dex_text[24];
	char wis_text[24];
	char con_text[24];

	format_ability_pair(str_text, sizeof(str_text), status.strength,
			    status.strength_base);
	format_ability_pair(dex_text, sizeof(dex_text), status.dexterity,
			    status.dexterity_base);
	format_ability_pair(wis_text, sizeof(wis_text), status.wisdom,
			    status.wisdom_base);
	format_ability_pair(con_text, sizeof(con_text), status.constitution,
			    status.constitution_base);
	snprintf(line, sizeof(line),
		 "Level:%d  Gold:%d  HP:%d(%d)  Arm:%d  Exp:%d/%ld  Vol:%d%% %s",
		 status.dungeon_level, status.gold, status.hp,
		 status.max_hit_points, armor, status.exp_level,
		 status.exp_points, status.volume_percent,
		 state_name[status.hungry_state]);
	snprintf(second_status_line, sizeof(second_status_line),
		 "Str:%s  Dex:%s  Wis:%s  Con:%s  Carry:%d(%d)",
		 str_text, dex_text, wis_text, con_text,
		 status.carry_weight, status.carry_capacity);
	footer_line_y = second_line_y + line_height + 4;
    }
    else
	snprintf(line, sizeof(line),
		 "Level:%d  Gold:%d  HP:%d(%d)  ST:%u  Arm:%d  Exp:%d/%ld %s",
		 status.dungeon_level, status.gold, status.hp,
		 status.max_hit_points,
		 status.strength, armor, status.exp_level, status.exp_points,
		 state_name[status.hungry_state]);

    if (settings.stylized_bottom_bar_enabled)
	draw_stylized_status_line(w, first_line_y, armor,
				  &status, state_name[status.hungry_state],
				  line);
    else
	al_draw_text(font, al_map_rgb(230, 230, 220), w / 2,
		     first_line_y, ALLEGRO_ALIGN_CENTRE, line);
    if (second_status_line[0] != '\0')
	al_draw_text(font, al_map_rgb(210, 220, 225), w / 2,
		     second_line_y, ALLEGRO_ALIGN_CENTRE,
		     second_status_line);
    if (!settings.side_panel_log_enabled)
	al_draw_text(font, al_map_rgb(180, 200, 255), 8, footer_line_y,
		     0, status.message);
    if (prompt_active)
	al_draw_text(font, al_map_rgb(245, 226, 170), w - 8,
		     footer_line_y, ALLEGRO_ALIGN_RIGHT, prompt_text);
    else
	al_draw_text(font, al_map_rgb(160, 160, 160), w - 8,
		     footer_line_y, ALLEGRO_ALIGN_RIGHT,
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
    return rogue_variant_cell_walkable(y, x);
}

static bool
find_blood_splat_cell(int *out_y, int *out_x)
{
    int attempt;
    int radius;
    int y;
    int x;
    int hero_y;
    int hero_x;

    rogue_variant_hero_position(&hero_y, &hero_x);

    for (attempt = 0; attempt < 16; attempt++)
    {
	radius = (attempt < 8) ? 1 : 2;
	y = hero_y + blood_random(radius * 2 + 1) - radius;
	x = hero_x + blood_random(radius * 2 + 1) - radius;
	if (!blood_tile_is_walkable(y, x))
	    continue;

	*out_y = y;
	*out_x = x;
	return TRUE;
    }

    if (blood_tile_is_walkable(hero_y, hero_x))
    {
	*out_y = hero_y;
	*out_x = hero_x;
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
    splat->level = rogue_variant_level_number();
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
	if (splat->level != rogue_variant_level_number())
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
next_space_wrap_len(const char *text, int start, int max_chars)
{
    int len;
    int end;
    int split;

    len = (int) strlen(text);
    while (start < len && isspace((unsigned char) text[start]))
	start++;
    if (start >= len)
	return 0;

    end = start + max_chars;
    if (end > len)
	end = len;
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

static int
text_overlay_visible_line_capacity(void)
{
    int line_height;
    int max_lines;

    line_height = al_get_font_line_height(font);
    if (line_height < 1)
	line_height = ROGUE_SMALL_FONT_SIZE;

    if (text_overlay_fill_vertical)
    {
	max_lines = (display_height() - 160) / (line_height + 2);
	if (max_lines < 4)
	    max_lines = 4;
	return max_lines;
    }

    return 14;
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
    if (visible_lines > text_overlay_visible_line_capacity())
	visible_lines = text_overlay_visible_line_capacity();
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
    if (text_overlay_fill_vertical)
    {
	h = window_h - 48;
	if (h < 220)
	    h = window_h - 16;
    }
    else
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
    if (shader_smoke_mode)
    {
	settings.dungeon_gloom_enabled = TRUE;
	settings.pixel_sharpen_enabled = TRUE;
	settings.posterize_enabled = TRUE;
    }

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
    al_set_new_display_flags(ALLEGRO_RESIZABLE | ALLEGRO_OPENGL);
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
    small_font = load_small_ui_font();
    detail_font = load_detail_ui_font();

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
    int old_blender_op;
    int old_blender_src;
    int old_blender_dst;
    bool render_to_scene;
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
	    rogue_variant_describe_cell(world_y, world_x,
					&view[screen_y][screen_x]);
	}

    render_to_scene = (bool)(scene_effects_need_bitmap()
			     && ensure_scene_bitmap());
    if (render_to_scene)
	al_set_target_bitmap(scene_bitmap);
    else
	al_set_target_backbuffer(display);

    al_clear_to_color(al_map_rgb(0, 0, 0));
    al_get_blender(&old_blender_op, &old_blender_src, &old_blender_dst);
    al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
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
    al_set_blender(old_blender_op, old_blender_src, old_blender_dst);
    draw_enemy_health_overlays(view, rows, cols);
    if (render_to_scene)
	save_shader_smoke_bitmap("rogue_scene_before_shader.png",
				 scene_bitmap);
    if (render_to_scene)
	copy_scene_to_source_bitmap();
    if (render_to_scene)
	draw_scene_with_gloom_shader();
    else
	al_set_target_backbuffer(display);
    draw_visual_effect_overlays();
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
	    if (event.keyboard.keycode == ALLEGRO_KEY_F1)
	    {
		show_manuals_menu();
		suppress_key_char_keycode = event.keyboard.keycode;
		rogue_allegro_render();
		continue;
	    }
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
    {
	pending_damage_taken += taken;
	damage_flash_until = al_get_time() + ROGUE_DAMAGE_FLASH_SECONDS;
    }
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
    text_overlay_fill_vertical = FALSE;
}

static void
append_text_overlay_line(const char *line)
{
    if (text_overlay_line_count >= ROGUE_OVERLAY_MAX_LINES)
	return;

    snprintf(text_overlay_lines[text_overlay_line_count],
	     sizeof(text_overlay_lines[text_overlay_line_count]), "%s", line);
    text_overlay_line_count++;
}

void
rogue_allegro_text_overlay_add(const char *line)
{
    char segment[ROGUE_OVERLAY_LINE_LEN];
    int start;
    int len;
    int text_len;

    if (line == NULL)
	line = "";
    if (text_overlay_line_count >= ROGUE_OVERLAY_MAX_LINES)
	return;

    if (*line == '\0'
	|| rogue_picker_line_key(line) != '\0'
	|| (int) strlen(line) <= TEXT_OVERLAY_WRAP_CHARS)
    {
	append_text_overlay_line(line);
	return;
    }

    start = 0;
    text_len = (int) strlen(line);
    while (start < text_len && text_overlay_line_count < ROGUE_OVERLAY_MAX_LINES)
    {
	while (start < text_len && isspace((unsigned char) line[start]))
	    start++;
	if (start >= text_len)
	    break;

	len = next_space_wrap_len(line, start, TEXT_OVERLAY_WRAP_CHARS);
	if (len <= 0)
	    break;
	if (len >= (int) sizeof(segment))
	    len = (int) sizeof(segment) - 1;
	memcpy(segment, line + start, (size_t) len);
	segment[len] = '\0';
	append_text_overlay_line(segment);
	start += len;
    }
}

char
rogue_allegro_text_overlay_show(const char *prompt)
{
    ALLEGRO_EVENT event;
    char ch;
    char chapter_choice;
    int visible_lines;
    int page;
    int overlay_scroll_direction;
    int overlay_scroll_keycode;
    int chapter_index;
    double overlay_scroll_next_time;
    double now;
    double wait_time;
    bool event_received;

    if (prompt != NULL && *prompt != '\0')
	snprintf(text_overlay_prompt, sizeof(text_overlay_prompt), "%s", prompt);
    text_overlay_selectable = FALSE;
    text_overlay_selected = -1;
    text_overlay_scroll = 0;
    text_overlay_active = TRUE;
    overlay_scroll_direction = 0;
    overlay_scroll_keycode = 0;
    overlay_scroll_next_time = 0.0;
    rogue_allegro_render();

    ch = '\0';
    while (ch == '\0')
    {
	visible_lines = text_overlay_line_count;
	if (visible_lines > text_overlay_visible_line_capacity())
	    visible_lines = text_overlay_visible_line_capacity();
	if (visible_lines < 1)
	    visible_lines = 1;
	page = visible_lines - 1;
	if (page < 1)
	    page = 1;

	if (overlay_scroll_direction != 0)
	{
	    now = al_get_time();
	    if (now >= overlay_scroll_next_time)
	    {
		text_overlay_scroll += overlay_scroll_direction;
		overlay_scroll_next_time = now
		    + ROGUE_OVERLAY_SCROLL_RATE_SECONDS;
		rogue_allegro_render();
		continue;
	    }
	    wait_time = overlay_scroll_next_time - now;
	    if (wait_time < 0.0)
		wait_time = 0.0;
	    event_received = al_wait_for_event_timed(
		queue, &event, (float)wait_time);
	    if (!event_received)
		continue;
	}
	else
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
	if (event.type == ALLEGRO_EVENT_KEY_UP)
	{
	    if (event.keyboard.keycode == overlay_scroll_keycode)
	    {
		overlay_scroll_direction = 0;
		overlay_scroll_keycode = 0;
	    }
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
		overlay_scroll_direction = -1;
		overlay_scroll_keycode = event.keyboard.keycode;
		overlay_scroll_next_time = al_get_time()
		    + ROGUE_OVERLAY_SCROLL_DELAY_SECONDS;
		break;
	    case ALLEGRO_KEY_DOWN:
	    case ALLEGRO_KEY_PAD_2:
		text_overlay_scroll++;
		overlay_scroll_direction = 1;
		overlay_scroll_keycode = event.keyboard.keycode;
		overlay_scroll_next_time = al_get_time()
		    + ROGUE_OVERLAY_SCROLL_DELAY_SECONDS;
		break;
	    case ALLEGRO_KEY_PGUP:
	    case ALLEGRO_KEY_PAD_9:
		text_overlay_scroll -= page;
		overlay_scroll_direction = 0;
		overlay_scroll_keycode = 0;
		break;
	    case ALLEGRO_KEY_PGDN:
	    case ALLEGRO_KEY_PAD_3:
		text_overlay_scroll += page;
		overlay_scroll_direction = 0;
		overlay_scroll_keycode = 0;
		break;
	    case ALLEGRO_KEY_HOME:
		text_overlay_scroll = 0;
		overlay_scroll_direction = 0;
		overlay_scroll_keycode = 0;
		break;
	    case ALLEGRO_KEY_END:
		text_overlay_scroll = text_overlay_line_count - visible_lines;
		overlay_scroll_direction = 0;
		overlay_scroll_keycode = 0;
		break;
	    case ALLEGRO_KEY_C:
		chapter_choice = show_manual_chapter_picker();
		chapter_index = tolower((unsigned char)chapter_choice) - 'a';
		if (chapter_index >= 0
		    && chapter_index < manual_chapter_count)
		    text_overlay_scroll = manual_chapter_lines[chapter_index];
		overlay_scroll_direction = 0;
		overlay_scroll_keycode = 0;
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
    int visible_lines;
    int page;
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
	visible_lines = text_overlay_line_count;
	if (visible_lines > text_overlay_visible_line_capacity())
	    visible_lines = text_overlay_visible_line_capacity();
	if (visible_lines < 1)
	    visible_lines = 1;
	page = visible_lines - 1;
	if (page < 1)
	    page = 1;

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
		case ALLEGRO_KEY_PGUP:
		case ALLEGRO_KEY_PAD_9:
		    text_overlay_selected = rogue_picker_move_selection(
			text_overlay_selected, text_overlay_line_count, -page);
		    break;
		case ALLEGRO_KEY_PGDN:
		case ALLEGRO_KEY_PAD_3:
		    text_overlay_selected = rogue_picker_move_selection(
			text_overlay_selected, text_overlay_line_count, page);
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
    text_overlay_fill_vertical = FALSE;
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

    death_overlay_active = FALSE;
}

void
rogue_allegro_shutdown(void)
{
    if (small_font != NULL && small_font != font)
	al_destroy_font(small_font);
    if (detail_font != NULL && detail_font != font)
	al_destroy_font(detail_font);
    if (font != NULL)
	al_destroy_font(font);
    if (gloom_shader != NULL)
	al_destroy_shader(gloom_shader);
    if (postprocess_shader != NULL)
	al_destroy_shader(postprocess_shader);
    if (scene_bitmap != NULL)
	al_destroy_bitmap(scene_bitmap);
    if (scene_source_bitmap != NULL)
	al_destroy_bitmap(scene_source_bitmap);
    if (atlas != NULL)
	al_destroy_bitmap(atlas);
    clear_atlas_visibility_cache();
    if (queue != NULL)
	al_destroy_event_queue(queue);
    if (display != NULL)
	al_destroy_display(display);

    font = NULL;
    detail_font = NULL;
    small_font = NULL;
    gloom_shader = NULL;
    postprocess_shader = NULL;
    scene_bitmap = NULL;
    scene_source_bitmap = NULL;
    atlas = NULL;
    queue = NULL;
    display = NULL;
    started = FALSE;
    held_movement_keycode = 0;
    held_movement = '\0';
    held_movement_next_time = 0.0;
    damage_flash_until = 0.0;
    gloom_shader_failed = FALSE;
    postprocess_shader_failed = FALSE;
    shader_smoke_mode = FALSE;
    scene_bitmap_width = 0;
    scene_bitmap_height = 0;
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
