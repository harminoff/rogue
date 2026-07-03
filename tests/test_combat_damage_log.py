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


if __name__ == "__main__":
    unittest.main()
