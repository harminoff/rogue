from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def test_android_default_only_bypasses_variant_entrypoints():
    main_c = (ROOT / "main.c").read_text(encoding="utf-8")
    assert "ROGUE_ANDROID_DEFAULT_ONLY" in main_c
    assert "#ifndef ROGUE_ANDROID_DEFAULT_ONLY" in main_c


def test_android_forces_tiles_mode():
    frontend_c = (ROOT / "frontend.c").read_text(encoding="utf-8")
    assert "ROGUE_ANDROID" in frontend_c
    assert "tiles_requested = TRUE" in frontend_c


def test_android_hides_desktop_tilepack_menu_hotkey():
    allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert "ROGUE_ANDROID" in allegro_c
    assert "show_tilepack_menu();" in allegro_c
    assert "#ifndef ROGUE_ANDROID" in allegro_c
