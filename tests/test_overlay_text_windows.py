import pathlib
import re
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class OverlayTextWindowTest(unittest.TestCase):
    def test_text_overlays_wrap_long_non_selectable_lines(self):
        text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn("TEXT_OVERLAY_WRAP_CHARS", text)
        self.assertIn("append_text_overlay_line", text)
        add_match = re.search(
            r"^rogue_allegro_text_overlay_add\(const char \*line\)(.*?)^char\nrogue_allegro_text_overlay_show",
            text,
            re.S | re.M,
        )
        self.assertIsNotNone(add_match)
        add_body = add_match.group(1)
        self.assertIn("next_wrap_len(line, start, TEXT_OVERLAY_WRAP_CHARS)", add_body)
        self.assertIn("rogue_picker_line_key(line) != '\\0'", add_body)

    def test_selectable_overlays_support_page_navigation(self):
        text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        pick_match = re.search(
            r"^rogue_allegro_text_overlay_pick\(const char \*prompt\)(.*?)^void\nrogue_allegro_text_overlay_clear",
            text,
            re.S | re.M,
        )
        self.assertIsNotNone(pick_match)
        pick_body = pick_match.group(1)
        self.assertIn("visible_lines", pick_body)
        self.assertIn("page = visible_lines - 1", pick_body)
        self.assertIn("ALLEGRO_KEY_PGUP", pick_body)
        self.assertIn("ALLEGRO_KEY_PGDN", pick_body)
        self.assertIn("-page", pick_body)
        self.assertIn("page)", pick_body)

    def test_death_prompt_clears_before_score_overlay(self):
        text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        wait_match = re.search(
            r"^rogue_allegro_wait_for_return\(const char \*prompt\)(.*?)^void\nrogue_allegro_shutdown",
            text,
            re.S | re.M,
        )
        self.assertIsNotNone(wait_match)
        wait_body = wait_match.group(1)
        self.assertIn("death_overlay_active = FALSE", wait_body)
        self.assertLess(
            wait_body.index("} while (ch != '\\n'"),
            wait_body.index("death_overlay_active = FALSE"),
        )

    def test_manual_reader_has_chapter_selection_and_clean_formatting(self):
        text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn("manual_chapter_titles", text)
        self.assertIn("manual_line_is_heading", text)
        self.assertIn("record_manual_chapter", text)
        self.assertIn("show_manual_chapter_picker", text)
        self.assertIn("manual_line_is_artifact", text)
        self.assertIn("USD:33-", text)
        self.assertIn("UNIX is a trademark", text)

        reader_match = re.search(
            r"^show_manual_reader\(const ROGUE_VARIANT_MANUAL_REF \*manual\)(.*?)^static void\nshow_manuals_menu_for_variant",
            text,
            re.S | re.M,
        )
        self.assertIsNotNone(reader_match)
        reader_body = reader_match.group(1)
        self.assertIn("Chapters", reader_body)

    def test_text_overlay_reader_supports_held_key_scrolling(self):
        text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        show_match = re.search(
            r"^rogue_allegro_text_overlay_show\(const char \*prompt\)(.*?)^char\nrogue_allegro_text_overlay_pick",
            text,
            re.S | re.M,
        )
        self.assertIsNotNone(show_match)
        show_body = show_match.group(1)
        self.assertIn("overlay_scroll_direction", show_body)
        self.assertIn("ROGUE_OVERLAY_SCROLL_RATE_SECONDS", show_body)
        self.assertIn("al_wait_for_event_timed", show_body)
        self.assertIn("ALLEGRO_EVENT_KEY_UP", show_body)
        self.assertIn("ALLEGRO_KEY_C", show_body)
        self.assertIn("show_manual_chapter_picker", show_body)

    def test_chapter_picker_drains_opening_keypress(self):
        text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        picker_match = re.search(
            r"^show_manual_chapter_picker\(void\)(.*?)^static void\nclean_manual_text",
            text,
            re.S | re.M,
        )
        self.assertIsNotNone(picker_match)
        picker_body = picker_match.group(1)
        self.assertIn("drain_keyboard_events()", picker_body)
        self.assertLess(
            picker_body.index("drain_keyboard_events()"),
            picker_body.index("rogue_allegro_text_overlay_pick"),
        )


if __name__ == "__main__":
    unittest.main()
