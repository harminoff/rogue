import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class CombatDamageLogTests(unittest.TestCase):
    def test_combat_damage_hook_is_wired_to_gui_log(self):
        frontend_h = (ROOT / "frontend.h").read_text(encoding="utf-8")
        frontend_c = (ROOT / "frontend.c").read_text(encoding="utf-8")
        fight_c = (ROOT / "fight.c").read_text(encoding="utf-8")
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn(
            "void rogue_frontend_record_damage(int dealt, int taken, int enemy_hp,",
            frontend_h,
        )
        self.assertIn(
            "void rogue_allegro_record_damage(int dealt, int taken, int enemy_hp,",
            frontend_c,
        )
        self.assertIn(
            "rogue_frontend_record_damage(damage_done, 0,",
            fight_c,
        )
        self.assertIn("tp->t_stats.s_maxhp", fight_c)
        self.assertIn("rogue_frontend_record_damage(0, damage_taken, 0, 0);", fight_c)
        self.assertIn("Damage dealt: %d", allegro_c)
        self.assertIn("Damage taken: %d", allegro_c)
        self.assertIn("Enemy HP: %d/%d", allegro_c)
        self.assertIn("draw_combat_log_entry", allegro_c)
        self.assertLess(
            allegro_c.index("Damage dealt: %d"),
            allegro_c.index("Damage taken: %d"),
        )
        record_message = allegro_c[
            allegro_c.index("rogue_allegro_record_message"):
            allegro_c.index("rogue_allegro_record_damage")
        ]
        self.assertIn("rogue_allegro_render();", record_message)

    def test_bottom_message_is_hidden_when_side_log_is_enabled(self):
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
        draw_status = allegro_c[
            allegro_c.index("draw_status(void)"):
            allegro_c.index("ascii_lower_char")
        ]

        self.assertIn("if (!settings.side_panel_log_enabled)", draw_status)
        self.assertLess(
            draw_status.index("if (!settings.side_panel_log_enabled)"),
            draw_status.index("status.message"),
        )

    def test_bottom_bar_stylized_setting_is_available(self):
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn("stylized_bottom_bar_enabled", allegro_c)
        self.assertIn('"stylizedBottomBar"', allegro_c)
        self.assertIn("Stylized Bottom Bar", allegro_c)
        self.assertIn("draw_stylized_status_line", allegro_c)
        draw_status = allegro_c[
            allegro_c.index("draw_status(void)"):
            allegro_c.index("ascii_lower_char")
        ]
        self.assertIn("if (settings.stylized_bottom_bar_enabled)", draw_status)

    def test_blood_splatter_is_limited_to_walkable_tiles(self):
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn("blood_tile_is_walkable", allegro_c)
        self.assertIn("find_blood_splat_cell", allegro_c)

        walkable = allegro_c[
            allegro_c.index("blood_tile_is_walkable"):
            allegro_c.index("clear_blood_splats")
        ]
        self.assertIn("rogue_variant_cell_walkable(y, x)", walkable)
        self.assertNotIn("isupper", walkable)

        add_splat = allegro_c[
            allegro_c.index("add_blood_splat"):
            allegro_c.index("spawn_blood_spatter")
        ]
        self.assertIn("blood_tile_is_walkable(y, x)", add_splat)

        spawn_splat = allegro_c[
            allegro_c.index("spawn_blood_spatter"):
            allegro_c.index("draw_blood_splats")
        ]
        self.assertIn("find_blood_splat_cell(&y, &x)", spawn_splat)

    def test_actor_foreground_draws_after_blood(self):
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn("draw_actor_foreground_cell", allegro_c)
        render = allegro_c[
            allegro_c.index("void\nrogue_allegro_render(void)"):
            allegro_c.index("char\nrogue_allegro_readchar")
        ]
        self.assertLess(
            render.index("draw_blood_splats(left, top, rows, cols);"),
            render.index("draw_actor_foreground_cell"),
        )

        foreground = allegro_c[
            allegro_c.index("draw_actor_foreground_cell"):
            allegro_c.index("static int\nstatus_piece_width")
        ]
        self.assertIn("cell->layer != ROGUE_TILE_ACTOR", foreground)
        self.assertIn("draw_glyph_foreground_cell", foreground)
        self.assertIn("draw_cell_underlay_or_default(cell, dx, dy);", foreground)
        self.assertLess(
            foreground.index("draw_cell_underlay_or_default(cell, dx, dy);"),
            foreground.index("draw_atlas_tile_foreground(atlas_index, dx, dy);"),
        )

    def test_wall_thickness_is_a_persisted_setting(self):
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn("wall_thickness", allegro_c)
        self.assertIn('"wallThickness"', allegro_c)
        self.assertIn("json_int_field", allegro_c)
        self.assertIn("wall_thickness_name", allegro_c)
        self.assertIn("Wall Thickness", allegro_c)
        self.assertIn("cycle_wall_thickness", allegro_c)
        self.assertIn("#define ROGUE_MAX_WALL_THICKNESS 4", allegro_c)
        self.assertIn('return "Full";', allegro_c)

        draw_wall = allegro_c[
            allegro_c.index("draw_wall_edge_cell"):
            allegro_c.index("draw_tile_cell")
        ]
        self.assertIn("wall_thickness_pixels()", draw_wall)
        self.assertIn("wall_thickness_source_pixels(source_w, source_h)", draw_wall)
        self.assertNotIn("ROGUE_TILE_DRAW_SIZE / 6", draw_wall)
        self.assertNotIn("source_w / 6", draw_wall)

        thickness = allegro_c[
            allegro_c.index("wall_thickness_from_size"):
            allegro_c.index("wall_thickness_pixels")
        ]
        self.assertIn("case ROGUE_FULL_WALL_THICKNESS", thickness)
        self.assertIn("return size;", thickness)

    def test_dungeon_gloom_shader_pipeline_exists(self):
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn("gloom_shader", allegro_c)
        self.assertIn("scene_bitmap", allegro_c)
        self.assertIn("ensure_scene_bitmap", allegro_c)
        self.assertIn("ensure_gloom_shader", allegro_c)
        self.assertIn("draw_scene_with_gloom_shader", allegro_c)
        self.assertIn("ROGUE_GLOOM_STRENGTH", allegro_c)
        self.assertIn("ROGUE_GLOOM_RADIUS", allegro_c)
        self.assertIn("u_gloom_strength", allegro_c)
        self.assertIn("u_gloom_radius", allegro_c)

        render = allegro_c[
            allegro_c.index("void\nrogue_allegro_render(void)"):
            allegro_c.index("char\nrogue_allegro_readchar")
        ]
        self.assertIn("draw_scene_with_gloom_shader();", render)
        self.assertLess(
            render.index("draw_scene_with_gloom_shader();"),
            render.index("draw_status();"),
        )

    def test_shader_settings_are_per_effect_and_draw_overlay(self):
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn("dungeon_gloom_enabled", allegro_c)
        self.assertIn('"dungeonGloom"', allegro_c)
        self.assertIn("show_shader_settings_menu", allegro_c)
        self.assertIn("Shader Settings", allegro_c)
        self.assertIn("Dungeon Gloom", allegro_c)
        self.assertNotIn("settings.shader_enabled", allegro_c)

        draw_shader = allegro_c[
            allegro_c.index("draw_scene_with_gloom_shader"):
            allegro_c.index("show_shader_settings_menu")
        ]
        self.assertIn("settings.dungeon_gloom_enabled", draw_shader)
        self.assertIn("u_screen_size", draw_shader)
        self.assertIn("al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA", draw_shader)
        self.assertIn("al_draw_filled_rectangle", draw_shader)
        self.assertIn("shader_uniforms_ready", draw_shader)
        self.assertLess(
            draw_shader.index("al_draw_bitmap(scene_bitmap, 0, 0, 0);"),
            draw_shader.index("if (settings.dungeon_gloom_enabled"),
        )

    def test_shader_diagnostic_smoke_saves_render_targets(self):
        frontend_c = (ROOT / "frontend.c").read_text(encoding="utf-8")
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn("--tiles-shader-smoke", frontend_c)
        self.assertIn("shader_smoke_requested", frontend_c)
        self.assertIn("rogue_allegro_enable_shader_smoke", frontend_c)
        shader_smoke_block = allegro_c[
            allegro_c.index("if (shader_smoke_mode)"):
            allegro_c.index("if (!al_init())")
        ]
        self.assertIn("shader_smoke_mode", allegro_c)
        self.assertIn("settings.pixel_sharpen_enabled = TRUE", allegro_c)
        self.assertIn("settings.posterize_enabled = TRUE", allegro_c)
        self.assertIn(
            "settings.crt_effect_mode = ROGUE_CRT_DRAMATIC",
            shader_smoke_block,
        )
        self.assertIn("rogue_scene_before_shader.png", allegro_c)
        self.assertIn("rogue_scene_source_shader.png", allegro_c)
        self.assertIn("rogue_scene_after_postprocess.png", allegro_c)
        self.assertIn("rogue_scene_after_shader.png", allegro_c)
        self.assertIn("al_save_bitmap", allegro_c)

    def test_damage_flash_and_low_hp_pulse_settings_and_render_hooks(self):
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn("damage_flash_enabled", allegro_c)
        self.assertIn("low_hp_pulse_enabled", allegro_c)
        self.assertIn('"damageFlash"', allegro_c)
        self.assertIn('"lowHpPulse"', allegro_c)
        self.assertIn("Damage Flash", allegro_c)
        self.assertIn("Low HP Pulse", allegro_c)

        record_start = allegro_c.index("void\nrogue_allegro_record_damage")
        record_damage = allegro_c[
            record_start:
            allegro_c.index("void\nrogue_allegro_text_overlay_begin", record_start)
        ]
        self.assertIn("damage_flash_until", record_damage)
        self.assertIn("ROGUE_DAMAGE_FLASH_SECONDS", record_damage)

        render = allegro_c[
            allegro_c.index("void\nrogue_allegro_render(void)"):
            allegro_c.index("char\nrogue_allegro_readchar")
        ]
        self.assertIn("draw_visual_effect_overlays();", render)
        self.assertLess(
            render.index("draw_visual_effect_overlays();"),
            render.index("draw_status();"),
        )
        self.assertIn("draw_damage_flash_overlay", allegro_c)
        self.assertIn("draw_low_hp_pulse_overlay", allegro_c)
        self.assertIn("low_hp_pulse_alpha", allegro_c)

    def test_pixel_sharpen_and_posterize_postprocess_settings(self):
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn("pixel_sharpen_enabled", allegro_c)
        self.assertIn("posterize_enabled", allegro_c)
        self.assertIn('"pixelSharpen"', allegro_c)
        self.assertIn('"posterize"', allegro_c)
        self.assertIn("Pixel Sharpen", allegro_c)
        self.assertIn("Posterize", allegro_c)

        self.assertIn("draw_scene_with_postprocess_shader", allegro_c)
        self.assertIn("postprocess_pixel_shader_source", allegro_c)
        self.assertIn("u_pixel_sharpen_enabled", allegro_c)
        self.assertIn("u_posterize_enabled", allegro_c)
        self.assertIn("u_scene_texture", allegro_c)
        self.assertIn("u_scene_texel_size", allegro_c)
        self.assertIn('scene_source_bitmap', allegro_c)
        self.assertIn("channel_clamp", allegro_c)
        self.assertIn("posterize_channel", allegro_c)
        postprocess_shader_source = allegro_c[
            allegro_c.index("postprocess_pixel_shader_source"):
            allegro_c.index("gloom_pixel_shader_source")
        ]
        postprocess_draw = allegro_c[
            allegro_c.index("draw_scene_with_postprocess_shader"):
            allegro_c.index("draw_scene_with_gloom_shader")
        ]
        self.assertIn("gl_FragCoord", postprocess_shader_source)
        self.assertIn("al_lock_bitmap(scene_source_bitmap", postprocess_draw)
        self.assertIn("ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE", postprocess_draw)
        self.assertIn("read_locked_rgba", postprocess_draw)
        self.assertIn("write_locked_rgba", postprocess_draw)
        self.assertIn("al_draw_bitmap(scene_bitmap", postprocess_draw)
        self.assertNotIn("al_use_shader(postprocess_shader)", postprocess_draw)

        render = allegro_c[
            allegro_c.index("void\nrogue_allegro_render(void)"):
            allegro_c.index("char\nrogue_allegro_readchar")
        ]
        postprocess_enabled = allegro_c[
            allegro_c.index("postprocess_enabled"):
            allegro_c.index("postprocess_pixel_shader_source")
        ]
        self.assertIn("settings.pixel_sharpen_enabled", postprocess_enabled)
        self.assertIn("settings.posterize_enabled", postprocess_enabled)
        self.assertIn("postprocess_enabled()", postprocess_enabled)
        self.assertIn("scene_effects_need_bitmap()", render)
        self.assertLess(
            render.index("save_shader_smoke_bitmap"),
            render.index("draw_scene_with_gloom_shader();"),
        )

    def test_readme_lists_crt_effect_shader_setting(self):
        readme = (ROOT / "README.md").read_text(encoding="utf-8")

        self.assertIn("CRT Effect", readme)
        self.assertIn("Off/Subtle/Balanced/Dramatic", readme)

    def test_crt_effect_modes_are_persisted_and_menu_driven(self):
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn("ROGUE_CRT_OFF", allegro_c)
        self.assertIn("ROGUE_CRT_SUBTLE", allegro_c)
        self.assertIn("ROGUE_CRT_BALANCED", allegro_c)
        self.assertIn("ROGUE_CRT_DRAMATIC", allegro_c)
        self.assertIn("crt_effect_mode", allegro_c)
        self.assertIn('"crtEffect"', allegro_c)
        self.assertIn("crt_effect_label", allegro_c)
        self.assertIn("cycle_crt_effect_mode", allegro_c)
        self.assertIn("CRT Effect", allegro_c)

        settings_struct = allegro_c[
            allegro_c.index("typedef struct rogue_allegro_settings {"):
            allegro_c.index("} ROGUE_ALLEGRO_SETTINGS;")
        ]
        settings_initializer = allegro_c[
            allegro_c.index("static ROGUE_ALLEGRO_SETTINGS settings = {"):
            allegro_c.index("#define ROGUE_TILE_DRAW_SIZE")
        ]
        settings_fields = [
            line.strip().split()[-1].rstrip(";")
            for line in settings_struct.splitlines()
            if line.strip().endswith(";")
        ]
        settings_values = [
            line.strip().rstrip(",")
            for line in settings_initializer.splitlines()
            if line.strip()
            and not line.strip().startswith("static ")
            and line.strip() != "};"
        ]
        settings_defaults = dict(zip(settings_fields, settings_values))
        self.assertEqual(len(settings_fields), len(settings_values))
        self.assertEqual("FALSE", settings_defaults["posterize_enabled"])
        self.assertEqual("ROGUE_CRT_OFF", settings_defaults["crt_effect_mode"])

        settings_menu = allegro_c[
            allegro_c.index("show_shader_settings_menu"):
            allegro_c.index("show_tilepack_menu")
        ]
        self.assertIn('"f) CRT Effect: %s"', settings_menu)
        self.assertIn("crt_effect_label(settings.crt_effect_mode)", settings_menu)
        self.assertIn("cycle_crt_effect_mode();", settings_menu)
        self.assertIn("save_settings();", settings_menu)

        cycle = allegro_c[
            allegro_c.index("cycle_crt_effect_mode"):
            allegro_c.index("static bool\npostprocess_enabled")
        ]
        self.assertIn("ROGUE_CRT_SUBTLE", cycle)
        self.assertIn("ROGUE_CRT_BALANCED", cycle)
        self.assertIn("ROGUE_CRT_DRAMATIC", cycle)
        self.assertIn("ROGUE_CRT_OFF", cycle)

    def test_crt_effect_participates_in_postprocess_pipeline(self):
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        postprocess_enabled = allegro_c[
            allegro_c.index("postprocess_enabled"):
            allegro_c.index("postprocess_pixel_shader_source")
        ]
        self.assertIn("settings.crt_effect_mode != ROGUE_CRT_OFF", postprocess_enabled)

        shader_source = allegro_c[
            allegro_c.index("postprocess_pixel_shader_source"):
            allegro_c.index("gloom_pixel_shader_source")
        ]
        self.assertIn("u_crt_effect_mode", shader_source)
        self.assertIn("u_crt_scanline_strength", shader_source)
        self.assertIn("u_crt_vignette_strength", shader_source)
        self.assertIn("u_crt_curvature", shader_source)
        self.assertIn("u_crt_rgb_offset", shader_source)
        self.assertIn("sample_uv", shader_source)
        self.assertIn("scanline", shader_source)

        postprocess_draw = allegro_c[
            allegro_c.index("draw_scene_with_postprocess_shader"):
            allegro_c.index("draw_scene_with_gloom_shader")
        ]
        self.assertIn("crt_sample_coordinates", postprocess_draw)
        self.assertIn("crt_scanline_factor", postprocess_draw)
        self.assertIn("crt_vignette_factor", postprocess_draw)
        self.assertIn("crt_rgb_offset_pixels", postprocess_draw)
        self.assertIn("settings.crt_effect_mode", postprocess_draw)
        self.assertIn("write_locked_rgba(target_lock, x, y, r, g, b, a);", postprocess_draw)
        self.assertIn("read_locked_rgba(source_lock, sample_x,", postprocess_draw)
        self.assertIn("sample_y > 0 ? sample_y - 1 : sample_y", postprocess_draw)
        self.assertIn("sample_x + 1 < scene_bitmap_width", postprocess_draw)

    def test_postprocess_shader_uses_allegro_default_vertex_shader(self):
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
        postprocess = allegro_c[
            allegro_c.index("ensure_postprocess_shader"):
            allegro_c.index("draw_scene_with_postprocess_shader")
        ]

        self.assertIn("al_get_shader_platform", postprocess)
        self.assertIn("al_get_default_shader_source", postprocess)
        self.assertIn("ALLEGRO_VERTEX_SHADER", postprocess)
        self.assertNotIn("postprocess_vertex_shader_source", allegro_c)

    def test_optional_enemy_health_overlay_uses_visible_monster_stats(self):
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn("enemy_health_overlay_enabled", allegro_c)
        self.assertIn('"enemyHealthOverlay"', allegro_c)
        self.assertIn("Enemy Health Overlay", allegro_c)
        self.assertIn("draw_enemy_health_overlays", allegro_c)
        self.assertIn("draw_enemy_health_overlay_cell", allegro_c)

        overlay = allegro_c[
            allegro_c.index("draw_enemy_health_overlay_cell"):
            allegro_c.index("draw_enemy_health_overlays")
        ]
        self.assertIn("moat(cell->y, cell->x)", overlay)
        self.assertIn("monster->t_stats.s_hpt", overlay)
        self.assertIn("monster->t_stats.s_maxhp", overlay)
        self.assertIn("monster->t_stats.s_lvl", overlay)
        self.assertIn("monster->t_stats.s_arm", overlay)
        self.assertIn("monster->t_disguise != monster->t_type", overlay)

        render = allegro_c[
            allegro_c.index("void\nrogue_allegro_render(void)"):
            allegro_c.index("char\nrogue_allegro_readchar")
        ]
        self.assertIn("draw_enemy_health_overlays(view, rows, cols);", render)
        self.assertLess(
            render.index("draw_actor_foreground_cell"),
            render.index("draw_enemy_health_overlays(view, rows, cols);"),
        )


if __name__ == "__main__":
    unittest.main()
