from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def _struct_body(text, struct_name):
    match = re.search(
        rf"typedef struct {struct_name} \{{(?P<body>.*?)\}}\s+\w+;",
        text,
        re.DOTALL,
    )
    assert match is not None
    return match.group("body")


def test_allegro_frontend_installs_touch_input():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert "al_install_touch_input()" in text
    assert "al_get_touch_input_event_source()" in text
    assert "ALLEGRO_EVENT_TOUCH_BEGIN" in text


def test_allegro_frontend_uses_mobile_controls_module():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert '#include "mobile_controls.h"' in text
    assert "ROGUE_MOBILE_LAYOUT" in text
    assert "rogue_mobile_layout_build" in text
    assert "rogue_mobile_movement_index_at" in text
    assert "rogue_mobile_action_index_at" in text


def test_mobile_reference_dpad_and_command_grid_are_used():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    controls = (ROOT / "mobile_controls.c").read_text(encoding="utf-8")
    assert "assets/mobile/dpad_reference.png" in text
    assert "draw_mobile_dpad" in text
    assert '"WIELD"' in controls
    assert '"PICK\\nUP"' in controls
    assert '"DESC"' in controls
    assert '"INVEN"' in controls
    assert '"QUAFF"' in controls
    assert '"READ"' in controls
    assert '"WAIT"' not in controls
    assert '"OPEN"' not in controls
    assert '"DOOR"' not in controls


def test_overlay_tiles_use_windows_underlay_rendering_contract():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert "draw_atlas_tile_foreground" in text
    assert "foreground_pixel_visible" in text
    visible_body = text[text.index("foreground_pixel_visible"):
                        text.index("foreground_background_pixel")]
    assert "r <= 32 && g <= 32 && b <= 32" in visible_body
    assert "foreground_background_pixel" in text
    assert "foreground_mark_edge_background" in text
    assert "background[index]" in text
    foreground_draw_body = text[text.index("draw_atlas_tile_foreground"):
                                text.index("resolved_cell_index")]
    assert "draw_atlas_tile_foreground_pixel" in text
    assert "al_draw_filled_rectangle" in foreground_draw_body
    assert "al_draw_scaled_bitmap(masked" not in foreground_draw_body
    assert "draw_object_foreground_cell" in text
    underlay_helper = text[text.index("draw_cell_underlay_or_default"):
                           text.index("is_wall_cell")]
    assert "atlas_tile_is_visible(underlay_index)" in underlay_helper

    tile_body = text[text.index("draw_tile_cell"):
                     text.index("draw_object_foreground_cell")]
    assert "if (cell->has_underlay && underlay_index >= 0 && atlas != NULL)" in tile_body
    assert "draw_atlas_tile_for_role(underlay_index, cell->under_role, dx, dy);" in tile_body
    assert "draw_cell_underlay_or_default(cell, dx, dy);" in tile_body
    assert "if (cell->layer == ROGUE_TILE_OBJECT)" in tile_body
    assert "return;" in tile_body

    object_body = text[text.index("draw_object_foreground_cell"):
                       text.index("draw_actor_foreground_cell")]
    assert "draw_cell_underlay_or_default(cell, dx, dy);" in object_body
    assert (object_body.index("draw_cell_underlay_or_default(cell, dx, dy);")
            < object_body.index("draw_atlas_tile_foreground(atlas_index, dx, dy);"))

    render_start = text.index("void\nrogue_allegro_render(void)")
    render_body = text[render_start:text.index("draw_enemy_health_overlays", render_start)]
    assert "draw_object_foreground_cell" in render_body
    assert "draw_blood_splats(left, top, rows, cols);" in render_body
    assert "draw_actor_foreground_cell" in render_body
    assert render_body.index("draw_object_foreground_cell") < render_body.index("draw_blood_splats")

    actor_body = text[text.index("draw_actor_foreground_cell"):
                      text.index("draw_enemy_health_overlay_cell")]
    assert "cell->layer != ROGUE_TILE_ACTOR" in actor_body
    assert "draw_cell_underlay_or_default(cell, dx, dy);" in actor_body
    assert (actor_body.index("draw_cell_underlay_or_default(cell, dx, dy);")
            < actor_body.index("draw_atlas_tile_foreground(atlas_index, dx, dy);"))
    assert "draw_atlas_tile_foreground(atlas_index, dx, dy);" in actor_body
    assert "draw_glyph_foreground_cell" in actor_body


def test_zoomed_floor_tiles_use_seam_safe_inner_source_scaling():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    seamless_body = text[text.index("draw_atlas_tile_seamless"):
                         text.index("draw_atlas_tile_for_role")]
    role_body = text[text.index("draw_atlas_tile_for_role"):
                     text.index("foreground_pixel_visible")]
    underlay_body = text[text.index("draw_cell_underlay_or_default"):
                         text.index("is_wall_cell")]
    tile_body = text[text.index("draw_tile_cell"):
                     text.index("draw_object_foreground_cell")]

    assert "source_inset = 1" in seamless_body
    assert "sx + source_inset" in seamless_body
    assert "source_w - source_inset * 2" in seamless_body
    assert 'strcmp(role, "terrain.floor") == 0' in role_body
    assert "draw_atlas_tile_seamless" in role_body
    assert "draw_atlas_tile_for_role(underlay_index, cell->under_role" in underlay_body
    assert "draw_atlas_tile_for_role(atlas_index, cell->role" in tile_body
    assert "draw_atlas_tile_foreground(atlas_index, dx, dy);" in text


def test_mobile_portrait_controls_have_scrolled_actions_and_menu_select():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    controls = (ROOT / "mobile_controls.c").read_text(encoding="utf-8")
    assert "#define ROGUE_MOBILE_ACTION_COLS 3" in controls
    assert "#define ROGUE_MOBILE_ACTION_VISIBLE_ROWS 3" in controls
    assert "mobile_action_scroll_y" in text
    assert "mobile_action_touch_scrolled" in text
    assert "mobile_log_visible_lines" in text
    assert "mobile_log_scroll_y" in text
    assert "ROGUE_MOBILE_HUD_FONT_SIZE" in text
    assert '"SELECT"' in text


def test_mobile_actions_include_manuals_and_options_buttons():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    controls = (ROOT / "mobile_controls.c").read_text(encoding="utf-8")
    header = (ROOT / "mobile_controls.h").read_text(encoding="utf-8")
    assert "#define ROGUE_MOBILE_ACTION_BUTTON_COUNT 30" in header
    assert "ROGUE_MOBILE_COMMAND_MANUALS" in controls
    assert "ROGUE_MOBILE_COMMAND_OPTIONS" in controls
    assert "ROGUE_MOBILE_COMMAND_CLOSE" in controls
    assert '"READ\\nBOOK"' in controls
    assert '"OPTS"' in controls
    assert '"CLOSE"' in controls
    assert "handle_mobile_special_action(command)" in text
    assert "show_manuals_menu();" in text
    assert "show_settings_menu();" in text
    assert "return ESCAPE;" in text


def test_super_rogue_mobile_actions_include_variant_commands():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    controls = (ROOT / "mobile_controls.c").read_text(encoding="utf-8")
    header = (ROOT / "mobile_controls.h").read_text(encoding="utf-8")

    assert "rogue_mobile_action_bar_build_for_variant" in header
    assert "rogue_mobile_action_bar_build_with_context(&mobile_layout" in text
    assert 'rogue_variant_current()->id' in text
    assert 'strcmp(variant_id, "srogue90") == 0' in controls
    for command in [
        "'<'", "'z'", "'p'", "'T'", "'P'", "'R'", "'c'", "'a'",
        "'D'", "'$'", "'#'", "'%'",
    ]:
        assert command in controls
    for label in [
        '"ASC"', '"ZAP"', '"DIR\\nZAP"', '"TAKE\\nOFF"', '"PUT\\nRING"',
        '"REM\\nRING"', '"CALL"', '"MAX"', '"DIP"', '"PRICE"', '"BUY"',
        '"SELL"',
    ]:
        assert label in controls


def test_standard_rogue_mobile_actions_include_variant_commands():
    controls = (ROOT / "mobile_controls.c").read_text(encoding="utf-8")
    header = (ROOT / "mobile_controls.h").read_text(encoding="utf-8")

    assert "#define ROGUE_MOBILE_ACTION_BUTTON_COUNT 30" in header
    assert 'strcmp(variant_id, "rogue54") == 0' in controls
    assert 'strcmp(variant_id, "rogue52") == 0' in controls
    assert 'strcmp(variant_id, "rogue36") == 0' in controls
    for command in [
        "','", "'<'", "'z'", "'p'", "'T'", "'P'", "'R'", "'c'",
        "'D'", "'^'", "'m'", "')'", "']'", "'='", "'@'", "'o'",
    ]:
        assert command in controls
    for label in [
        '"PICK\\nUP"', '"ASC"', '"ZAP"', '"DIR\\nZAP"', '"TAKE\\nOFF"',
        '"PUT\\nRING"', '"REM\\nRING"', '"CALL"', '"DISC"', '"TRAP"',
        '"NO\\nPICK"', '"WEP"', '"ARM"', '"RINGS"', '"STATS"',
        '"GAME\\nOPT"',
    ]:
        assert label in controls


def test_mobile_actions_are_filtered_by_current_game_context():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    controls = (ROOT / "mobile_controls.c").read_text(encoding="utf-8")
    header = (ROOT / "mobile_controls.h").read_text(encoding="utf-8")
    variant_header = (ROOT / "variant.h").read_text(encoding="utf-8")
    variant_c = (ROOT / "variant.c").read_text(encoding="utf-8")
    tiles = (ROOT / "tiles.c").read_text(encoding="utf-8")

    assert "ROGUE_MOBILE_ACTION_CONTEXT" in header
    assert "rogue_mobile_action_bar_build_with_context" in header
    assert "action_visible_for_context" in controls
    assert "ROGUE_MOBILE_ACTION_TRADE" in controls
    assert "ROGUE_MOBILE_ACTION_POOL" in controls
    assert "ROGUE_MOBILE_ACTION_STAIRS" in controls
    assert "ROGUE_MOBILE_ACTION_OBJECT" in controls
    assert "rogue_variant_action_context" in variant_header
    assert "rogue_variant_action_context" in variant_c
    assert "rogue_variant_action_context" in text
    assert "rogue_mobile_action_bar_build_with_context(&mobile_layout" in text
    assert "srogue90_bridge_level_type" in tiles
    assert "context->in_trading_post" in tiles
    assert "context->on_magic_pool" in tiles


def test_shared_tile_and_variant_structs_do_not_use_ambiguous_bool_fields():
    tile_header = (ROOT / "tiles.h").read_text(encoding="utf-8")
    variant_header = (ROOT / "variant.h").read_text(encoding="utf-8")

    for text, struct_name in [
        (tile_header, "rogue_tile_cell"),
        (variant_header, "rogue_variant_status"),
        (variant_header, "rogue_variant_action_context"),
    ]:
        body = _struct_body(text, struct_name)
        assert re.search(r"\bbool\b", body) is None


def test_mobile_stats_show_super_rogue_extended_values():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    stats_body = text[text.index("draw_mobile_stats"):text.index("mobile_log_visible_lines")]

    assert "draw_mobile_stats_line" in text
    assert "status.has_extended_stats" in stats_body
    assert '"DEX:%s  WIS:%s  CON:%s"' in stats_body
    assert '"CARRY:%d/%d  VOL:%d%%  XP:%ld"' in stats_body
    assert "format_stat_pair" in stats_body


def test_mobile_stats_show_standard_rogue_exp_and_hunger():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    stats_body = text[text.index("draw_mobile_stats"):text.index("mobile_log_visible_lines")]

    assert '"XP:%d/%ld"' in stats_body
    assert "status.exp_level" in stats_body
    assert "status.exp_points" in stats_body
    assert "status.hungry_state > 0" in stats_body
    assert "state_name[status.hungry_state]" in stats_body


def test_mobile_action_buttons_use_large_game_font():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert "#define ROGUE_MOBILE_ACTION_FONT_SIZE" in text
    assert "static ALLEGRO_FONT *mobile_action_font = NULL;" in text
    assert "load_mobile_action_font" in text
    action_body = text[text.index("draw_mobile_action_bar"):
                       text.index("draw_mobile_dpad")]
    assert "mobile_action_font != NULL" in action_body
    assert "ui_font = font;" not in action_body


def test_mobile_dpad_center_uses_button_font_without_select_box():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    dpad_body = text[text.index("draw_mobile_dpad"):
                     text.index("draw_mobile_controls")]
    assert "draw_mobile_dpad_center_label" in text
    assert "mobile_action_font != NULL" in dpad_body
    assert '"SELECT"' in dpad_body
    assert '"WAIT"' in dpad_body
    assert '"ENTER"' in dpad_body
    assert "death_overlay_active" in dpad_body
    assert "al_map_rgb(255, 225, 120)" not in dpad_body


def test_mobile_death_overlay_center_button_sends_enter():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    readchar_body = text[text.index("rogue_allegro_readchar"):
                         text.index("rogue_allegro_show_prompt")]
    assert "death_overlay_active" in readchar_body
    assert "pressed_index == 4" in readchar_body
    assert "return '\\n';" in readchar_body


def test_android_variant_picker_has_touch_selection_and_start_target():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    picker_body = text[text.index("rogue_allegro_choose_variant"):
                       text.index("rogue_allegro_prepare_game_start")]
    assert "variant_picker_index_at_point" in text
    assert "variant_picker_start_hit" in text
    assert "variant_picker_start_geometry" in text
    assert "mobile_bottom_safe_height()" in text[text.index("variant_picker_start_geometry"):
                                                 text.index("variant_picker_start_hit")]
    assert "ALLEGRO_EVENT_TOUCH_BEGIN" in picker_body
    assert "ALLEGRO_EVENT_MOUSE_BUTTON_DOWN" in picker_body
    assert "return variant == NULL ? rogue_variant_default()->id : variant->id;" in picker_body


def test_android_variant_tiles_do_not_mark_full_rogue36_or_srogue_maps_seen():
    text = (ROOT / "tiles.c").read_text(encoding="utf-8")
    rogue36_body = text[text.index("rogue36_tile_describe_cell"):
                        text.index("srogue90_underlay_glyph")]
    srogue90_body = text[text.index("srogue90_tile_describe_cell"):
                         text.index("rogue52_variant_status")]

    assert "seen = FALSE;" in rogue36_body
    assert "seen = FALSE;" in srogue90_body
    assert "seen = (bool)(glyph != ' ' && !glyph_is_object);" not in rogue36_body
    assert "seen = (bool)(glyph != ' ' && !glyph_is_object);" not in srogue90_body
    assert "srogue90_bridge_cansee(y, x)" in srogue90_body


def test_android_variant_tiles_do_not_render_stale_player_glyphs():
    text = (ROOT / "tiles.c").read_text(encoding="utf-8")
    rogue36_body = text[text.index("rogue36_tile_describe_cell"):
                        text.index("srogue90_underlay_glyph")]
    srogue90_body = text[text.index("srogue90_tile_describe_cell"):
                         text.index("rogue52_variant_status")]

    assert "normalize_stale_player_glyph" in text
    assert "if (*glyph == PLAYER)" in text
    assert "*glyph = terrain_glyph;" in text

    rogue36_hero = "if (rogue36_bridge_hero_y() == y"
    rogue36_stale = "normalize_stale_player_glyph(&glyph, terrain_glyph);"
    assert rogue36_stale in rogue36_body
    assert rogue36_body.index(rogue36_hero) < rogue36_body.index(rogue36_stale)
    assert rogue36_body.index(rogue36_stale) < rogue36_body.index("monster = rogue36_bridge_monster_at")

    srogue90_hero = "if (srogue90_bridge_hero_y() == y"
    srogue90_stale = "normalize_stale_player_glyph(&glyph, terrain_glyph);"
    assert srogue90_stale in srogue90_body
    assert srogue90_body.index(srogue90_hero) < srogue90_body.index(srogue90_stale)
    assert srogue90_body.index(srogue90_stale) < srogue90_body.index("monster = srogue90_bridge_monster_at")


def test_mobile_search_button_records_feedback():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert "mobile_prepare_command_feedback" in text
    feedback_body = text[text.index("mobile_prepare_command_feedback"):
                         text.index("handle_mobile_special_action")]
    assert "command == 's'" in feedback_body
    assert "You search." in feedback_body


def test_mobile_zoom_uses_pinch_not_side_slider():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert "mobile_pinch_active" in text
    assert "mobile_pinch_distance" in text
    assert "mobile_begin_or_update_pinch" in text
    assert "draw_mobile_zoom_slider();" not in text
    assert "rogue_mobile_zoom_at(" not in text


def test_mobile_pinch_zoom_throttles_render_work():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert "#define ROGUE_MOBILE_PINCH_RENDER_INTERVAL" in text
    assert "mobile_pinch_render_requested" in text
    assert "mobile_pinch_last_render_at" in text
    assert "mobile_consume_pinch_render_request" in text

    set_zoom_body = text[text.index("set_zoom(int tile_draw_size)"):
                         text.index("handle_view_key")]
    assert "return FALSE;" in set_zoom_body
    assert "return TRUE;" in set_zoom_body

    pinch_body = text[text.index("mobile_begin_or_update_pinch"):
                      text.index("mobile_track_pinch_begin")]
    assert "if (set_zoom(target_zoom))" in pinch_body
    assert "mobile_pinch_render_requested = TRUE;" in pinch_body

    readchar_body = text[text.index("rogue_allegro_readchar"):
                         text.index("rogue_allegro_show_prompt")]
    assert "render_before_wait" in readchar_body
    assert "mobile_consume_pinch_render_request()" in readchar_body
    assert "render_before_wait = FALSE;" in readchar_body


def test_mobile_hud_is_stats_only_without_white_meter_or_press_fill():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    stats_body = text[text.index("draw_mobile_stats"):text.index("draw_mobile_log")]
    assert "segments = 36" not in stats_body
    assert "meter_x" not in stats_body
    assert "meter_y" not in stats_body
    assert "draw_mobile_pressed_rect" not in text
    assert "mobile_pressed_action_index" not in text
    assert "mobile_pressed_movement_index" not in text


def test_android_overlays_have_visible_close_control():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert "draw_text_overlay_close_button" in text
    assert "mobile_text_overlay_close_hit" in text
    assert "text_overlay_close_rect" in text
    show_start = text.index("char\nrogue_allegro_text_overlay_show")
    pick_start = text.index("char\nrogue_allegro_text_overlay_pick", show_start)
    show_body = text[show_start:pick_start]
    assert "mobile_text_overlay_close_hit" in show_body
    assert "ch = ESCAPE;" in show_body
    clear_start = text.index("rogue_allegro_text_overlay_clear", pick_start)
    pick_body = text[pick_start:clear_start]
    assert "mobile_text_overlay_close_hit" in pick_body
    assert "chosen = ESCAPE;" in pick_body


def test_android_manual_reader_is_fullscreen_with_chapter_button():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    draw_body = text[text.index("draw_text_overlay"):
                     text.index("draw_death_overlay")]
    reader_start = text.index("static void\nshow_manual_reader")
    reader_body = text[reader_start:
                       text.index("show_manuals_menu_for_variant",
                                  reader_start)]
    show_body = text[text.index("char\nrogue_allegro_text_overlay_show"):
                     text.index("char\nrogue_allegro_text_overlay_pick")]

    assert "text_overlay_fullscreen" in text
    assert "draw_text_overlay_chapters_button" in text
    assert "mobile_text_overlay_chapters_hit" in text
    assert "mobile_text_overlay_pick_line_at" in text
    assert "w = window_w;" in draw_body
    assert "h = window_h - y;" in draw_body
    assert "text_overlay_fullscreen = TRUE;" in reader_body
    assert "text_overlay_fullscreen = FALSE;" in reader_body
    assert '"CHAPTERS"' in text
    assert "mobile_text_overlay_chapters_hit((int) event.touch.x" in show_body
    assert "show_manual_chapter_picker()" in show_body
    assert "manual_chapter_lines[chapter_index]" in show_body


def test_android_manual_reader_wraps_text_for_mobile_width():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    add_body = text[text.index("rogue_allegro_text_overlay_add"):
                    text.index("mobile_overlay_pointer_movement_index")]

    assert "mobile_text_overlay_wrap_chars" in text
    assert "wrap_chars = mobile_text_overlay_wrap_chars();" in add_body
    assert "next_space_wrap_len(line, start, wrap_chars)" in add_body
    assert "text_overlay_fullscreen" in add_body


def test_android_text_overlays_drag_scroll_on_mobile():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    show_body = text[text.index("char\nrogue_allegro_text_overlay_show"):
                     text.index("char\nrogue_allegro_text_overlay_pick")]
    pick_body = text[text.index("char\nrogue_allegro_text_overlay_pick"):
                     text.index("void\nrogue_allegro_text_overlay_clear")]

    assert "mobile_text_overlay_touch_id" in text
    assert "mobile_text_overlay_touch_start_y" in text
    assert "mobile_text_overlay_touch_start_scroll" in text
    assert "mobile_text_overlay_touch_scrolled" in text
    assert "mobile_text_overlay_scroll_begin" in text
    assert "mobile_text_overlay_scroll_move" in text
    assert "mobile_text_overlay_scroll_end" in text
    assert "ALLEGRO_EVENT_TOUCH_MOVE" in show_body
    assert "mobile_text_overlay_scroll_move" in show_body
    assert "ALLEGRO_EVENT_TOUCH_MOVE" in pick_body
    assert "mobile_text_overlay_scroll_move" in pick_body


def test_android_chapter_picker_uses_fullscreen_overlay_on_mobile():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    chapter_start = text.index("static char\nshow_manual_chapter_picker")
    chapter_body = text[chapter_start:
                        text.index("static void\nclean_manual_text",
                                   chapter_start)]

    assert "text_overlay_fill_vertical = TRUE;" in chapter_body
    assert "text_overlay_fullscreen = TRUE;" in chapter_body
    assert "rogue_allegro_text_overlay_pick" in chapter_body


def test_android_selectable_overlays_can_be_tapped_directly():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    pick_body = text[text.index("char\nrogue_allegro_text_overlay_pick"):
                     text.index("void\nrogue_allegro_text_overlay_clear")]

    assert "mobile_text_overlay_pick_line_at((int) event.touch.x" in pick_body
    assert "mobile_text_overlay_pick_line_at(event.mouse.x" in pick_body
    assert "text_overlay_selected = tapped_line;" in pick_body
    assert "chosen = selected_key;" in pick_body


def test_android_settings_hide_desktop_only_options():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    settings_body = text[text.index("show_settings_menu"):
                         text.index("map_special_key")]
    assert "#ifndef ROGUE_ANDROID" in settings_body
    assert 'snprintf(line, sizeof(line), "a) Side Panel Log: %s"' in settings_body
    assert 'snprintf(line, sizeof(line), "c) Stylized Bottom Bar: %s"' in settings_body


def test_android_settings_can_swap_mobile_controls():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    settings_body = text[text.index("show_settings_menu"):
                         text.index("map_special_key")]
    layout_body = text[text.index("build_mobile_layout"):
                       text.index("draw_mobile_panel")]
    assert "mobile_controls_swapped" in text
    assert "mobileControlsSwapped" in text
    assert "Swap Controls" in settings_body
    assert "#ifdef ROGUE_ANDROID" in settings_body
    assert "settings.mobile_controls_swapped" in layout_body
    assert "dpad_x = action_x + action_w + mobile_gap();" in layout_body


def test_android_allows_larger_zoom_than_desktop():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert "#define ROGUE_DESKTOP_MAX_TILE_DRAW_SIZE 64" in text
    assert "#define ROGUE_ANDROID_MAX_TILE_DRAW_SIZE 128" in text
    assert "#define ROGUE_MAX_TILE_DRAW_SIZE ROGUE_ANDROID_MAX_TILE_DRAW_SIZE" in text


def test_android_mobile_layout_reserves_top_safe_area():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert "mobile_top_safe_height" in text
    assert "rogue_platform_android_safe_top_inset()" in text
    assert "ROGUE_MOBILE_MIN_TOP_SAFE_HEIGHT" in text

    stats_body = text[text.index("draw_mobile_stats"):text.index("draw_mobile_log")]
    assert "y = mobile_top_safe_height();" in stats_body
    assert "draw_mobile_panel(0, y, display_width(), h);" in stats_body

    play_top_body = text[text.index("mobile_play_top"):
                         text.index("mobile_play_height")]
    assert "mobile_top_safe_height()" in play_top_body

    log_body = text[text.index("mobile_log_top"):
                    text.index("mobile_play_top")]
    assert "mobile_top_safe_height()" in log_body


def test_mobile_log_shows_more_lines_and_scrolls():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert "#define ROGUE_MOBILE_LOG_MIN_VISIBLE_LINES 7" in text
    assert "mobile_log_visible_lines" in text
    assert "mobile_log_scroll_max" in text
    assert "mobile_log_touch_id" in text
    assert "mobile_log_scroll_y" in text

    log_body = text[text.index("draw_mobile_log"):
                    text.index("draw_mobile_action_bar")]
    assert "visible_lines = mobile_log_visible_lines(line_h);" in log_body
    assert "first = message_log_count - visible_lines - mobile_log_scroll_y;" in log_body
    assert "message_log_count - 4" not in log_body

    readchar_body = text[text.index("rogue_allegro_readchar"):
                         text.index("rogue_allegro_show_prompt")]
    assert "mobile_log_index_at" in readchar_body
    assert "mobile_log_scroll_clamp" in readchar_body


def test_android_suppresses_desktop_enemy_health_panels():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    overlay_body = text[text.index("draw_enemy_health_overlay_cell"):
                        text.index("draw_enemy_health_overlays")]
    assert "#ifdef ROGUE_ANDROID" in overlay_body
    assert "return;" in overlay_body
    assert "back_color = al_map_rgba(8, 10, 14, 210);" in overlay_body


def test_android_suppresses_space_continue_prompts():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert "android_prompt_response" in text
    assert '? \'\\n\' : \' \'' in text
    assert "response = android_prompt_response" in text
    assert "return response" in text
