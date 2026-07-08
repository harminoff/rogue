#ifndef MOBILE_CONTROLS_H
#define MOBILE_CONTROLS_H

typedef struct rogue_mobile_rect {
    int x;
    int y;
    int w;
    int h;
} ROGUE_MOBILE_RECT;

typedef struct rogue_mobile_button {
    ROGUE_MOBILE_RECT rect;
    char command;
    const char *label;
} ROGUE_MOBILE_BUTTON;

#define ROGUE_MOBILE_BUTTON_COUNT 9
#define ROGUE_MOBILE_ACTION_BUTTON_COUNT 30
#define ROGUE_MOBILE_COMMAND_MANUALS ((char) 0x1f)
#define ROGUE_MOBILE_COMMAND_OPTIONS ((char) 0x1e)
#define ROGUE_MOBILE_COMMAND_CLOSE ((char) 0x1d)

typedef struct rogue_mobile_layout {
    ROGUE_MOBILE_BUTTON buttons[ROGUE_MOBILE_BUTTON_COUNT];
    int button_count;
    ROGUE_MOBILE_BUTTON actions[ROGUE_MOBILE_ACTION_BUTTON_COUNT];
    int action_count;
    ROGUE_MOBILE_RECT action_viewport;
    int action_content_height;
    ROGUE_MOBILE_RECT zoom_slider;
} ROGUE_MOBILE_LAYOUT;

typedef struct rogue_mobile_action_context {
    int on_stairs;
    int on_object;
    int in_trading_post;
    int on_magic_pool;
} ROGUE_MOBILE_ACTION_CONTEXT;

void rogue_mobile_layout_build(ROGUE_MOBILE_LAYOUT *layout, int x, int y, int w, int h);
char rogue_mobile_command_at(const ROGUE_MOBILE_LAYOUT *layout, int x, int y);
int rogue_mobile_movement_index_at(const ROGUE_MOBILE_LAYOUT *layout, int x, int y);
void rogue_mobile_action_bar_build(ROGUE_MOBILE_LAYOUT *layout, int x, int y, int w, int h, int scroll_y);
void rogue_mobile_action_bar_build_for_variant(ROGUE_MOBILE_LAYOUT *layout, int x, int y, int w, int h, int scroll_y, const char *variant_id);
void rogue_mobile_action_bar_build_with_context(ROGUE_MOBILE_LAYOUT *layout, int x, int y, int w, int h, int scroll_y, const char *variant_id, const ROGUE_MOBILE_ACTION_CONTEXT *context);
char rogue_mobile_action_at(const ROGUE_MOBILE_LAYOUT *layout, int x, int y);
int rogue_mobile_action_index_at(const ROGUE_MOBILE_LAYOUT *layout, int x, int y);
int rogue_mobile_action_scroll_max(const ROGUE_MOBILE_LAYOUT *layout);
int rogue_mobile_action_scroll_clamp(const ROGUE_MOBILE_LAYOUT *layout, int scroll_y);
void rogue_mobile_zoom_slider_build(ROGUE_MOBILE_LAYOUT *layout, int x, int y, int w, int h);
int rogue_mobile_zoom_at(const ROGUE_MOBILE_LAYOUT *layout, int x, int y, int min_value, int max_value);

#endif
