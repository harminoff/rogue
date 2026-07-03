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
            draw_status.index("huh"),
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
        self.assertIn("case ' '", walkable)
        self.assertIn("case '|'", walkable)
        self.assertIn("case '-'", walkable)
        self.assertIn("return FALSE", walkable)
        self.assertIn("case FLOOR", walkable)
        self.assertIn("case PASSAGE", walkable)
        self.assertIn("case DOOR", walkable)
        self.assertIn("case TRAP", walkable)
        self.assertIn("case STAIRS", walkable)
        self.assertIn("default:", walkable)
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
        self.assertNotIn("underlay", foreground)

    def test_wall_thickness_is_a_persisted_setting(self):
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn("wall_thickness", allegro_c)
        self.assertIn('"wallThickness"', allegro_c)
        self.assertIn("json_int_field", allegro_c)
        self.assertIn("wall_thickness_name", allegro_c)
        self.assertIn("Wall Thickness", allegro_c)
        self.assertIn("cycle_wall_thickness", allegro_c)

        draw_wall = allegro_c[
            allegro_c.index("draw_wall_edge_cell"):
            allegro_c.index("draw_tile_cell")
        ]
        self.assertIn("wall_thickness_pixels()", draw_wall)
        self.assertIn("wall_thickness_source_pixels(source_w, source_h)", draw_wall)
        self.assertNotIn("ROGUE_TILE_DRAW_SIZE / 6", draw_wall)
        self.assertNotIn("source_w / 6", draw_wall)


if __name__ == "__main__":
    unittest.main()
