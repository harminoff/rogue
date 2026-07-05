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

typedef struct rogue_mobile_layout {
    ROGUE_MOBILE_BUTTON buttons[ROGUE_MOBILE_BUTTON_COUNT];
    int button_count;
} ROGUE_MOBILE_LAYOUT;

void rogue_mobile_layout_build(ROGUE_MOBILE_LAYOUT *layout, int x, int y, int w, int h);
char rogue_mobile_command_at(const ROGUE_MOBILE_LAYOUT *layout, int x, int y);

#endif
