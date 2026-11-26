import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'simulator'))
from english_ui import TOKEN, translate

class EnglishUITests(unittest.TestCase):
    def test_catalogue_covers_existing_literals_and_preserves_code(self):
        catalogue = json.loads((ROOT / 'simulator/english_ui.json').read_text(encoding='utf-8'))
        for name, mapping in catalogue.items():
            with self.subTest(source=name):
                path = ROOT / 'src' / name
                before = path.read_bytes()
                source = path.read_text(encoding='utf-8')
                tokens = {m[0][1:-1] for m in TOKEN.finditer(source) if m[0].startswith('"')}
                self.assertFalse(mapping.keys() - tokens, 'Stale translation keys')
                translated = translate(source, mapping)
                mask = lambda s: TOKEN.sub(lambda m: '""' if m[0].startswith('"') else m[0], s)
                self.assertEqual(mask(source), mask(translated))
                self.assertEqual(before, path.read_bytes())

    def test_comments_characters_and_raw_strings_are_untouched(self):
        source = '''// "PRIMA"
/* "PRIMA" */ char quote='"'; const char* raw=R"tag("PRIMA")tag";
const char* label="PRIMA"; const char* command="select -a all";'''
        translated = translate(source, {'PRIMA': 'BEFORE'})
        self.assertEqual(source.replace('label="PRIMA"', 'label="BEFORE"'), translated)

    def test_format_parameters_cannot_change(self):
        with self.assertRaises(ValueError):
            translate('"%lu inviati"', {'%lu inviati': '%s sent'})
        self.assertEqual(translate('"%lu inviati"', {'%lu inviati': '%lu sent'}), '"%lu sent"')

    def test_unsafe_quoting_is_rejected(self):
        for value in ['bad"quote', 'bad\nline']:
            with self.assertRaises(ValueError):
                translate('"PRIMA"', {'PRIMA': value})

    def test_onboarding_uses_existing_english_body_once(self):
        catalogue = json.loads((ROOT / 'simulator/english_ui.json').read_text(encoding='utf-8'))
        source = (ROOT / 'src/onboarding.cpp').read_text(encoding='utf-8')
        result = translate(source, catalogue['onboarding.cpp'])
        self.assertNotIn('"IT\\n"', result)
        self.assertEqual(result.count('13:37 is a SECURITY RESEARCH'), 1)
        self.assertIn('"ACCEPT"', result)
        self.assertIn('#define ONBOARD_ENABLE   0', result)

if __name__ == '__main__':
    unittest.main()
