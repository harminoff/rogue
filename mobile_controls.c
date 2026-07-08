#include "mobile_controls.h"
#include <string.h>

#define ROGUE_MOBILE_ZOOM_STEP 4
#define ROGUE_MOBILE_ACTION_COLS 3
#define ROGUE_MOBILE_ACTION_VISIBLE_ROWS 3
#define ROGUE_MOBILE_ACTION_ALWAYS 0
#define ROGUE_MOBILE_ACTION_STAIRS 1
#define ROGUE_MOBILE_ACTION_OBJECT 2
#define ROGUE_MOBILE_ACTION_TRADE 4
#define ROGUE_MOBILE_ACTION_POOL 8

typedef struct rogue_mobile_action_def {
    char command;
    const char *label;
    int flags;
} ROGUE_MOBILE_ACTION_DEF;

static int
rect_contains(const ROGUE_MOBILE_RECT *rect, int x, int y)
{
    return rect != 0 &&
	x >= rect->x && x < rect->x + rect->w &&
	y >= rect->y && y < rect->y + rect->h;
}

void
rogue_mobile_layout_build(ROGUE_MOBILE_LAYOUT *layout, int x, int y, int w, int h)
{
    static const char commands[ROGUE_MOBILE_BUTTON_COUNT] = {
	'y', 'k', 'u',
	'h', '.', 'l',
	'b', 'j', 'n'
    };
    static const char *labels[ROGUE_MOBILE_BUTTON_COUNT] = {
	"Up Left", "Up", "Up Right",
	"Left", "Wait/Use", "Right",
	"Down Left", "Down", "Down Right"
    };
    int col_widths[3];
    int row_heights[3];
    int row, col, index;

    if (layout == 0)
    {
	return;
    }

    col_widths[0] = w / 3;
    col_widths[1] = w / 3;
    col_widths[2] = w - col_widths[0] - col_widths[1];
    row_heights[0] = h / 3;
    row_heights[1] = h / 3;
    row_heights[2] = h - row_heights[0] - row_heights[1];

    layout->button_count = ROGUE_MOBILE_BUTTON_COUNT;
    index = 0;
    for (row = 0; row < 3; row++)
    {
	for (col = 0; col < 3; col++)
	{
	    layout->buttons[index].rect.x = x + (col_widths[0] * col);
	    layout->buttons[index].rect.y = y + (row_heights[0] * row);
	    layout->buttons[index].rect.w = col_widths[col];
	    layout->buttons[index].rect.h = row_heights[row];
	    layout->buttons[index].command = commands[index];
	    layout->buttons[index].label = labels[index];
	    index++;
	}
    }
}

char
rogue_mobile_command_at(const ROGUE_MOBILE_LAYOUT *layout, int x, int y)
{
    int index;

    index = rogue_mobile_movement_index_at(layout, x, y);
    if (index < 0)
    {
	return '\0';
    }
    return layout->buttons[index].command;
}

int
rogue_mobile_movement_index_at(const ROGUE_MOBILE_LAYOUT *layout, int x, int y)
{
    const ROGUE_MOBILE_RECT *rect;
    int count;
    int i;

    if (layout == 0)
    {
	return -1;
    }

    count = layout->button_count;
    if (count < 0)
    {
	count = 0;
    }
    if (count > ROGUE_MOBILE_BUTTON_COUNT)
    {
	count = ROGUE_MOBILE_BUTTON_COUNT;
    }

    for (i = 0; i < count; i++)
    {
	rect = &layout->buttons[i].rect;

	if (rect_contains(rect, x, y))
	{
	    return i;
	}
    }

    return -1;
}

void
rogue_mobile_action_bar_build(ROGUE_MOBILE_LAYOUT *layout, int x, int y, int w,
			      int h, int scroll_y)
{
    rogue_mobile_action_bar_build_for_variant(layout, x, y, w, h, scroll_y, 0);
}

void
rogue_mobile_action_bar_build_for_variant(ROGUE_MOBILE_LAYOUT *layout, int x,
					  int y, int w, int h, int scroll_y,
					  const char *variant_id)
{
    rogue_mobile_action_bar_build_with_context(layout, x, y, w, h, scroll_y,
					       variant_id, NULL);
}

static int
action_visible_for_context(const ROGUE_MOBILE_ACTION_DEF *action,
			   const ROGUE_MOBILE_ACTION_CONTEXT *context)
{
    if (action == NULL)
	return 0;
    if (context == NULL || action->flags == ROGUE_MOBILE_ACTION_ALWAYS)
	return 1;
    if ((action->flags & ROGUE_MOBILE_ACTION_STAIRS) && !context->on_stairs)
	return 0;
    if ((action->flags & ROGUE_MOBILE_ACTION_OBJECT) && !context->on_object)
	return 0;
    if ((action->flags & ROGUE_MOBILE_ACTION_TRADE)
	&& !context->in_trading_post)
	return 0;
    if ((action->flags & ROGUE_MOBILE_ACTION_POOL)
	&& !context->on_magic_pool)
	return 0;
    return 1;
}

void
rogue_mobile_action_bar_build_with_context(
    ROGUE_MOBILE_LAYOUT *layout, int x, int y, int w, int h, int scroll_y,
    const char *variant_id, const ROGUE_MOBILE_ACTION_CONTEXT *context)
{
    static const ROGUE_MOBILE_ACTION_DEF default_actions[] = {
	{ 's', "SRCH", ROGUE_MOBILE_ACTION_ALWAYS },
	{ ',', "PICK\nUP", ROGUE_MOBILE_ACTION_OBJECT },
	{ '>', "DESC", ROGUE_MOBILE_ACTION_STAIRS },
	{ 'i', "INVEN", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'e', "EAT", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'q', "QUAFF", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'r', "READ", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'w', "WIELD", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'W', "WEAR", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 't', "THROW", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'd', "DROP", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'f', "FIGHT", ROGUE_MOBILE_ACTION_ALWAYS },
	{ ROGUE_MOBILE_COMMAND_MANUALS, "READ\nBOOK",
	  ROGUE_MOBILE_ACTION_ALWAYS },
	{ ROGUE_MOBILE_COMMAND_OPTIONS, "OPTS", ROGUE_MOBILE_ACTION_ALWAYS },
	{ ROGUE_MOBILE_COMMAND_CLOSE, "CLOSE", ROGUE_MOBILE_ACTION_ALWAYS }
    };
    static const ROGUE_MOBILE_ACTION_DEF rogue54_actions[] = {
	{ 's', "SRCH", ROGUE_MOBILE_ACTION_ALWAYS },
	{ ',', "PICK\nUP", ROGUE_MOBILE_ACTION_OBJECT },
	{ '>', "DESC", ROGUE_MOBILE_ACTION_STAIRS },
	{ 'i', "INVEN", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'e', "EAT", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'q', "QUAFF", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'r', "READ", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'w', "WIELD", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'W', "WEAR", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 't', "THROW", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'd', "DROP", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'f', "FIGHT", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'z', "ZAP", ROGUE_MOBILE_ACTION_ALWAYS },
	{ '<', "ASC", ROGUE_MOBILE_ACTION_STAIRS },
	{ 'I', "ITEM", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'T', "TAKE\nOFF", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'P', "PUT\nRING", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'R', "REM\nRING", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'c', "CALL", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'D', "DISC", ROGUE_MOBILE_ACTION_ALWAYS },
	{ '^', "TRAP", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'm', "NO\nPICK", ROGUE_MOBILE_ACTION_ALWAYS },
	{ ')', "WEP", ROGUE_MOBILE_ACTION_ALWAYS },
	{ ']', "ARM", ROGUE_MOBILE_ACTION_ALWAYS },
	{ '=', "RINGS", ROGUE_MOBILE_ACTION_ALWAYS },
	{ '@', "STATS", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'o', "GAME\nOPT", ROGUE_MOBILE_ACTION_ALWAYS },
	{ ROGUE_MOBILE_COMMAND_MANUALS, "READ\nBOOK",
	  ROGUE_MOBILE_ACTION_ALWAYS },
	{ ROGUE_MOBILE_COMMAND_OPTIONS, "OPTS", ROGUE_MOBILE_ACTION_ALWAYS },
	{ ROGUE_MOBILE_COMMAND_CLOSE, "CLOSE", ROGUE_MOBILE_ACTION_ALWAYS }
    };
    static const ROGUE_MOBILE_ACTION_DEF rogue52_actions[] = {
	{ 's', "SRCH", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'i', "INVEN", ROGUE_MOBILE_ACTION_ALWAYS },
	{ '>', "DESC", ROGUE_MOBILE_ACTION_STAIRS },
	{ 'e', "EAT", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'q', "QUAFF", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'r', "READ", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'w', "WIELD", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'W', "WEAR", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'd', "DROP", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 't', "THROW", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'f', "FIGHT", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'I', "ITEM", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'z', "ZAP", ROGUE_MOBILE_ACTION_ALWAYS },
	{ '<', "ASC", ROGUE_MOBILE_ACTION_STAIRS },
	{ '^', "TRAP", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'T', "TAKE\nOFF", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'P', "PUT\nRING", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'R', "REM\nRING", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'c', "CALL", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'D', "DISC", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'o', "GAME\nOPT", ROGUE_MOBILE_ACTION_ALWAYS },
	{ ROGUE_MOBILE_COMMAND_MANUALS, "READ\nBOOK",
	  ROGUE_MOBILE_ACTION_ALWAYS },
	{ ROGUE_MOBILE_COMMAND_OPTIONS, "OPTS", ROGUE_MOBILE_ACTION_ALWAYS },
	{ ROGUE_MOBILE_COMMAND_CLOSE, "CLOSE", ROGUE_MOBILE_ACTION_ALWAYS }
    };
    static const ROGUE_MOBILE_ACTION_DEF rogue36_actions[] = {
	{ 's', "SRCH", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'i', "INVEN", ROGUE_MOBILE_ACTION_ALWAYS },
	{ '>', "DESC", ROGUE_MOBILE_ACTION_STAIRS },
	{ 'e', "EAT", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'q', "QUAFF", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'r', "READ", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'w', "WIELD", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'W', "WEAR", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'd', "DROP", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 't', "THROW", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'f', "FIGHT", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'I', "ITEM", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'z', "ZAP", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'p', "DIR\nZAP", ROGUE_MOBILE_ACTION_ALWAYS },
	{ '<', "ASC", ROGUE_MOBILE_ACTION_STAIRS },
	{ 'T', "TAKE\nOFF", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'P', "PUT\nRING", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'R', "REM\nRING", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'c', "CALL", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'o', "GAME\nOPT", ROGUE_MOBILE_ACTION_ALWAYS },
	{ ROGUE_MOBILE_COMMAND_MANUALS, "READ\nBOOK",
	  ROGUE_MOBILE_ACTION_ALWAYS },
	{ ROGUE_MOBILE_COMMAND_OPTIONS, "OPTS", ROGUE_MOBILE_ACTION_ALWAYS },
	{ ROGUE_MOBILE_COMMAND_CLOSE, "CLOSE", ROGUE_MOBILE_ACTION_ALWAYS }
    };
    static const ROGUE_MOBILE_ACTION_DEF srogue90_actions[] = {
	{ 's', "SRCH", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'i', "INVEN", ROGUE_MOBILE_ACTION_ALWAYS },
	{ '>', "DESC", ROGUE_MOBILE_ACTION_STAIRS },
	{ 'e', "EAT", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'q', "QUAFF", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'r', "READ", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'w', "WIELD", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'W', "WEAR", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'd', "DROP", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 't', "THROW", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'f', "FWD", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'I', "ITEM", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'z', "ZAP", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'p', "DIR\nZAP", ROGUE_MOBILE_ACTION_ALWAYS },
	{ '<', "ASC", ROGUE_MOBILE_ACTION_STAIRS },
	{ 'T', "TAKE\nOFF", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'P', "PUT\nRING", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'R', "REM\nRING", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'c', "CALL", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'a', "MAX", ROGUE_MOBILE_ACTION_ALWAYS },
	{ 'D', "DIP", ROGUE_MOBILE_ACTION_POOL },
	{ '$', "PRICE", ROGUE_MOBILE_ACTION_TRADE },
	{ '#', "BUY", ROGUE_MOBILE_ACTION_TRADE },
	{ '%', "SELL", ROGUE_MOBILE_ACTION_TRADE },
	{ ROGUE_MOBILE_COMMAND_MANUALS, "READ\nBOOK",
	  ROGUE_MOBILE_ACTION_ALWAYS },
	{ ROGUE_MOBILE_COMMAND_OPTIONS, "OPTS", ROGUE_MOBILE_ACTION_ALWAYS },
	{ ROGUE_MOBILE_COMMAND_CLOSE, "CLOSE", ROGUE_MOBILE_ACTION_ALWAYS }
    };
    const ROGUE_MOBILE_ACTION_DEF *actions;
    int source_count;
    int action_count;
    int col_widths[ROGUE_MOBILE_ACTION_COLS];
    int row_height;
    int total_rows;
    int row;
    int col;
    int i;

    if (layout == 0)
    {
	return;
    }

    actions = default_actions;
    source_count = (int)(sizeof(default_actions) / sizeof(default_actions[0]));
    if (variant_id != 0 && strcmp(variant_id, "srogue90") == 0)
    {
	actions = srogue90_actions;
	source_count = (int)(sizeof(srogue90_actions) /
			     sizeof(srogue90_actions[0]));
    }
    else if (variant_id != 0 && strcmp(variant_id, "rogue54") == 0)
    {
	actions = rogue54_actions;
	source_count = (int)(sizeof(rogue54_actions) /
			     sizeof(rogue54_actions[0]));
    }
    else if (variant_id != 0 && strcmp(variant_id, "rogue52") == 0)
    {
	actions = rogue52_actions;
	source_count = (int)(sizeof(rogue52_actions) /
			     sizeof(rogue52_actions[0]));
    }
    else if (variant_id != 0 && strcmp(variant_id, "rogue36") == 0)
    {
	actions = rogue36_actions;
	source_count = (int)(sizeof(rogue36_actions) /
			     sizeof(rogue36_actions[0]));
    }

    action_count = 0;
    for (i = 0; i < source_count
	 && action_count < ROGUE_MOBILE_ACTION_BUTTON_COUNT; i++)
    {
	if (!action_visible_for_context(&actions[i], context))
	    continue;
	layout->actions[action_count].command = actions[i].command;
	layout->actions[action_count].label = actions[i].label;
	action_count++;
    }

    layout->action_count = action_count;
    layout->action_viewport.x = x;
    layout->action_viewport.y = y;
    layout->action_viewport.w = w;
    layout->action_viewport.h = h;
    row_height = h / ROGUE_MOBILE_ACTION_VISIBLE_ROWS;
    if (row_height < 1)
	row_height = 1;
    total_rows = (action_count + ROGUE_MOBILE_ACTION_COLS - 1)
		 / ROGUE_MOBILE_ACTION_COLS;
    layout->action_content_height = row_height * total_rows;
    scroll_y = rogue_mobile_action_scroll_clamp(layout, scroll_y);

    for (col = 0; col < ROGUE_MOBILE_ACTION_COLS; col++)
	col_widths[col] = w / ROGUE_MOBILE_ACTION_COLS;
    col_widths[ROGUE_MOBILE_ACTION_COLS - 1] =
	w - col_widths[0] - col_widths[1];

    for (i = 0; i < action_count; i++)
    {
	row = i / ROGUE_MOBILE_ACTION_COLS;
	col = i % ROGUE_MOBILE_ACTION_COLS;
	layout->actions[i].rect.x = x + (col_widths[0] * col);
	layout->actions[i].rect.y = y + (row_height * row) - scroll_y;
	layout->actions[i].rect.w = col_widths[col];
	layout->actions[i].rect.h = row_height;
    }
}

char
rogue_mobile_action_at(const ROGUE_MOBILE_LAYOUT *layout, int x, int y)
{
    int index;

    index = rogue_mobile_action_index_at(layout, x, y);
    if (index < 0)
    {
	return '\0';
    }
    return layout->actions[index].command;
}

int
rogue_mobile_action_index_at(const ROGUE_MOBILE_LAYOUT *layout, int x, int y)
{
    int count;
    int i;

    if (layout == 0 || !rect_contains(&layout->action_viewport, x, y))
    {
	return -1;
    }

    count = layout->action_count;
    if (count < 0)
    {
	count = 0;
    }
    if (count > ROGUE_MOBILE_ACTION_BUTTON_COUNT)
    {
	count = ROGUE_MOBILE_ACTION_BUTTON_COUNT;
    }

    for (i = 0; i < count; i++)
    {
	if (rect_contains(&layout->actions[i].rect, x, y))
	{
	    return i;
	}
    }

    return -1;
}

int
rogue_mobile_action_scroll_max(const ROGUE_MOBILE_LAYOUT *layout)
{
    int max_scroll;

    if (layout == 0)
    {
	return 0;
    }

    max_scroll = layout->action_content_height - layout->action_viewport.h;
    if (max_scroll < 0)
    {
	max_scroll = 0;
    }
    return max_scroll;
}

int
rogue_mobile_action_scroll_clamp(const ROGUE_MOBILE_LAYOUT *layout,
				 int scroll_y)
{
    int max_scroll;

    if (scroll_y < 0)
    {
	return 0;
    }

    max_scroll = rogue_mobile_action_scroll_max(layout);
    if (scroll_y > max_scroll)
    {
	return max_scroll;
    }
    return scroll_y;
}

void
rogue_mobile_zoom_slider_build(ROGUE_MOBILE_LAYOUT *layout, int x, int y,
			       int w, int h)
{
    if (layout == 0)
    {
	return;
    }

    layout->zoom_slider.x = x;
    layout->zoom_slider.y = y;
    layout->zoom_slider.w = w;
    layout->zoom_slider.h = h;
}

int
rogue_mobile_zoom_at(const ROGUE_MOBILE_LAYOUT *layout, int x, int y,
		    int min_value, int max_value)
{
    int range;
    int span;
    int offset;
    int delta;
    int steps;

    if (layout == 0 || !rect_contains(&layout->zoom_slider, x, y) ||
	min_value >= max_value)
    {
	return 0;
    }

    range = max_value - min_value;
    span = layout->zoom_slider.h - 1;
    if (span <= 0)
    {
	return max_value;
    }

    offset = y - layout->zoom_slider.y;
    delta = (offset * range + (span / 2)) / span;
    steps = (delta + (ROGUE_MOBILE_ZOOM_STEP / 2)) / ROGUE_MOBILE_ZOOM_STEP;
    return max_value - (steps * ROGUE_MOBILE_ZOOM_STEP);
}
