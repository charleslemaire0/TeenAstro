"""SHC flash command selection. Does not talk to a serial port."""

import sys
import types
import unittest
from pathlib import Path
from unittest.mock import patch

from teenastro_uploader import flash


class ShcFlashTests(unittest.TestCase):
    def test_s3_writes_merged_image_with_esp32s3(self):
        bin_path = Path("TeenAstroSHC_1.6_S3_English.bin")
        with patch.object(flash.fw, "resolve_shc_bin", return_value=bin_path), patch.object(
            flash, "_invoke_esptool"
        ) as invoke:
            flash.upload_shc_com("1.6", False, "English", "COM5", "S3")
        invoke.assert_called_once_with(
            [
                "--chip",
                "esp32s3",
                "--port",
                "COM5",
                "--baud",
                "921600",
                "write_flash",
                "0x0",
                str(bin_path),
            ]
        )

    def test_esptool_systemexit_zero_returns(self):
        fake = types.ModuleType("esptool")

        def main(_args):
            raise SystemExit(0)

        fake.main = main
        with patch.dict(sys.modules, {"esptool": fake}):
            flash._invoke_esptool(["--chip", "esp32s3"])

    def test_esptool_systemexit_keeps_log(self):
        fake = types.ModuleType("esptool")

        def main(_args):
            print("Failed to connect")
            raise SystemExit(2)

        fake.main = main
        with patch.dict(sys.modules, {"esptool": fake}):
            with self.assertRaises(RuntimeError) as caught:
                flash._invoke_esptool(["--chip", "esp32s3"])
        self.assertIn("exit 2", str(caught.exception))
        self.assertIn("Failed to connect", str(caught.exception))


if __name__ == "__main__":
    unittest.main()
