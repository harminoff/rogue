/*
 * Small, frontend-independent helpers for selectable text overlays.
 */

#include <ctype.h>
#include "overlay_picker.h"

char
rogue_picker_line_key(const char *line)
{
    const char *scan;

    if (line == 0)
	return '\0';

    scan = line;
    while (*scan != '\0' && isspace((unsigned char) *scan))
	scan++;

    if (!isgraph((unsigned char) scan[0]) || scan[1] != ')')
	return '\0';

    return scan[0];
}

int
rogue_picker_clamp_selection(int selection, int count)
{
    if (count <= 0)
	return -1;
    if (selection < 0)
	return 0;
    if (selection >= count)
	return count - 1;

    return selection;
}

int
rogue_picker_move_selection(int selection, int count, int delta)
{
    selection = rogue_picker_clamp_selection(selection, count);
    if (selection < 0)
	return -1;

    return rogue_picker_clamp_selection(selection + delta, count);
}

int
rogue_picker_find_key(char key, const char **lines, int count)
{
    int i;
    char line_key;
    int wanted;

    if (lines == 0 || count <= 0 || key == '\0')
	return -1;

    wanted = tolower((unsigned char) key);
    for (i = 0; i < count; i++)
    {
	line_key = rogue_picker_line_key(lines[i]);
	if (line_key != '\0'
	    && tolower((unsigned char) line_key) == wanted)
	    return i;
    }

    return -1;
}

int
rogue_picker_first_keyed_line(const char **lines, int count)
{
    int i;

    if (lines == 0 || count <= 0)
	return -1;

    for (i = 0; i < count; i++)
	if (rogue_picker_line_key(lines[i]) != '\0')
	    return i;

    return -1;
}

void
rogue_overlay_find_bounds(const char **rows, int row_count, int col_count,
			  int *top, int *bottom, int *left, int *right)
{
    int y, x;

    *top = -1;
    *bottom = -1;
    *left = -1;
    *right = -1;

    if (rows == 0 || row_count <= 0 || col_count <= 0)
	return;

    for (y = 0; y < row_count; y++)
    {
	if (rows[y] == 0)
	    continue;
	for (x = 0; x < col_count && rows[y][x] != '\0'; x++)
	{
	    if (!isspace((unsigned char) rows[y][x]))
	    {
		if (*top < 0)
		    *top = y;
		*bottom = y;
		if (*left < 0 || x < *left)
		    *left = x;
		if (x > *right)
		    *right = x;
	    }
	}
    }
}

void
rogue_overlay_copy_span(const char *row, int left, int right,
			char *out, int out_size)
{
    int i, len;

    if (out == 0 || out_size <= 0)
	return;

    out[0] = '\0';
    if (row == 0 || left < 0 || right < left)
	return;

    len = right - left + 1;
    if (len >= out_size)
	len = out_size - 1;

    for (i = 0; i < len; i++)
	out[i] = row[left + i] == '\0' ? ' ' : row[left + i];
    out[len] = '\0';
}
