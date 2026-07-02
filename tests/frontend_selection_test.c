#include <stdio.h>

#include "../frontend.h"

static int failures = 0;

char
md_readchar(void)
{
    return '\0';
}

static void
expect_bool(const char *name, bool got, bool expected)
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
    expect_bool("packaged exe defaults to tiles",
		rogue_frontend_default_tiles_for_executable(
		    "C:\\Games\\RogueTiles\\RogueTiles.exe"),
		TRUE);
    expect_bool("dev exe stays ascii by default",
		rogue_frontend_default_tiles_for_executable(
		    "C:\\Programming\\Repos\\RogueTiles\\native-build\\rogue54.exe"),
		FALSE);
    expect_bool("case insensitive package name",
		rogue_frontend_default_tiles_for_executable("roguetiles.EXE"),
		TRUE);

    if (failures != 0)
    {
	printf("%d frontend selection test(s) failed\n", failures);
	return 1;
    }

    printf("frontend selection tests passed\n");
    return 0;
}
