import sys
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tile_picker import serve_picker


class PackagedPickerTests(unittest.TestCase):
    def test_repo_root_uses_executable_directory_when_frozen(self):
        executable = Path("C:/Games/RogueTiles/TilePicker.exe")
        with mock.patch.object(sys, "frozen", True, create=True), \
             mock.patch.object(sys, "executable", str(executable)):
            self.assertEqual(serve_picker.repo_root(), executable.parent)


if __name__ == "__main__":
    unittest.main()
