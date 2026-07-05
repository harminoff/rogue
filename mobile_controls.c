#include "mobile_controls.h"

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
    const ROGUE_MOBILE_RECT *rect;
    int count;
    int i;

    if (layout == 0)
    {
	return '\0';
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

	if (x >= rect->x && x < rect->x + rect->w &&
	    y >= rect->y && y < rect->y + rect->h)
	{
	    return layout->buttons[i].command;
	}
    }

    return '\0';
}
