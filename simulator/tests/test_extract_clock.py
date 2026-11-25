import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import extract_clock

class ExtractionTests(unittest.TestCase):
    def test_original_source_is_reused_verbatim(self):
        path = Path(__file__).resolve().parents[2] / 'src/main.cpp'
        before = path.read_bytes()
        source = before.decode('utf-8')
        generated = extract_clock.extract(source)
        for name in extract_clock.FUNCTIONS:
            self.assertIn(extract_clock.function(source, name), generated)
        self.assertIn(extract_clock.between(source, '    clock_screen = lv_obj_create(NULL);',
                                           '    gps_screen_create();'), generated)
        self.assertEqual(before, path.read_bytes())

    def test_comments_and_strings_do_not_terminate_function(self):
        source = 'static void clock() { /* } */ const char* s="}"; if(true){ } }\nvoid other() {}'
        result = extract_clock.function(source, 'clock')
        self.assertTrue(result.endswith('if(true){ } }'))
        self.assertNotIn('other', result)

    def test_ambiguous_and_missing_boundaries_fail(self):
        for source in ['start start end', 'nothing here']:
            with self.assertRaises(ValueError):
                extract_clock.between(source, 'start', 'end')

    def test_missing_or_duplicate_function_fails(self):
        for source in ['', 'void clock() {}\nvoid clock() {}']:
            with self.assertRaises(ValueError):
                extract_clock.function(source, 'clock')

if __name__ == '__main__':
    unittest.main()
