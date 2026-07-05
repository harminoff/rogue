from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_platform_path_api_exists():
    header = (ROOT / "rogue_platform.h").read_text(encoding="utf-8")
    assert "rogue_platform_configure_storage" in header
    assert "rogue_platform_asset_path" in header
    assert "rogue_platform_user_path" in header


def test_tilepack_uses_platform_asset_paths():
    text = (ROOT / "tilepack.c").read_text(encoding="utf-8")
    assert '#include "rogue_platform.h"' in text
    assert "rogue_platform_read_text_file" in text
    assert "rogue_platform_asset_path" in text
    assert 'fopen(path, "rb")' not in text


def test_allegro_frontend_uses_platform_paths_for_settings_and_atlas():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert '#include "rogue_platform.h"' in text
    assert "rogue_platform_user_path" in text
    assert "rogue_platform_asset_path" in text
