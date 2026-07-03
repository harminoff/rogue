import sys
import base64
import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tile_picker import serve_picker

PNG_1X1 = base64.b64decode(
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+/p9sAAAAASUVORK5CYII="
)


class PackagedPickerTests(unittest.TestCase):
    def test_repo_root_uses_executable_directory_when_frozen(self):
        executable = Path("C:/Games/RogueTiles/TilePicker.exe")
        with mock.patch.object(sys, "frozen", True, create=True), \
             mock.patch.object(sys, "executable", str(executable)):
            self.assertEqual(serve_picker.repo_root(), executable.parent)

    def test_save_custom_profile_writes_named_runtime_tilepack(self):
        data_url = "data:image/png;base64," + base64.b64encode(PNG_1X1).decode("ascii")
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            result = serve_picker.save_profile(root, {
                "name": "My Custom Pack",
                "profile": {
                    "sourceMode": "custom",
                    "customSource": {
                        "fileName": "custom.png",
                        "dataUrl": data_url,
                        "atlas": {
                            "tileWidth": 16,
                            "tileHeight": 16,
                            "columns": 2,
                        },
                    },
                    "customSelections": {
                        "actor.player": {"index": 1, "name": "player"},
                    },
                },
            })

            self.assertEqual(result["ok"], True)
            self.assertEqual(result["tilepackPath"], "tilepacks/My_Custom_Pack/tilepack.json")
            self.assertTrue((root / "tile_picker" / "profiles" / "My_Custom_Pack.json").exists())
            tilepack = json.loads((root / "tilepacks" / "My_Custom_Pack" / "tilepack.json").read_text(encoding="utf-8"))
            mapping = json.loads((root / "tilepacks" / "My_Custom_Pack" / "mapping.json").read_text(encoding="utf-8"))
            self.assertEqual(tilepack["name"], "My_Custom_Pack")
            self.assertEqual(tilepack["tileWidth"], 16)
            self.assertEqual(mapping["roles"]["actor.player"]["index"], 1)


if __name__ == "__main__":
    unittest.main()
