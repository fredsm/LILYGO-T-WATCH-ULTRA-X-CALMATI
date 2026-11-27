"""Keep the default layout reference inventory aligned with built-in screens."""
import re
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class LayoutInventoryTests(unittest.TestCase):
    def test_default_inventory(self):
        lines = (ROOT / "sd/Settings/layout_default.txt").read_text().splitlines()
        fields = dict(tuple(part.strip() for part in line.split("=", 1))
                      for line in lines if line.strip() and not line.startswith("#"))
        screens = [name.strip() for name in fields["available-screens"].split(",")]
        expected = {path.stem.removesuffix("_screen")
                    for path in (ROOT / "src").glob("*_screen.h")}
        expected |= {"home", "onboarding", "task_manager"}
        self.assertEqual(len(screens), len(set(screens)), "Duplicate screen identifiers")
        self.assertEqual(set(screens), expected)
        targets = [name.strip() for name in fields["available-home-targets"].split(",")]
        source = (ROOT / "src/layout_config.cpp").read_text()
        compiled = re.search(r"const char\* const targets\[\]=\{(.*?)\};", source, re.S)
        self.assertIsNotNone(compiled)
        self.assertEqual(targets, re.findall(r'"([^"]+)"', compiled.group(1)))
        self.assertTrue(set(targets) - {"none"} <= set(screens))


if __name__ == "__main__":
    unittest.main()
