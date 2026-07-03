import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tile_picker import build_picker
from tile_picker import generate_tile_mapping


class TilePickerGenerationTests(unittest.TestCase):
    def test_custom_profile_generates_mapping_and_atlas_metadata(self):
        profile = {
            "source": {
                "type": "embedded_png",
                "path": "assets/generated/custom-tiles.png",
                "tileWidth": 16,
                "tileHeight": 24,
                "columns": 8,
            },
            "tiles": {
                "terrain.floor": {"index": 3, "name": "custom_floor"},
                "actor.player": {"index": 4, "name": "custom_player"},
                "monster.H": {"index": 5, "name": "custom_hobgoblin"},
                "monster.rogue52.M": {"index": 6, "name": "custom_mimic"},
            },
        }

        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            assets = root / "assets" / "rltiles"
            assets.mkdir(parents=True)
            (assets / "rogue-rltiles-map.json").write_text(
                json.dumps(
                    {
                        "variantMonsters": {
                            "rogue52": {
                                "M": {"name": "mimic", "atlas": "small_mimic"}
                            }
                        }
                    }
                ),
                encoding="utf-8",
            )
            (root / "tile_picker" / "data").mkdir(parents=True)
            profile_path = root / "tile_picker" / "data" / "profile.json"
            profile_path.write_text(json.dumps(profile), encoding="utf-8")

            result = generate_tile_mapping.generate(root, custom_profile=profile_path)

            header = (root / "generated" / "rogue_tile_mapping.h").read_text(encoding="utf-8")
            source = (root / "generated" / "rogue_tile_mapping.c").read_text(encoding="utf-8")
            self.assertEqual(result["mode"], "custom")
            self.assertIn("rogue_tile_atlas_source_width", header)
            self.assertIn('"assets/generated/custom-tiles.png"', source)
            self.assertIn("return 16;", source)
            self.assertIn("return 24;", source)
            self.assertIn("{ FLOOR, ROGUE_TILE_TERRAIN", source)
            self.assertIn('"custom_floor"', source)
            self.assertIn("{ 'H', \"monster.H\", \"custom_hobgoblin\", 5, \"hobgoblin\" }", source)
            self.assertIn(
                '{ "rogue52", \'M\', "monster.rogue52.M", "custom_mimic", 6, "mimic" }',
                source,
            )

    def test_picker_data_contains_rogue_roles_and_rltiles_catalog(self):
        atlas = {
            "tileSize": 32,
            "width": 2,
            "tiles": ["floor_of_a_room", "human", "hobgoblin"],
        }
        mapping = {
            "atlas": {"image": "rltiles-2d.png", "metadata": "rltiles-2d.json", "tileSize": 32},
            "terrain": {"floor": {"glyph": ".", "atlas": "floor_of_a_room"}},
            "objects": {},
            "actors": {"player": {"glyph": "@", "atlas": "human"}},
            "monsters": {"H": {"name": "hobgoblin", "atlas": "hobgoblin"}},
        }

        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            assets = root / "assets" / "rltiles"
            assets.mkdir(parents=True)
            (assets / "rltiles-2d.json").write_text(json.dumps(atlas), encoding="utf-8")
            (assets / "rogue-rltiles-map.json").write_text(json.dumps(mapping), encoding="utf-8")
            (assets / "rltiles-2d.png").write_bytes(b"png")

            data = build_picker.build_data(root)

            roles = {role["role"]: role for role in data["rogue"]["roles"]}
            self.assertEqual(roles["terrain.floor"]["currentRltilesIndex"], 0)
            self.assertEqual(roles["actor.player"]["currentRltilesIndex"], 1)
            self.assertEqual(roles["monster.H"]["currentRltilesIndex"], 2)
            self.assertEqual(data["rltiles"]["tiles"][1]["name"], "human")


if __name__ == "__main__":
    unittest.main()
