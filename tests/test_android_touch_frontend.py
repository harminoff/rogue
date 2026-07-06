from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


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
    assert "rogue_mobile_command_at" in text
    assert "pending_touch_command" in text


def test_mobile_attack_is_visible_but_disabled():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert '"ATK"' in text
    assert '"WAIT"' in text
    assert '"LOOK"' in text
    assert '"DOWN"' in text
    assert "mobile_attack_disabled" in text
