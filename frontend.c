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
#include "variant.h"

static ROGUE_FRONTEND_KIND frontend_kind = ROGUE_FRONTEND_CURSES;
static bool tiles_requested = FALSE;
static bool smoke_requested = FALSE;
static bool shader_smoke_requested = FALSE;
static bool launcher_restart_requested = FALSE;

#ifdef ROGUE_ENABLE_ALLEGRO
bool rogue_allegro_start(bool smoke);
const char *rogue_allegro_choose_variant(void);
void rogue_allegro_enable_shader_smoke(void);
void rogue_allegro_render(void);
char rogue_allegro_readchar(void);
void rogue_allegro_show_prompt(const char *prompt);
void rogue_allegro_clear_prompt(void);
void rogue_allegro_record_message(const char *message);
void rogue_allegro_record_damage(int dealt, int taken, int enemy_hp,
				 int enemy_max_hp);
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

static void
launch_tiles_launcher(void)
{
#ifdef _WIN32
    char exe_path[MAX_PATH];
    char command_line[MAX_PATH + 32];
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;

    if (GetModuleFileNameA(NULL, exe_path, sizeof(exe_path)) == 0)
	return;
    exe_path[sizeof(exe_path) - 1] = '\0';
    snprintf(command_line, sizeof(command_line), "\"%s\"%s", exe_path,
	     rogue_frontend_default_tiles_for_executable(exe_path)
	     ? "" : " --tiles");
    command_line[sizeof(command_line) - 1] = '\0';

    memset(&startup, 0, sizeof(startup));
    memset(&process, 0, sizeof(process));
    startup.cb = sizeof(startup);
    if (CreateProcessA(NULL, command_line, NULL, NULL, FALSE, 0,
		       NULL, NULL, &startup, &process))
    {
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
    }
#endif
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

#ifdef ROGUE_ANDROID
    tiles_requested = TRUE;
#endif

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
	else if (strcmp(argv[read_idx], "--tiles-shader-smoke") == 0)
	{
	    tiles_requested = TRUE;
	    smoke_requested = TRUE;
	    shader_smoke_requested = TRUE;
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
    {
	if (shader_smoke_requested)
	    rogue_allegro_enable_shader_smoke();
	return rogue_allegro_start(smoke_requested);
    }
#endif

    return TRUE;
}

bool
rogue_frontend_choose_variant(void)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    const char *id;

    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
    {
	if (smoke_requested)
	    return TRUE;
	id = rogue_allegro_choose_variant();
	if (id == NULL)
	    return FALSE;
	return rogue_variant_select(id);
    }
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
rogue_frontend_record_message(const char *message)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	rogue_allegro_record_message(message);
#endif

    (void) message;
}

void
rogue_frontend_record_damage(int dealt, int taken, int enemy_hp,
			     int enemy_max_hp)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	rogue_allegro_record_damage(dealt, taken, enemy_hp, enemy_max_hp);
#endif

    (void) dealt;
    (void) taken;
    (void) enemy_hp;
    (void) enemy_max_hp;
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
rogue_frontend_request_launcher_restart(void)
{
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
	launcher_restart_requested = TRUE;
}

void
rogue_frontend_shutdown(void)
{
#ifdef ROGUE_ENABLE_ALLEGRO
    if (frontend_kind == ROGUE_FRONTEND_ALLEGRO)
    {
	rogue_allegro_shutdown();
	if (launcher_restart_requested)
	{
	    launcher_restart_requested = FALSE;
	    launch_tiles_launcher();
	}
    }
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
