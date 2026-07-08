import base64
import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tile_picker import tilepack_writer


PNG_1X1 = base64.b64decode(
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+/p9sAAAAASUVORK5CYII="
)


class TilePackWriterTests(unittest.TestCase):
    def test_write_default_pack_from_rltiles_mapping(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            assets = root / "assets" / "rltiles"
            assets.mkdir(parents=True)
            (assets / "rltiles-2d.png").write_bytes(PNG_1X1)
            (assets / "rltiles-2d.json").write_text(
                json.dumps({"width": 2, "tiles": ["floor", "human", "hobgoblin"]}),
                encoding="utf-8",
            )
            (assets / "rogue-rltiles-map.json").write_text(
                json.dumps({
                    "terrain": {"floor": {"atlas": "floor"}},
                    "actors": {"player": {"atlas": "human"}},
                    "objects": {},
                    "monsters": {"H": {"atlas": "hobgoblin"}},
                }),
                encoding="utf-8",
            )

            result = tilepack_writer.write_default_pack(root)

            self.assertEqual(result["ok"], True)
            tilepack = json.loads((root / "tilepacks" / "default" / "tilepack.json").read_text(encoding="utf-8"))
            mapping = json.loads((root / "tilepacks" / "default" / "mapping.json").read_text(encoding="utf-8"))
            self.assertEqual(tilepack["tileWidth"], 32)
            self.assertEqual(tilepack["tileHeight"], 32)
            self.assertEqual(tilepack["columns"], 2)
            self.assertEqual(tilepack["image"], "tiles.png")
            self.assertEqual(tilepack["fallbackToGenerated"], True)
            self.assertEqual(mapping["roles"]["terrain.floor"]["index"], 0)
            self.assertEqual(mapping["roles"]["actor.player"]["index"], 1)
            self.assertEqual(mapping["roles"]["monster.H"]["index"], 2)
            self.assertTrue((root / "tilepacks" / "default" / "tiles.png").exists())

    def test_write_active_custom_pack_decodes_uploaded_image(self):
        data_url = "data:image/png;base64," + base64.b64encode(PNG_1X1).decode("ascii")
        profile = {
            "source": {
                "dataUrl": data_url,
                "tileWidth": 16,
                "tileHeight": 24,
                "columns": 4,
            },
            "tiles": {
                "terrain.floor": {"index": 3, "name": "custom floor"},
                "actor.player": {"index": 4, "name": "custom player"},
                "monster.M": {"index": 5, "name": "custom medusa"},
                "monster.rogue52.M": {"index": 6, "name": "custom mimic"},
            },
        }

        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)

            result = tilepack_writer.write_active_custom_pack(root, profile)

            self.assertEqual(result["ok"], True)
            tilepack = json.loads((root / "tilepacks" / "active" / "tilepack.json").read_text(encoding="utf-8"))
            mapping = json.loads((root / "tilepacks" / "active" / "mapping.json").read_text(encoding="utf-8"))
            self.assertEqual(tilepack["tileWidth"], 16)
            self.assertEqual(tilepack["tileHeight"], 24)
            self.assertEqual(tilepack["columns"], 4)
            self.assertEqual(tilepack["fallbackToGenerated"], False)
            self.assertEqual(mapping["roles"]["actor.player"]["index"], 4)
            self.assertEqual(mapping["roles"]["monster.M"]["index"], 5)
            self.assertEqual(mapping["roles"]["monster.rogue52.M"]["index"], 6)
            self.assertEqual(mapping["roles"]["monster.rogue52.M"]["name"], "custom mimic")
            self.assertTrue((root / "tilepacks" / "active" / "tiles.png").exists())

    def test_custom_pack_rejects_missing_source(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            with self.assertRaises(tilepack_writer.TilePackError):
                tilepack_writer.write_active_custom_pack(root, {"tiles": {}})


if __name__ == "__main__":
    unittest.main()
