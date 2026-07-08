from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def test_android_build_keeps_variant_picker_entrypoints_reachable():
    main_c = (ROOT / "main.c").read_text(encoding="utf-8")
    cmake = (ROOT / "android/app/src/main/cpp/CMakeLists.txt").read_text(
        encoding="utf-8"
    )
    assert "ROGUE_ANDROID_DEFAULT_ONLY" not in cmake
    assert "rogue_frontend_choose_variant()" in main_c
    assert "rogue52_main(argc, argv, envp)" in main_c
    assert "rogue36_main(argc, argv, envp)" in main_c
    assert "srogue90_main(argc, argv, envp)" in main_c


def test_android_forces_tiles_mode():
    frontend_c = (ROOT / "frontend.c").read_text(encoding="utf-8")
    assert "ROGUE_ANDROID" in frontend_c
    assert "tiles_requested = TRUE" in frontend_c


def test_android_hides_desktop_tilepack_menu_hotkey():
    allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert "ROGUE_ANDROID" in allegro_c
    assert "show_tilepack_menu();" in allegro_c
    assert "#ifndef ROGUE_ANDROID" in allegro_c


def test_android_defers_variant_state_render_until_game_is_ready():
    allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    start_body = allegro_c[allegro_c.index("rogue_allegro_start"):
                           allegro_c.index("rogue_allegro_prepare_game_start")]
    render_body = allegro_c[allegro_c.index("rogue_allegro_render"):
                             allegro_c.index("rogue_allegro_readchar")]
    early_guard = render_body[render_body.index("if (!mobile_game_render_ready)"):
                              render_body.index("rows = view_rows();")]

    assert "mobile_game_render_ready" in allegro_c
    assert "stdscr != NULL" in start_body
    assert "mobile_game_render_ready = TRUE;" in start_body
    assert "if (!mobile_game_render_ready)" in render_body
    assert "rogue_variant_describe_cell" not in early_guard
    assert "draw_mobile_controls" not in early_guard
    assert "draw_text_overlay();" in early_guard
    assert "al_flip_display();" in early_guard


def test_android_startup_input_does_not_build_mobile_controls_before_game_ready():
    allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    readchar_body = allegro_c[allegro_c.index("rogue_allegro_readchar"):
                              allegro_c.index("rogue_allegro_show_prompt")]

    assert "if (!mobile_game_render_ready)" in readchar_body
    startup_guard = readchar_body[readchar_body.index("if (!mobile_game_render_ready)"):
                                  readchar_body.index("render_before_wait = TRUE;")]
    assert "return '\\n';" in startup_guard
    assert "build_mobile_layout" not in startup_guard
    assert "rogue_variant_action_context" not in startup_guard
