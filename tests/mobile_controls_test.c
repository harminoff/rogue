#include <stdio.h>
#include <string.h>

#include "mobile_controls.h"

typedef struct rogue_mobile_layout_probe {
    ROGUE_MOBILE_LAYOUT layout;
    int fake_w;
    int fake_h;
    char fake_command;
} ROGUE_MOBILE_LAYOUT_PROBE;

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

static int
layout_has_command(const ROGUE_MOBILE_LAYOUT *layout, char command)
{
    int i;

    for (i = 0; i < layout->action_count; i++)
	if (layout->actions[i].command == command)
	    return 1;
    return 0;
}

static void
expect_command_present(const char *name, const ROGUE_MOBILE_LAYOUT *layout,
		       char command, int expected)
{
    int got;

    got = layout_has_command(layout, command);
    if (got != expected)
    {
	printf("%s: command %d got %d expected %d\n",
	       name, command, got, expected);
	failures++;
    }
}

int
main(void)
{
    ROGUE_MOBILE_LAYOUT layout;
    ROGUE_MOBILE_LAYOUT_PROBE probe;
    ROGUE_MOBILE_ACTION_CONTEXT context;
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

    rogue_mobile_action_bar_build(&layout, 100, 200, 240, 120, 0);
    expect_int("action count", layout.action_count, 15);
    expect_int("action grid scrolls two hidden rows",
	       rogue_mobile_action_scroll_max(&layout), 80);
    expect_char("first action taps search",
		rogue_mobile_action_at(&layout, 104, 204), 's');
    expect_char("second action taps pick up",
		rogue_mobile_action_at(&layout, 184, 204), ',');
    expect_char("third action taps descend",
		rogue_mobile_action_at(&layout, 284, 204), '>');
    expect_char("fourth action starts inventory row",
		rogue_mobile_action_at(&layout, 104, 244), 'i');
    expect_char("middle action taps eat",
		rogue_mobile_action_at(&layout, 184, 244), 'e');
    expect_char("sixth action taps quaff",
		rogue_mobile_action_at(&layout, 284, 244), 'q');
    expect_char("seventh action taps read",
		rogue_mobile_action_at(&layout, 104, 284), 'r');
    expect_char("eighth action taps wield",
		rogue_mobile_action_at(&layout, 184, 284), 'w');
    expect_char("last visible action taps wear",
		rogue_mobile_action_at(&layout, 284, 284), 'W');
    expect_int("action index hits wear",
	       rogue_mobile_action_index_at(&layout, 284, 284), 8);
    expect_char("action ignores left outside",
		rogue_mobile_action_at(&layout, 99, 204), '\0');
    expect_int("action index ignores outside",
	       rogue_mobile_action_index_at(&layout, 99, 204), -1);
    expect_char("action ignores below viewport",
		rogue_mobile_action_at(&layout, 104, 320), '\0');
    rogue_mobile_action_bar_build(&layout, 100, 200, 240, 120, 40);
    expect_char("scrolled first hidden action taps throw",
		rogue_mobile_action_at(&layout, 104, 284), 't');
    expect_char("scrolled second hidden action taps drop",
		rogue_mobile_action_at(&layout, 184, 284), 'd');
    expect_char("scrolled third hidden action taps fight",
		rogue_mobile_action_at(&layout, 284, 284), 'f');
    expect_int("scrolled action index hits fight",
	       rogue_mobile_action_index_at(&layout, 284, 284), 11);
    rogue_mobile_action_bar_build(&layout, 100, 200, 240, 120, 80);
    expect_char("scrolled manuals action opens books",
		rogue_mobile_action_at(&layout, 104, 284),
		ROGUE_MOBILE_COMMAND_MANUALS);
    expect_char("scrolled options action opens settings",
		rogue_mobile_action_at(&layout, 184, 284),
		ROGUE_MOBILE_COMMAND_OPTIONS);
    expect_char("scrolled close action exits overlays",
		rogue_mobile_action_at(&layout, 284, 284),
		ROGUE_MOBILE_COMMAND_CLOSE);
    expect_int("scrolled action index hits close",
	       rogue_mobile_action_index_at(&layout, 284, 284), 14);
    expect_int("negative action scroll clamps to zero",
	       rogue_mobile_action_scroll_clamp(&layout, -40), 0);
    expect_int("large action scroll clamps to max",
	       rogue_mobile_action_scroll_clamp(&layout, 999),
	       rogue_mobile_action_scroll_max(&layout));

    memset(&context, 0, sizeof(context));
    rogue_mobile_action_bar_build_with_context(&layout, 100, 200, 240, 120, 0,
					       "srogue90", &context);
    expect_command_present("super rogue hides price away from trading post",
			   &layout, '$', 0);
    expect_command_present("super rogue hides buy away from trading post",
			   &layout, '#', 0);
    expect_command_present("super rogue hides sell away from trading post",
			   &layout, '%', 0);
    expect_command_present("super rogue hides dip away from pool",
			   &layout, 'D', 0);
    expect_command_present("super rogue hides descend away from stairs",
			   &layout, '>', 0);
    expect_command_present("super rogue hides ascend away from stairs",
			   &layout, '<', 0);
    context.in_trading_post = 1;
    rogue_mobile_action_bar_build_with_context(&layout, 100, 200, 240, 120, 0,
					       "srogue90", &context);
    expect_command_present("super rogue shows price in trading post",
			   &layout, '$', 1);
    expect_command_present("super rogue shows buy in trading post",
			   &layout, '#', 1);
    expect_command_present("super rogue shows sell in trading post",
			   &layout, '%', 1);
    context.on_magic_pool = 1;
    rogue_mobile_action_bar_build_with_context(&layout, 100, 200, 240, 120, 0,
					       "srogue90", &context);
    expect_command_present("super rogue shows dip at pool",
			   &layout, 'D', 1);
    context.on_stairs = 1;
    rogue_mobile_action_bar_build_with_context(&layout, 100, 200, 240, 120, 0,
					       "srogue90", &context);
    expect_command_present("super rogue shows descend on stairs",
			   &layout, '>', 1);
    expect_command_present("super rogue shows ascend on stairs",
			   &layout, '<', 1);

    memset(&context, 0, sizeof(context));
    rogue_mobile_action_bar_build_with_context(&layout, 100, 200, 240, 120, 0,
					       "rogue54", &context);
    expect_command_present("rogue54 hides pickup away from objects",
			   &layout, ',', 0);
    context.on_object = 1;
    rogue_mobile_action_bar_build_with_context(&layout, 100, 200, 240, 120, 0,
					       "rogue54", &context);
    expect_command_present("rogue54 shows pickup on objects",
			   &layout, ',', 1);

    rogue_mobile_action_bar_build_for_variant(&layout, 100, 200, 240, 120, 0,
					      "rogue54");
    expect_int("rogue54 action count", layout.action_count, 30);
    expect_int("rogue54 action grid scrolls seven hidden rows",
	       rogue_mobile_action_scroll_max(&layout), 280);
    expect_char("rogue54 keeps pickup action",
		rogue_mobile_action_at(&layout, 184, 204), ',');
    rogue_mobile_action_bar_build_for_variant(&layout, 100, 200, 240, 120, 80,
					      "rogue54");
    expect_char("rogue54 scrolled zap action",
		rogue_mobile_action_at(&layout, 104, 284), 'z');
    expect_char("rogue54 scrolled ascend action",
		rogue_mobile_action_at(&layout, 184, 284), '<');
    expect_char("rogue54 scrolled item inspect action",
		rogue_mobile_action_at(&layout, 284, 284), 'I');
    rogue_mobile_action_bar_build_for_variant(&layout, 100, 200, 240, 120, 120,
					      "rogue54");
    expect_char("rogue54 scrolled take off action",
		rogue_mobile_action_at(&layout, 104, 284), 'T');
    expect_char("rogue54 scrolled put ring action",
		rogue_mobile_action_at(&layout, 184, 284), 'P');
    expect_char("rogue54 scrolled remove ring action",
		rogue_mobile_action_at(&layout, 284, 284), 'R');
    rogue_mobile_action_bar_build_for_variant(&layout, 100, 200, 240, 120, 160,
					      "rogue54");
    expect_char("rogue54 scrolled call action",
		rogue_mobile_action_at(&layout, 104, 284), 'c');
    expect_char("rogue54 scrolled discoveries action",
		rogue_mobile_action_at(&layout, 184, 284), 'D');
    expect_char("rogue54 scrolled trap identify action",
		rogue_mobile_action_at(&layout, 284, 284), '^');
    rogue_mobile_action_bar_build_for_variant(&layout, 100, 200, 240, 120, 280,
					      "rogue54");
    expect_char("rogue54 keeps manuals action",
		rogue_mobile_action_at(&layout, 104, 284),
		ROGUE_MOBILE_COMMAND_MANUALS);
    expect_char("rogue54 keeps options action",
		rogue_mobile_action_at(&layout, 184, 284),
		ROGUE_MOBILE_COMMAND_OPTIONS);
    expect_char("rogue54 keeps close action",
		rogue_mobile_action_at(&layout, 284, 284),
		ROGUE_MOBILE_COMMAND_CLOSE);

    rogue_mobile_action_bar_build_for_variant(&layout, 100, 200, 240, 120, 0,
					      "rogue52");
    expect_int("rogue52 action count", layout.action_count, 24);
    expect_char("rogue52 replaces pickup with inventory",
		rogue_mobile_action_at(&layout, 184, 204), 'i');
    rogue_mobile_action_bar_build_for_variant(&layout, 100, 200, 240, 120, 80,
					      "rogue52");
    expect_char("rogue52 scrolled zap action",
		rogue_mobile_action_at(&layout, 104, 284), 'z');
    expect_char("rogue52 scrolled ascend action",
		rogue_mobile_action_at(&layout, 184, 284), '<');
    expect_char("rogue52 scrolled trap identify action",
		rogue_mobile_action_at(&layout, 284, 284), '^');
    rogue_mobile_action_bar_build_for_variant(&layout, 100, 200, 240, 120, 160,
					      "rogue52");
    expect_char("rogue52 scrolled call action",
		rogue_mobile_action_at(&layout, 104, 284), 'c');
    expect_char("rogue52 scrolled discoveries action",
		rogue_mobile_action_at(&layout, 184, 284), 'D');
    expect_char("rogue52 scrolled game options action",
		rogue_mobile_action_at(&layout, 284, 284), 'o');

    rogue_mobile_action_bar_build_for_variant(&layout, 100, 200, 240, 120, 0,
					      "rogue36");
    expect_int("rogue36 action count", layout.action_count, 23);
    expect_char("rogue36 replaces pickup with inventory",
		rogue_mobile_action_at(&layout, 184, 204), 'i');
    rogue_mobile_action_bar_build_for_variant(&layout, 100, 200, 240, 120, 80,
					      "rogue36");
    expect_char("rogue36 scrolled zap action",
		rogue_mobile_action_at(&layout, 104, 284), 'z');
    expect_char("rogue36 scrolled directional zap action",
		rogue_mobile_action_at(&layout, 184, 284), 'p');
    expect_char("rogue36 scrolled ascend action",
		rogue_mobile_action_at(&layout, 284, 284), '<');
    rogue_mobile_action_bar_build_for_variant(&layout, 100, 200, 240, 120, 160,
					      "rogue36");
    expect_char("rogue36 scrolled call action",
		rogue_mobile_action_at(&layout, 104, 284), 'c');
    expect_char("rogue36 scrolled game options action",
		rogue_mobile_action_at(&layout, 184, 284), 'o');
    expect_char("rogue36 scrolled manuals action",
		rogue_mobile_action_at(&layout, 284, 284),
		ROGUE_MOBILE_COMMAND_MANUALS);

    rogue_mobile_action_bar_build_for_variant(&layout, 100, 200, 240, 120, 0,
					      "srogue90");
    expect_int("super rogue action count", layout.action_count, 27);
    expect_int("super rogue action grid scrolls six hidden rows",
	       rogue_mobile_action_scroll_max(&layout), 240);
    expect_char("super rogue first action taps search",
		rogue_mobile_action_at(&layout, 104, 204), 's');
    expect_char("super rogue replaces pickup with inventory",
		rogue_mobile_action_at(&layout, 184, 204), 'i');
    expect_char("super rogue first row descends",
		rogue_mobile_action_at(&layout, 284, 204), '>');
    rogue_mobile_action_bar_build_for_variant(&layout, 100, 200, 240, 120, 80,
					      "srogue90");
    expect_char("super rogue scrolled zap action",
		rogue_mobile_action_at(&layout, 104, 284), 'z');
    expect_char("super rogue scrolled directional zap action",
		rogue_mobile_action_at(&layout, 184, 284), 'p');
    expect_char("super rogue scrolled ascend action",
		rogue_mobile_action_at(&layout, 284, 284), '<');
    rogue_mobile_action_bar_build_for_variant(&layout, 100, 200, 240, 120, 120,
					      "srogue90");
    expect_char("super rogue scrolled take off action",
		rogue_mobile_action_at(&layout, 104, 284), 'T');
    expect_char("super rogue scrolled put ring action",
		rogue_mobile_action_at(&layout, 184, 284), 'P');
    expect_char("super rogue scrolled remove ring action",
		rogue_mobile_action_at(&layout, 284, 284), 'R');
    rogue_mobile_action_bar_build_for_variant(&layout, 100, 200, 240, 120, 200,
					      "srogue90");
    expect_char("super rogue scrolled price action",
		rogue_mobile_action_at(&layout, 104, 284), '$');
    expect_char("super rogue scrolled buy action",
		rogue_mobile_action_at(&layout, 184, 284), '#');
    expect_char("super rogue scrolled sell action",
		rogue_mobile_action_at(&layout, 284, 284), '%');
    rogue_mobile_action_bar_build_for_variant(&layout, 100, 200, 240, 120, 240,
					      "srogue90");
    expect_char("super rogue keeps manuals action",
		rogue_mobile_action_at(&layout, 104, 284),
		ROGUE_MOBILE_COMMAND_MANUALS);
    expect_char("super rogue keeps options action",
		rogue_mobile_action_at(&layout, 184, 284),
		ROGUE_MOBILE_COMMAND_OPTIONS);
    expect_char("super rogue keeps close action",
		rogue_mobile_action_at(&layout, 284, 284),
		ROGUE_MOBILE_COMMAND_CLOSE);

    rogue_mobile_layout_build(&layout, 100, 200, 180, 120);
    expect_int("pressed center movement index",
	       rogue_mobile_movement_index_at(&layout, 170, 250), 4);
    expect_int("outside movement index",
	       rogue_mobile_movement_index_at(&layout, 99, 250), -1);

    rogue_mobile_zoom_slider_build(&layout, 300, 100, 20, 120);
    expect_int("zoom top picks maximum tile size",
	       rogue_mobile_zoom_at(&layout, 304, 100, 16, 64), 64);
    expect_int("zoom bottom picks minimum tile size",
	       rogue_mobile_zoom_at(&layout, 304, 219, 16, 64), 16);
    expect_int("zoom middle rounds to step",
	       rogue_mobile_zoom_at(&layout, 304, 159, 16, 64), 40);
    expect_int("zoom ignores outside x",
	       rogue_mobile_zoom_at(&layout, 299, 159, 16, 64), 0);

    memset(&probe, 0, sizeof(probe));
    rogue_mobile_layout_build(&probe.layout, 10, 20, 31, 32);
    probe.fake_w = 99;
    probe.fake_h = 99;
    probe.fake_command = 'X';
    probe.layout.button_count = ROGUE_MOBILE_BUTTON_COUNT + 1;
    expect_char("high button count ignores out of bounds slot",
		rogue_mobile_command_at(&probe.layout, 10, 0), '\0');

    if (failures != 0)
    {
	printf("%d mobile control test(s) failed\n", failures);
	return 1;
    }

    printf("mobile control tests passed\n");
    return 0;
}
