#include <stdio.h>

#include "mobile_controls.h"

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

int
main(void)
{
    ROGUE_MOBILE_LAYOUT layout;
    const char expected[3][3] = {
	{ 'y', 'k', 'u' },
	{ 'h', '.', 'l' },
	{ 'b', 'j', 'n' }
    };
    const int sample_x[3] = { 11, 21, 40 };
    const int sample_y[3] = { 21, 31, 51 };
    int row, col;

    rogue_mobile_layout_build(&layout, 10, 20, 31, 32);

    expect_int("button count", layout.button_count, ROGUE_MOBILE_BUTTON_COUNT);

    for (row = 0; row < 3; row++)
    {
	for (col = 0; col < 3; col++)
	{
	    expect_char("grid command",
			rogue_mobile_command_at(&layout,
						sample_x[col],
						sample_y[row]),
			expected[row][col]);
	}
    }

    expect_char("left outside", rogue_mobile_command_at(&layout, 9, 20), '\0');
    expect_char("top outside", rogue_mobile_command_at(&layout, 10, 19), '\0');
    expect_char("right outside", rogue_mobile_command_at(&layout, 41, 20), '\0');
    expect_char("bottom outside", rogue_mobile_command_at(&layout, 10, 52), '\0');

    expect_char("column boundary moves right",
		rogue_mobile_command_at(&layout, 20, 20), 'k');
    expect_char("row boundary moves down",
		rogue_mobile_command_at(&layout, 10, 30), 'h');
    expect_char("last covered pixel",
		rogue_mobile_command_at(&layout, 40, 51), 'n');

    if (failures != 0)
    {
	printf("%d mobile control test(s) failed\n", failures);
	return 1;
    }

    printf("mobile control tests passed\n");
    return 0;
}
