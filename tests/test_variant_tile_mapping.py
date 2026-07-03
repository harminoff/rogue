import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tile_picker import build_picker
from tile_picker import generate_tile_mapping
from tile_picker import tilepack_writer


def write_fixture(root: Path) -> None:
    assets = root / "assets" / "rltiles"
    assets.mkdir(parents=True)
    atlas = {
        "tileSize": 32,
        "width": 4,
        "tiles": [
            "medusa",
            "small_mimic",
            "giant_ant",
            "floating_eye",
        ],
    }
    mapping = {
        "atlas": {
            "image": "rltiles-2d.png",
            "metadata": "rltiles-2d.json",
            "tileSize": 32,
        },
        "terrain": {},
        "objects": {},
        "actors": {},
        "monsters": {
            "A": {"name": "aquator", "atlas": "medusa"},
            "M": {"name": "medusa", "atlas": "medusa"},
        },
        "variantMonsters": {
            "rogue36": {
                "M": {"name": "mimic", "atlas": "small_mimic"},
            },
            "rogue52": {
                "A": {"name": "giant ant", "atlas": "giant_ant"},
                "E": {"name": "floating eye", "atlas": "floating_eye"},
                "M": {"name": "mimic", "atlas": "small_mimic"},
            }
        },
    }
    (assets / "rltiles-2d.json").write_text(json.dumps(atlas), encoding="utf-8")
    (assets / "rogue-rltiles-map.json").write_text(json.dumps(mapping), encoding="utf-8")
    (assets / "rltiles-2d.png").write_bytes(b"png")


class VariantTileMappingTests(unittest.TestCase):
    def test_picker_exposes_variant_monster_roles(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            write_fixture(root)

            data = build_picker.build_data(root)

            roles = {role["role"]: role for role in data["rogue"]["roles"]}
            self.assertEqual(roles["monster.M"]["name"], "medusa")
            self.assertEqual(roles["monster.rogue36.M"]["name"], "mimic")
            self.assertEqual(roles["monster.rogue52.M"]["name"], "mimic")
            self.assertEqual(roles["monster.rogue52.M"]["currentRltilesIndex"], 1)

    def test_generated_c_contains_variant_monster_lookup_table(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            write_fixture(root)

            generate_tile_mapping.generate(root, force_rltiles=True)

            header = (root / "generated" / "rogue_tile_mapping.h").read_text(encoding="utf-8")
            source = (root / "generated" / "rogue_tile_mapping.c").read_text(encoding="utf-8")
            self.assertIn("ROGUE_GENERATED_VARIANT_MONSTER_MAPPING", header)
            self.assertIn("rogue_tile_variant_monster_mappings", source)
            self.assertIn('{ \'M\', "monster.M", "medusa", 0, "medusa" }', source)
            self.assertIn('{ "rogue36", \'M\', "monster.rogue36.M", "small_mimic", 1, "mimic" }', source)
            self.assertIn('{ "rogue52", \'M\', "monster.rogue52.M", "small_mimic", 1, "mimic" }', source)

    def test_default_tilepack_includes_variant_monster_roles(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            write_fixture(root)

            tilepack_writer.write_default_pack(root)

            mapping = json.loads((root / "tilepacks" / "default" / "mapping.json").read_text(encoding="utf-8"))
            self.assertEqual(mapping["roles"]["monster.M"]["index"], 0)
            self.assertEqual(mapping["roles"]["monster.rogue36.M"]["index"], 1)
            self.assertEqual(mapping["roles"]["monster.rogue52.M"]["index"], 1)


if __name__ == "__main__":
    unittest.main()
