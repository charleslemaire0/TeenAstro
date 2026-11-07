"""Unit tests that do not need hardware."""

import unittest

from teenastro_uploader.detect import pcb_from_board, pcb_from_focuser
from teenastro_uploader.firmware import (
    firmware_file_list,
    mainunit_hex_stem,
    mainunit_mcu,
    shc_bin_filename,
    shc_chip,
    shc_ota_filename,
)


class MappingTests(unittest.TestCase):
    def test_pcb_from_board(self):
        self.assertEqual(pcb_from_board(240, 2), "2.4 TMC2130")
        self.assertEqual(pcb_from_board(250, 3), "2.5 TMC5160")
        self.assertIsNone(pcb_from_board(240, 1))

    def test_pcb_from_focuser(self):
        self.assertEqual(pcb_from_focuser("2.3", 0), "2.3 TMC2130")
        self.assertEqual(pcb_from_focuser("2.4.0", 3), "2.4 TMC5160")
        self.assertIsNone(pcb_from_focuser("2.4", 0))

    def test_firmware_list_and_mcu(self):
        files = firmware_file_list("1.6")
        self.assertEqual(len(files), 19)
        self.assertIn("TeenAstro_1_6_250_TMC2130.hex", files)
        self.assertIn("TeenAstroSHC_1.6_S3_English.bin", files)
        self.assertIn("TeenAstroSHC_1.6_S3_German_OTA.bin", files)
        self.assertEqual(mainunit_hex_stem("1.6", "2.5 TMC2130"), "TeenAstro_1_6_250_TMC2130")
        self.assertEqual(mainunit_hex_stem("1.6.15", "2.5 TMC5160"), "TeenAstro_1_6_250_TMC5160")
        self.assertEqual(mainunit_mcu("2.5 TMC2130"), "TEENSY40")
        self.assertEqual(mainunit_mcu("2.4 TMC2130"), "TEENSY31")
        self.assertEqual(shc_bin_filename("1.6", "French", "0.x"), "TeenAstroSHC_1.6_French.bin")
        self.assertEqual(
            shc_bin_filename("1.6", "French", "S3"), "TeenAstroSHC_1.6_S3_French.bin"
        )
        self.assertEqual(shc_ota_filename("1.6", "English"), "TeenAstroSHC_1.6_S3_English_OTA.bin")
        self.assertEqual(shc_chip("S3"), "esp32s3")
        self.assertEqual(shc_chip("0.x"), "esp8266")


if __name__ == "__main__":
    unittest.main()
