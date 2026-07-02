/*
 * Frontend selection for curses ASCII and graphical tile modes.
 */

#ifndef ROGUE_FRONTEND_H
#define ROGUE_FRONTEND_H

#include <curses.h>

typedef enum rogue_frontend_kind {
    ROGUE_FRONTEND_CURSES,
    ROGUE_FRONTEND_ALLEGRO
} ROGUE_FRONTEND_KIND;

bool rogue_frontend_init(int *argc, char **argv);
bool rogue_frontend_start(void);
void rogue_frontend_render(void);
char rogue_frontend_readchar(void);
void rogue_frontend_show_prompt(const char *prompt);
void rogue_frontend_clear_prompt(void);
void rogue_frontend_show_death(const char *killer, int gold, bool has_amulet);
void rogue_frontend_wait_for_return(const char *prompt);
void rogue_frontend_text_overlay_begin(const char *title);
void rogue_frontend_text_overlay_add(const char *line);
char rogue_frontend_text_overlay_show(const char *prompt);
char rogue_frontend_text_overlay_pick(const char *prompt);
void rogue_frontend_text_overlay_clear(void);
bool rogue_frontend_text_input(const char *title, const char *prompt,
			       const char *initial, char *out, int out_size);
bool rogue_frontend_confirm(const char *title, const char *prompt);
bool rogue_frontend_notice(const char *title, const char *message);
void rogue_frontend_shutdown(void);
bool rogue_frontend_is_tiles(void);
bool rogue_frontend_smoke_requested(void);
bool rogue_frontend_default_tiles_for_executable(const char *path);

#endif
