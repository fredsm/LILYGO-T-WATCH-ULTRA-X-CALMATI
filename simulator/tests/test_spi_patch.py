import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    "patch_spi_clients", Path(__file__).resolve().parents[2] / "scripts/patch_spi_clients.py")
patch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)

class PatchTest(unittest.TestCase):
    def test_repeated_and_changed_source(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            radio = root / "RadioLib/src/hal/Arduino/ArduinoHal.cpp"
            nfc = root / "ST25R3916/src/st25r3916_com.cpp"
            radio.parent.mkdir(parents=True)
            nfc.parent.mkdir(parents=True)
            radio.write_text('#include "ArduinoHal.h"\nspi->beginTransaction(spiSettings);\nspi->endTransaction();\n')
            nfc.write_text('#include "rfal_rfst25r3916.h"\n' +
                ('dev_spi->beginTransaction(SPISettings(spi_speed, MSBFIRST, SPI_MODE1));\n'
                 'dev_spi->endTransaction();\n') * 11)
            patch.patch_clients(root)
            first = (radio.read_text(), nfc.read_text())
            patch.patch_clients(root)
            self.assertEqual(first, (radio.read_text(), nfc.read_text()))
            self.assertEqual(first[1].count('twatch_spi_end_tagged(dev_spi);'), 11)
            self.assertIn('twatch_spi_begin_tagged(spi, spiSettings, 1);', first[0])
            radio.write_text(first[0] + 'spi->endTransaction();\n')
            with self.assertRaises(RuntimeError):
                patch.patch_clients(root)

if __name__ == "__main__":
    unittest.main()
