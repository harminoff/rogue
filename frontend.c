/*
 * Frontend selection and curses fallback.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#if defined(_WIN32) && defined(ROGUE_ENABLE_ALLEGRO)
#include <windows.h>
#endif
#include "rogue.h"
#include "frontend.h"

static ROGUE_FRONTEND_KIND frontend_kind = ROGUE_FRONTEND_CURSES;
static bool tiles_requested = FALSE;
static bool smoke_requested = FALSE;

#ifdef ROGUE_ENABLE_ALLEGRO
bool rogue_allegro_start(bool smoke);
void rogue_allegro_render(void);
char rogue_allegro_readchar(void);
void rogue_allegro_show_prompt(const char *prompt);
void rogue_allegro_clear_prompt(void);
void rogue_allegro_show_death(const char *killer, int gold, bool has_amulet);
void rogue_allegro_wait_for_return(const char *prompt);
void rogue_allegro_text_overlay_begin(const char *title);
void rogue_allegro_text_overlay_add(const char *line);
char rogue_allegro_text_overlay_show(const char *prompt);
char rogue_allegro_text_overlay_pick(const char *prompt);
void rogue_allegro_text_overlay_clear(void);
bool rogue_allegro_text_input(const char *title, const char *prompt,
			      const char *initial, char *out, int out_size);
bool rogue_allegro_confirm(const char *title, const char *prompt);
bool rogue_allegro_notice(const char *title, const char *message);
void rogue_allegro_shutdown(void);
#endif

static int
ascii_lower(int ch)
{
    if (ch >= 'A' && ch <= 'Z')
	return ch + ('a' - 'A');
    return ch;
}

static bool
ascii_equal_case_insensitive(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0')
    {
	if (ascii_lower((unsigned char) *left)
	    != ascii_lower((unsigned char) *right))
	    return FALSE;
	left++;
	right++;
    }

    return (bool)(*left == '\0' && *right == '\0');
}

static const char *
path_basename(const char *path)
{
    const char *base;
    const char *p;

    if (path == NULL)
	return "";

    base = path;
    for (p = path; *p != '\0'; p++)
	if (*p == '/' || *p == '\\')
	    base = p + 1;

    return base;
}

bool
rogue_frontend_default_tiles_for_executable(const char *path)
{
    const char *base;

    base = path_basename(path);
    return (bool)(ascii_equal_case_insensitive(base, "RogueTiles.exe")
		  || ascii_equal_case_insensitive(base, "RogueTiles"));
}

bool
rogue_frontend_init(int *argc, char **argv)
{
    int read_idx, write_idx;

    if (argv != NULL && argv[0] != NULL
	&& rogue_frontend_default_tiles_for_executable(argv[0]))
	tiles_requested = TRUE;

    write_idx = 1;
    for (read_idx = 1; read_idx < *argc; read_idx++)
    {
	if (strcmp(argv[read_idx], "--tiles") == 0)
	    tiles_requested = TRUE;
	else if (strcmp(argv[read_idx], "--tiles-smoke") == 0)
	{
	    tiles_requested = TRUE;
	    smoke_requested = TRUE;
	}
	else
	    argv[write_idx++] = argv[read_idx];
    }

    argv[write_idx] = NULL;
    *argc = write_idx;

    if (tiles_requested)
    {
#ifdef ROGUE_ENABLE_ALLEGRO
	frontend_kind = ROGUE_FRONTEND_ALLEGRO;
#ifdef _WIN32
	if (getenv("TERM") == NULL || strcmp(getenv("TERM"), "ms-terminal") == 0)
	    _putenv("TERM=xterm");
	if (getenv("TERMINFO") == NULL)
	    _putenv("TERMINFO=C:\\msys64\\mingw64\\share\\terminfo");
#endif
#else
	fprintf(stderr, "This executable was not built with Allegro tile mode.\n");
	return FALSE;
#endif
    }

    return TRUE;
}

bool
rogue_frontend_start(void)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	return rogue_allegro_start(smoke_requested);
#endif

    return TRUE;
}

void
rogue_frontend_render(void)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	rogue_allegro_render();
#endif
}

char
rogue_frontend_readchar(void)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	return rogue_allegro_readchar();
#endif

    return (char) md_readchar();
}

void
rogue_frontend_show_prompt(const char *prompt)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	rogue_allegro_show_prompt(prompt);
#endif
}

void
rogue_frontend_clear_prompt(void)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	rogue_allegro_clear_prompt();
#endif
}

void
rogue_frontend_show_death(const char *killer, int gold, bool has_amulet)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	rogue_allegro_show_death(killer, gold, has_amulet);
#endif
}

void
rogue_frontend_wait_for_return(const char *prompt)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
    {
	rogue_allegro_wait_for_return(prompt);
	return;
    }
#endif

    (void) prompt;
}

void
rogue_frontend_text_overlay_begin(const char *title)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	rogue_allegro_text_overlay_begin(title);
#endif

    (void) title;
}

void
rogue_frontend_text_overlay_add(const char *line)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	rogue_allegro_text_overlay_add(line);
#endif

    (void) line;
}

char
rogue_frontend_text_overlay_show(const char *prompt)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	return rogue_allegro_text_overlay_show(prompt);
#endif

    (void) prompt;
    return '\0';
}

char
rogue_frontend_text_overlay_pick(const char *prompt)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	return rogue_allegro_text_overlay_pick(prompt);
#endif

    (void) prompt;
    return '\0';
}

void
rogue_frontend_text_overlay_clear(void)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	rogue_allegro_text_overlay_clear();
#endif
}

bool
rogue_frontend_text_input(const char *title, const char *prompt,
			  const char *initial, char *out, int out_size)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	return rogue_allegro_text_input(title, prompt, initial, out, out_size);
#endif

    (void) title;
    (void) prompt;
    (void) initial;
    (void) out;
    (void) out_size;
    return FALSE;
}

bool
rogue_frontend_confirm(const char *title, const char *prompt)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	return rogue_allegro_confirm(title, prompt);
#endif

    (void) title;
    (void) prompt;
    return FALSE;
}

bool
rogue_frontend_notice(const char *title, const char *message)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
    {
	if (rogue_allegro_notice(title, message))
	    return TRUE;
#ifdef _WIN32
	MessageBoxA(NULL, message != NULL ? message : "",
		    title != NULL ? title : "Rogue",
		    MB_OK | MB_ICONINFORMATION);
	return TRUE;
#endif
    }
#endif

    (void) title;
    (void) message;
    return FALSE;
}

void
rogue_frontend_shutdown(void)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	rogue_allegro_shutdown();
#endif
}

bool
rogue_frontend_is_tiles(void)
{
    return (bool)(frontend_kind == ROGUE_FRONTEND_ALLEGRO);
}

bool
rogue_frontend_smoke_requested(void)
{
    return smoke_requested;
}
