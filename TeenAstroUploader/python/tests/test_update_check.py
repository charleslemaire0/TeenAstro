import unittest
from datetime import datetime, timedelta, timezone

from teenastro_uploader.update_check import (
    UpdateInfo,
    commit_time,
    firmware_note,
    format_notice,
    higher_release_note,
    release_lines_from_refs,
    uploader_version_from_release,
)


class UpdateCheckTests(unittest.TestCase):
    def test_release_tag(self):
        parsed = uploader_version_from_release(
            {"tag_name": "v1.6.8", "html_url": "https://example/rel"}
        )
        self.assertEqual(parsed, ("1.6.8", "https://example/rel"))
        self.assertIsNone(uploader_version_from_release({"tag_name": "nightly"}))

    def test_commit_time(self):
        when = commit_time([{"commit": {"committer": {"date": "2026-11-08T18:00:00Z"}}}])
        self.assertEqual(when, datetime(2026, 11, 8, 18, 0, tzinfo=timezone.utc))
        self.assertIsNone(commit_time([]))

    def test_release_lines(self):
        lines = release_lines_from_refs([
            {"ref": "refs/heads/Release_1.5"},
            {"ref": "refs/heads/Release_1.6"},
            {"ref": "refs/heads/master"},
        ])
        self.assertEqual(lines, ("1.5", "1.6"))

    def test_firmware_missing_or_older(self):
        remote = datetime(2026, 11, 8, 18, 0, tzinfo=timezone.utc)
        self.assertIn(
            "not downloaded",
            firmware_note("1.6", "Latest", remote, None),
        )
        older = remote - timedelta(days=1)
        self.assertIn("after your download", firmware_note("1.6", "Stable", remote, older))
        self.assertIsNone(firmware_note("1.6", "Stable", remote, remote))
        self.assertIsNone(firmware_note("1.6", "Stable", None, None))

    def test_higher_release_line(self):
        note = higher_release_note(("1.6",), ("1.5", "1.6", "1.7"))
        self.assertIn("1.7", note)
        self.assertIsNone(higher_release_note(("1.6",), ("1.6",)))

    def test_notice_joins_both(self):
        text = format_notice(
            "1.6.7",
            UpdateInfo("1.6.8", "https://example/rel", ("1.6 Latest was updated after your download.",)),
        )
        self.assertIn("1.6.8", text)
        self.assertIn("This copy is 1.6.7.", text)
        self.assertIn("Newer firmware:", text)
        self.assertIn("1.6 Latest", text)

    def test_notice_silent_when_current(self):
        self.assertIsNone(format_notice("1.6.7", UpdateInfo()))


if __name__ == "__main__":
    unittest.main()
