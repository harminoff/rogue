#include <stdio.h>

#include "../overlay_picker.h"

void rogue_overlay_find_bounds(const char **rows, int row_count, int col_count,
			       int *top, int *bottom, int *left, int *right);
void rogue_overlay_copy_span(const char *row, int left, int right,
			     char *out, int out_size);

static int failures = 0;

static void
expect_int(const char *name, int got, int expected)
{
    if (got != expected)
    {
	printf("%s: got %d expected %d\n", name, got, expected);
	failures++;
    }
}

static void
expect_char(const char *name, char got, char expected)
{
    if (got != expected)
    {
	printf("%s: got %d expected %d\n", name, got, expected);
	failures++;
    }
}

static void
expect_str(const char *name, const char *got, const char *expected)
{
    const char *g, *e;

    g = got;
    e = expected;
    while (*g != '\0' && *e != '\0' && *g == *e)
    {
	g++;
	e++;
    }
    if (*g != *e)
    {
	printf("%s: got \"%s\" expected \"%s\"\n", name, got, expected);
	failures++;
    }
}

int
main(void)
{
    const char *lines[] = {
	"a) potion of healing",
	"b) scroll of identify",
	"z) long sword",
	"not an item",
	"    d) mace",
	"|) wall"
    };
    const char *map_rows[] = {
	"     ",
	"  $  ",
	"     ",
	" :   "
    };
    const char *empty_rows[] = {
	"     ",
	"     "
    };
    char span[16];
    int top, bottom, left, right;

    expect_char("line key extracts pack letter",
		rogue_picker_line_key(lines[0]), 'a');
    expect_char("line key ignores non-item line",
		rogue_picker_line_key(lines[3]), '\0');
    expect_char("line key handles padded item line",
	        rogue_picker_line_key(lines[4]), 'd');
    expect_char("line key handles symbol choices",
		rogue_picker_line_key(lines[5]), '|');
    expect_char("line key ignores null",
		rogue_picker_line_key(NULL), '\0');

    expect_int("clamp empty", rogue_picker_clamp_selection(0, 0), -1);
    expect_int("clamp negative", rogue_picker_clamp_selection(-3, 3), 0);
    expect_int("clamp high", rogue_picker_clamp_selection(9, 3), 2);

    expect_int("move down", rogue_picker_move_selection(0, 3, 1), 1);
    expect_int("move up stays at top", rogue_picker_move_selection(0, 3, -1), 0);
    expect_int("move down stays at bottom", rogue_picker_move_selection(2, 3, 1), 2);

    expect_int("find lower key", rogue_picker_find_key('b', lines, 5), 1);
    expect_int("find upper key", rogue_picker_find_key('B', lines, 5), 1);
    expect_int("find padded key", rogue_picker_find_key('d', lines, 6), 4);
    expect_int("find symbol key", rogue_picker_find_key('|', lines, 6), 5);
    expect_int("find missing key", rogue_picker_find_key('x', lines, 6), -1);
    expect_int("first keyed line skips prompt text",
	       rogue_picker_first_keyed_line(&lines[3], 3), 1);

    rogue_overlay_find_bounds(map_rows, 4, 5, &top, &bottom, &left, &right);
    expect_int("map top", top, 1);
    expect_int("map bottom", bottom, 3);
    expect_int("map left", left, 1);
    expect_int("map right", right, 2);
    rogue_overlay_copy_span(map_rows[1], left, right, span, sizeof(span));
    expect_str("copy map span preserves x offset", span, " $");

    rogue_overlay_find_bounds(empty_rows, 2, 5, &top, &bottom, &left, &right);
    expect_int("empty map top", top, -1);
    expect_int("empty map bottom", bottom, -1);
    expect_int("empty map left", left, -1);
    expect_int("empty map right", right, -1);

    if (failures != 0)
    {
	printf("%d picker test(s) failed\n", failures);
	return 1;
    }

    printf("overlay picker tests passed\n");
    return 0;
}
