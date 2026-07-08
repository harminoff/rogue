import json
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class VariantMetadataTest(unittest.TestCase):
    def test_rogue54_rogue36_rogue52_and_srogue90_are_registered(self):
        text = (ROOT / "variant.c").read_text(encoding="utf-8")

        self.assertIn('"rogue54"', text)
        self.assertIn('"Rogue 5.4.4"', text)
        self.assertIn('"rogue36"', text)
        self.assertIn('"Rogue 3.6.2"', text)
        self.assertIn('"rogue52"', text)
        self.assertIn('"Rogue 5.2.1"', text)
        self.assertIn('"srogue90"', text)
        self.assertIn('"Super-Rogue 9.0.1"', text)
        self.assertIn('"BSD-style"', text)
        self.assertIn('"Bundled"', text)

    def test_rogue54_is_default_variant(self):
        text = (ROOT / "variant.c").read_text(encoding="utf-8")

        self.assertIn("static const ROGUE_VARIANT_INFO *current_variant = &variants[0];", text)
        self.assertLess(text.index('"rogue54"'), text.index('"rogue36"'))
        self.assertLess(text.index('"rogue54"'), text.index('"rogue52"'))

    def test_rogue36_license_is_preserved(self):
        license_text = (ROOT / "variants" / "rogue36" / "LICENSE.TXT").read_text(
            encoding="utf-8"
        )
        readme_text = (ROOT / "variants" / "rogue36" / "README-RogueTiles.txt").read_text(
            encoding="utf-8"
        )

        self.assertIn("Redistribution and use in source and binary forms", license_text)
        self.assertIn("Michael Toy and Glenn Wichman", license_text)
        self.assertIn("rogue3.6.2-src.tar.gz", readme_text)
        self.assertIn("A378B23B19F65C245CFBEEF53AA7292D5DA5BEB1302BE5332948DFF78908F68E", readme_text)

    def test_rogue52_license_is_preserved(self):
        license_text = (ROOT / "variants" / "rogue52" / "LICENSE.TXT").read_text(
            encoding="utf-8"
        )

        self.assertIn("Redistribution and use in source and binary forms", license_text)
        self.assertIn("Michael Toy, Ken Arnold and Glenn Wichman", license_text)
        self.assertIn("Nicholas J. Kisseberth", license_text)
        self.assertIn("David Burren", license_text)

    def test_srogue90_license_is_preserved(self):
        license_text = (ROOT / "variants" / "srogue90" / "LICENSE.TXT").read_text(
            encoding="utf-8"
        )
        readme_text = (ROOT / "variants" / "srogue90" / "README-RogueTiles.txt").read_text(
            encoding="utf-8"
        )

        self.assertIn("Redistribution and use in source and binary forms", license_text)
        self.assertIn("Products derived from this software may not be called", license_text)
        self.assertIn("Super-Rogue", license_text)
        self.assertIn("srogue9.0-1-src.tar.gz", readme_text)
        self.assertIn("D307975AECFFEF48AE034B1F9ACFE84FA822DB59835927EF698FDA61E0072CC6", readme_text)

    def test_variants_include_manual_and_guide_references(self):
        header_text = (ROOT / "variant.h").read_text(encoding="utf-8")
        variant_text = (ROOT / "variant.c").read_text(encoding="utf-8")
        frontend_text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn("ROGUE_VARIANT_MANUAL_REF", header_text)
        self.assertIn("readable_path", header_text)
        self.assertIn("manuals[ROGUE_VARIANT_MAX_MANUALS]", header_text)
        self.assertIn("A Guide to the Dungeons of Doom", variant_text)
        self.assertIn("Bundled guide", variant_text)
        self.assertIn("BSD-style source distribution", variant_text)
        self.assertNotIn("Epyx IBM PC manual", variant_text)
        self.assertNotIn("britzl.github.io/roguearchive", variant_text)
        self.assertIn("rogue54.6", variant_text)
        self.assertIn("rogue54.doc", variant_text)
        self.assertIn("variants/rogue36/rogue.6", variant_text)
        self.assertIn("variants/rogue36/rogue.r", variant_text)
        self.assertIn("variants/rogue52/rogue.6", variant_text)
        self.assertTrue((ROOT / "rogue54.6").exists())
        self.assertTrue((ROOT / "rogue54.doc").exists())
        self.assertTrue((ROOT / "variants" / "rogue36" / "rogue.6").exists())
        self.assertTrue((ROOT / "variants" / "rogue36" / "rogue.r").exists())
        self.assertTrue((ROOT / "variants" / "rogue52" / "rogue.6").exists())
        self.assertTrue((ROOT / "variants" / "srogue90" / "rogue.nr").exists())
        open_manual_body = frontend_text[
            frontend_text.index("open_manual_file"):
            frontend_text.index("manual_has_local_text")
        ]
        self.assertIn("rogue_platform_asset_path", open_manual_body)
        self.assertIn("open_manual_file(manual->readable_path", frontend_text)
        package_text = (ROOT / "scripts" / "package-windows.ps1").read_text(encoding="utf-8")
        self.assertIn("rogue54.6", package_text)
        self.assertIn("rogue54.doc", package_text)
        self.assertIn("$nativeDir $manual", package_text)
        self.assertIn("show_manuals_menu", frontend_text)
        self.assertIn("show_manual_reader", frontend_text)
        self.assertIn("load_manual_text_overlay", frontend_text)
        self.assertIn("manual_line_is_artifact", frontend_text)
        self.assertIn("flush_manual_paragraph", frontend_text)
        self.assertIn("rogue_allegro_text_overlay_pick", frontend_text)
        self.assertIn("ALLEGRO_KEY_F1", frontend_text)
        self.assertIn("current->manuals", frontend_text)
        self.assertIn("show_manuals_menu_for_variant(selected_variant)", frontend_text)

    def test_rogue36_runtime_bridge_is_registered(self):
        variant_text = (ROOT / "variant.c").read_text(encoding="utf-8")
        tiles_text = (ROOT / "tiles.c").read_text(encoding="utf-8")
        makefile_text = (ROOT / "Makefile.std").read_text(encoding="utf-8")
        main_text = (ROOT / "main.c").read_text(encoding="utf-8")
        port_text = (ROOT / "variants" / "rogue36" / "rogue36_port.c").read_text(
            encoding="utf-8"
        )
        rogue36_main_text = (
            ROOT / "variants" / "rogue36" / "main.c"
        ).read_text(encoding="utf-8")
        symbols_text = (
            ROOT / "variants" / "rogue36" / "rogue36_symbols.redef"
        ).read_text(encoding="utf-8")

        self.assertIn("rogue36_variant_status", variant_text)
        self.assertIn("rogue36_tile_describe_cell", variant_text)
        self.assertIn("rogue36_tile_describe_cell", tiles_text)
        self.assertIn('apply_variant_monster_mapping("rogue36"', tiles_text)
        self.assertIn("glyph_is_object", tiles_text)
        self.assertIn("object_visible", tiles_text)
        self.assertIn("ROGUE36_CFILES", makefile_text)
        self.assertIn("ROGUE36_REFSECTIONS", makefile_text)
        self.assertIn("rogue36_refptr_", makefile_text)
        self.assertIn("rogue36_port.c", makefile_text)
        self.assertIn("rogue36_main", main_text)
        self.assertIn("prepare_rogue36_shared_state", main_text)
        self.assertIn("food_left = HUNGERTIME", main_text)
        self.assertIn("extern struct thing rogue36_player", port_text)
        self.assertIn("return rogue36_player.t_stats.s_hpt", port_text)
        self.assertIn("return rogue36_hungry_state", port_text)
        self.assertIn("return LINES > 1 ? LINES - 1 : LINES", port_text)
        self.assertIn("for (item = rogue36_lvl_obj", port_text)
        self.assertIn("rogue36_bridge_trap_type_at", port_text)
        self.assertIn("extern struct trap traps", port_text)
        self.assertIn("monster_has_disguise", tiles_text)
        self.assertIn("disguise != '\\0' && disguise != monster_type", tiles_text)
        self.assertIn("if (y <= 0 || y >= rows", tiles_text)
        self.assertIn("apply_trap_mapping", tiles_text)
        self.assertIn("if (rogue_frontend_is_tiles())", rogue36_main_text)
        self.assertIn("resize_term(24, 80)", rogue36_main_text)
        self.assertIn("max_hp rogue36_max_hp", symbols_text)
        self.assertIn("cw rogue36_cw", symbols_text)
        self.assertIn("mw rogue36_mw", symbols_text)

    def test_srogue90_status_uses_two_line_super_rogue_hud(self):
        header_text = (ROOT / "variant.h").read_text(encoding="utf-8")
        tiles_text = (ROOT / "tiles.c").read_text(encoding="utf-8")
        frontend_text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
        port_text = (
            ROOT / "variants" / "srogue90" / "srogue90_port.c"
        ).read_text(encoding="utf-8")

        self.assertIn("has_extended_stats", header_text)
        self.assertIn("dexterity_base", header_text)
        self.assertIn("carry_capacity", header_text)
        self.assertIn("volume_percent", header_text)
        self.assertIn("#define SROGUE90_MAXCOLS 256", tiles_text)
        self.assertIn("return LINES > 1 ? LINES - 1 : LINES", port_text)
        self.assertIn("srogue90_bridge_status_row_start", port_text)
        self.assertIn("return LINES > 3 ? LINES - 3 : LINES", port_text)
        self.assertIn("srogue90_bridge_volume_percent", port_text)
        self.assertIn("return cansee(y, x) ? TRUE : FALSE", port_text)
        self.assertIn("&& !srogue90_bridge_player_is_blind()", tiles_text)
        self.assertIn("&& srogue90_bridge_cansee(y, x)", tiles_text)
        self.assertIn("object_visible = (bool)(glyph_is_object && visible)", tiles_text)
        self.assertIn("status->has_extended_stats = TRUE", tiles_text)
        self.assertIn("status->dexterity = srogue90_bridge_dexterity()", tiles_text)
        self.assertIn("y >= srogue90_bridge_status_row_start()", tiles_text)
        self.assertIn("Vol:%d%%", frontend_text)
        self.assertIn("Str:%s  Dex:%s  Wis:%s  Con:%s  Carry:%d(%d)", frontend_text)

    def test_variant_giant_ant_uses_visible_sprite(self):
        mapping = json.loads(
            (ROOT / "assets" / "rltiles" / "rogue-rltiles-map.json").read_text(
                encoding="utf-8"
            )
        )

        for variant in ("rogue36", "rogue52"):
            ant = mapping["variantMonsters"][variant]["A"]
            self.assertEqual(ant["name"], "giant ant")
            self.assertEqual(ant["atlas"], "queen_ant")
            self.assertNotEqual(ant["atlas"], "giant_ant")

        srogue_ant = next(
            item for item in mapping["variantMonsters"]["srogue90"]
            if item["glyph"] == "A"
        )
        self.assertEqual(srogue_ant["name"], "giant ant")
        self.assertEqual(srogue_ant["atlas"], "queen_ant")

    def test_variant_kobold_uses_visible_sprite(self):
        mapping = json.loads(
            (ROOT / "assets" / "rltiles" / "rogue-rltiles-map.json").read_text(
                encoding="utf-8"
            )
        )

        for variant in ("rogue36", "rogue52"):
            kobold = mapping["variantMonsters"][variant]["K"]
            self.assertEqual(kobold["name"], "kobold")
            self.assertEqual(kobold["atlas"], "big_kobold")
            self.assertNotEqual(kobold["atlas"], "kobold")

        srogue_kobold = next(
            item for item in mapping["variantMonsters"]["srogue90"]
            if item["glyph"] == "K"
        )
        self.assertEqual(srogue_kobold["name"], "kobold")
        self.assertEqual(srogue_kobold["atlas"], "big_kobold")

    def test_rogue36_core_menus_use_gui_overlays_in_tile_mode(self):
        command_text = (ROOT / "variants" / "rogue36" / "command.c").read_text(
            encoding="utf-8"
        )
        pack_text = (ROOT / "variants" / "rogue36" / "pack.c").read_text(
            encoding="utf-8"
        )
        options_text = (ROOT / "variants" / "rogue36" / "options.c").read_text(
            encoding="utf-8"
        )
        monsters_text = (ROOT / "variants" / "rogue36" / "monsters.c").read_text(
            encoding="utf-8"
        )
        io_text = (ROOT / "variants" / "rogue36" / "io.c").read_text(
            encoding="utf-8"
        )

        self.assertIn('rogue_frontend_text_overlay_begin("Command Help")', command_text)
        self.assertIn('rogue_frontend_text_overlay_begin("Identify")', command_text)
        self.assertIn("tile_pick_pack_letter", pack_text)
        self.assertIn('rogue_frontend_text_overlay_begin("Inventory")', pack_text)
        self.assertIn("guard++ < MAXPACK + 5", pack_text)
        self.assertIn("tile_option()", options_text)
        self.assertIn('rogue_frontend_text_overlay_begin("Options")', options_text)
        self.assertIn("rogue_frontend_text_input", options_text)
        self.assertIn("tile_pick_genocide_monster", monsters_text)
        self.assertIn('rogue_frontend_text_overlay_begin("Genocide")', monsters_text)
        self.assertIn("rogue_frontend_text_overlay_begin(message)", io_text)

    def test_super_rogue_messages_are_forwarded_to_tile_log(self):
        io_text = (ROOT / "variants" / "srogue90" / "io.c").read_text(
            encoding="utf-8"
        )

        self.assertIn("rogue_frontend_record_message(msgbuf)", io_text)
        self.assertIn("rogue_frontend_render()", io_text)

    def test_rogue36_save_score_death_and_win_use_gui_overlays_in_tile_mode(self):
        save_text = (ROOT / "variants" / "rogue36" / "save.c").read_text(
            encoding="utf-8"
        )
        rip_text = (ROOT / "variants" / "rogue36" / "rip.c").read_text(
            encoding="utf-8"
        )

        self.assertIn("#include \"../../frontend.h\"", save_text)
        self.assertIn("restore_error", save_text)
        self.assertIn("rogue_frontend_confirm(\"Save Game\"", save_text)
        self.assertIn("rogue_frontend_notice(\"Restore Failed\"", save_text)
        self.assertIn("rogue_frontend_start()", save_text)
        self.assertIn("tile_score_pause_shown", rip_text)
        self.assertIn("rogue_frontend_show_death", rip_text)
        self.assertIn('rogue_frontend_text_overlay_begin("Scores")', rip_text)
        self.assertIn('rogue_frontend_text_overlay_begin("You Made It!")', rip_text)
        self.assertIn('rogue_frontend_text_overlay_begin("Spoils")', rip_text)
        self.assertIn("rogue_frontend_request_launcher_restart()", rip_text)

    def test_rogue52_help_uses_gui_overlay_in_tile_mode(self):
        command_text = (ROOT / "variants" / "rogue52" / "command.c").read_text(
            encoding="utf-8"
        )

        self.assertIn("format_help_line", command_text)
        self.assertIn("rogue_frontend_is_tiles()", command_text)
        self.assertIn('rogue_frontend_text_overlay_begin("Command Help")', command_text)
        self.assertIn("rogue_frontend_text_overlay_add(line)", command_text)
        self.assertIn("rogue_frontend_text_overlay_show", command_text)

    def test_rogue52_identify_uses_gui_picker_in_tile_mode(self):
        command_text = (ROOT / "variants" / "rogue52" / "command.c").read_text(
            encoding="utf-8"
        )

        self.assertIn("ident_list[]", command_text)
        self.assertIn('rogue_frontend_text_overlay_begin("Identify")', command_text)
        self.assertIn("rogue_frontend_text_overlay_pick", command_text)
        self.assertIn("for (ch = 'A'; ch <= 'Z'; ch++)", command_text)
        self.assertIn("monsters[ch - 'A'].m_name", command_text)

    def test_rogue52_pack_item_selection_uses_gui_picker_in_tile_mode(self):
        pack_text = (ROOT / "variants" / "rogue52" / "pack.c").read_text(
            encoding="utf-8"
        )

        self.assertIn("tile_pick_pack_letter", pack_text)
        self.assertIn("pack_type_matches", pack_text)
        self.assertIn('rogue_frontend_text_overlay_begin("Inventory")', pack_text)
        self.assertIn("rogue_frontend_text_overlay_pick(prompt)", pack_text)
        self.assertIn("if (rogue_frontend_is_tiles())", pack_text)
        self.assertIn("ch = tile_pick_pack_letter(purpose, type)", pack_text)
        self.assertIn("for (obj = pack, och = 'a';", pack_text)

    def test_rogue52_inventory_lists_use_gui_overlay_in_tile_mode(self):
        things_text = (ROOT / "variants" / "rogue52" / "things.c").read_text(
            encoding="utf-8"
        )

        self.assertIn("#include \"../../frontend.h\"", things_text)
        self.assertIn("rogue_frontend_text_overlay_begin", things_text)
        self.assertIn("rogue_frontend_text_overlay_add(tile_line)", things_text)
        self.assertIn("rogue_frontend_text_overlay_show", things_text)
        self.assertIn("rogue_frontend_text_overlay_clear", things_text)

    def test_rogue52_discovered_items_uses_gui_type_picker_in_tile_mode(self):
        things_text = (ROOT / "variants" / "rogue52" / "things.c").read_text(
            encoding="utf-8"
        )

        self.assertIn("tile_pick_discovery_type", things_text)
        self.assertIn('rogue_frontend_text_overlay_begin("Discoveries")', things_text)
        self.assertIn("!) Potions", things_text)
        self.assertIn("?) Scrolls", things_text)
        self.assertIn("=) Rings", things_text)
        self.assertIn("/) Sticks", things_text)
        self.assertIn("*) All discovered items", things_text)
        self.assertIn("ch = tile_pick_discovery_type()", things_text)

    def test_rogue52_options_use_gui_menu_in_tile_mode(self):
        options_text = (ROOT / "variants" / "rogue52" / "options.c").read_text(
            encoding="utf-8"
        )

        self.assertIn("#include \"../../frontend.h\"", options_text)
        self.assertIn("tile_option()", options_text)
        self.assertIn("tile_option_line", options_text)
        self.assertIn("tile_change_option", options_text)
        self.assertIn('rogue_frontend_text_overlay_begin("Options")', options_text)
        self.assertIn("rogue_frontend_text_overlay_pick", options_text)
        self.assertIn("rogue_frontend_text_input", options_text)
        self.assertIn("if (rogue_frontend_is_tiles())", options_text)

    def test_rogue52_genocide_uses_gui_monster_picker_in_tile_mode(self):
        monsters_text = (ROOT / "variants" / "rogue52" / "monsters.c").read_text(
            encoding="utf-8"
        )

        self.assertIn("#include \"../../frontend.h\"", monsters_text)
        self.assertIn("tile_pick_genocide_monster", monsters_text)
        self.assertIn('rogue_frontend_text_overlay_begin("Genocide")', monsters_text)
        self.assertIn("monsters[i].m_name", monsters_text)
        self.assertIn("rogue_frontend_text_overlay_pick", monsters_text)
        self.assertIn("if (rogue_frontend_is_tiles())", monsters_text)

    def test_rogue52_quit_uses_gui_confirmation_in_tile_mode(self):
        main_text = (ROOT / "variants" / "rogue52" / "main.c").read_text(
            encoding="utf-8"
        )

        self.assertIn("#include \"../../frontend.h\"", main_text)
        self.assertIn("rogue_frontend_confirm(\"Quit\"", main_text)
        self.assertIn("if (rogue_frontend_is_tiles())", main_text)

    def test_rogue52_shell_escape_uses_gui_notice_in_tile_mode(self):
        main_text = (ROOT / "variants" / "rogue52" / "main.c").read_text(
            encoding="utf-8"
        )

        self.assertIn("shell escape is not available in tile mode", main_text)
        self.assertIn("rogue_frontend_notice(\"Shell Escape\"", main_text)
        self.assertIn("return;", main_text)

    def test_rogue52_save_restore_uses_gui_prompts_and_notices_in_tile_mode(self):
        save_text = (ROOT / "variants" / "rogue52" / "save.c").read_text(
            encoding="utf-8"
        )
        options_text = (ROOT / "variants" / "rogue52" / "options.c").read_text(
            encoding="utf-8"
        )

        self.assertIn("#include \"../../frontend.h\"", save_text)
        self.assertIn("restore_error", save_text)
        self.assertIn("rogue_frontend_confirm(\"Save Game\"", save_text)
        self.assertIn("rogue_frontend_confirm", save_text)
        self.assertIn("\"Overwrite Save\"", save_text)
        self.assertIn("rogue_frontend_notice(\"Restore Failed\"", save_text)
        self.assertIn("rogue_frontend_start()", save_text)
        self.assertIn("rogue_frontend_text_input(\"Input\"", options_text)

    def test_rogue52_score_death_and_win_use_gui_overlays_in_tile_mode(self):
        rip_text = (ROOT / "variants" / "rogue52" / "rip.c").read_text(
            encoding="utf-8"
        )

        self.assertIn("#include \"../../frontend.h\"", rip_text)
        self.assertIn("tile_score_pause_shown", rip_text)
        self.assertIn("rogue_frontend_show_death", rip_text)
        self.assertIn("rogue_frontend_wait_for_return", rip_text)
        self.assertIn('rogue_frontend_text_overlay_begin("Scores")', rip_text)
        self.assertIn('rogue_frontend_text_overlay_begin("You Made It!")', rip_text)
        self.assertIn('rogue_frontend_text_overlay_begin("Spoils")', rip_text)
        self.assertIn("rogue_frontend_text_overlay_show", rip_text)

    def test_tile_death_returns_to_variant_picker_launcher(self):
        frontend_header = (ROOT / "frontend.h").read_text(encoding="utf-8")
        frontend_text = (ROOT / "frontend.c").read_text(encoding="utf-8")
        rip54_text = (ROOT / "rip.c").read_text(encoding="utf-8")
        rip52_text = (ROOT / "variants" / "rogue52" / "rip.c").read_text(
            encoding="utf-8"
        )
        rip36_text = (ROOT / "variants" / "rogue36" / "rip.c").read_text(
            encoding="utf-8"
        )

        self.assertIn("rogue_frontend_request_launcher_restart", frontend_header)
        self.assertIn("launcher_restart_requested", frontend_text)
        self.assertIn("CreateProcessA", frontend_text)
        self.assertIn(" --tiles", frontend_text)
        self.assertIn("Press Enter to return to game select", rip54_text)
        self.assertIn("rogue_frontend_request_launcher_restart()", rip54_text)
        self.assertIn("Press Enter to return to game select", rip52_text)
        self.assertIn("rogue_frontend_request_launcher_restart()", rip52_text)
        self.assertIn("Press Enter to return to game select", rip36_text)
        self.assertIn("rogue_frontend_request_launcher_restart()", rip36_text)


if __name__ == "__main__":
    unittest.main()
