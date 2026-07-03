/*
 * Small, frontend-independent helpers for selectable text overlays.
 */

#ifndef ROGUE_OVERLAY_PICKER_H
#define ROGUE_OVERLAY_PICKER_H

char rogue_picker_line_key(const char *line);
int rogue_picker_clamp_selection(int selection, int count);
int rogue_picker_move_selection(int selection, int count, int delta);
int rogue_picker_find_key(char key, const char **lines, int count);
int rogue_picker_first_keyed_line(const char **lines, int count);
void rogue_overlay_find_bounds(const char **rows, int row_count, int col_count,
			       int *top, int *bottom, int *left, int *right);
void rogue_overlay_copy_span(const char *row, int left, int right,
			     char *out, int out_size);

#endif
