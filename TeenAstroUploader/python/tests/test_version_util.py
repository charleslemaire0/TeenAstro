import unittest

from teenastro_uploader.version_util import (
    firmware_supported_for_config,
    parse_firmware_version,
)


class VersionUtilTests(unittest.TestCase):
    def test_parse(self):
        self.assertEqual(parse_firmware_version("1.5"), (1, 5, 0))
        self.assertEqual(parse_firmware_version("1.5.2"), (1, 5, 2))
        self.assertEqual(parse_firmware_version("1.4.0"), (1, 4, 0))

    def test_supported(self):
        self.assertTrue(firmware_supported_for_config("1.5"))
        self.assertTrue(firmware_supported_for_config("1.5.2"))
        self.assertTrue(firmware_supported_for_config("1.6.0"))
        self.assertFalse(firmware_supported_for_config("1.4.9"))
        self.assertFalse(firmware_supported_for_config("1.3"))
        self.assertFalse(firmware_supported_for_config("?"))


if __name__ == "__main__":
    unittest.main()
