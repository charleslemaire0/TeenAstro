import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from teenastro_uploader import download
from teenastro_uploader.download import OFFLINE_MESSAGE, OfflineError


class _Proc:
    def __init__(self, returncode: int, stderr: str = ""):
        self.returncode = returncode
        self.stderr = stderr


class DownloadOfflineTests(unittest.TestCase):
    def test_curl_exit_6_is_offline(self):
        with patch.object(download.subprocess, "run", return_value=_Proc(6)):
            with self.assertRaises(OfflineError) as caught:
                download.download_file("https://github.com/example/file.hex", Path("ignored.hex"))
        self.assertEqual(str(caught.exception), OFFLINE_MESSAGE)

    def test_download_all_stops_on_first_offline(self):
        calls = {"n": 0}

        def fail_offline(url, dest):
            calls["n"] += 1
            raise OfflineError(OFFLINE_MESSAGE)

        with tempfile.TemporaryDirectory() as tmp:
            with patch.object(download, "firmware_dir", return_value=Path(tmp)), patch.object(
                download, "download_file", side_effect=fail_offline
            ):
                result = download.download_all("1.6")

        self.assertTrue(result.offline)
        self.assertEqual(result.error_message, OFFLINE_MESSAGE)
        self.assertEqual(result.success_count, 0)
        self.assertEqual(calls["n"], 1)
        self.assertNotIn("curl failed", result.error_message)
        self.assertNotIn("http", result.error_message)

    def test_http_404_is_not_offline(self):
        with patch.object(download.subprocess, "run", return_value=_Proc(22)):
            self.assertFalse(
                download.download_file("https://github.com/example/missing.hex", Path("missing.hex"))
            )

    def test_other_curl_error_keeps_url_once(self):
        with patch.object(
            download.subprocess, "run", return_value=_Proc(35, "SSL certificate problem")
        ):
            with self.assertRaises(RuntimeError) as caught:
                download.download_file("https://github.com/example/file.hex", Path("ignored.hex"))
        message = str(caught.exception)
        self.assertIn("exit 35", message)
        self.assertEqual(message.count("https://github.com/example/file.hex"), 1)


if __name__ == "__main__":
    unittest.main()
