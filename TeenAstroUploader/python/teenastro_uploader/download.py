"""Download firmware files via system curl (CrowdStrike-friendly on Windows)."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path

from .firmware import firmware_file_list, github_firmware_url
from .paths import firmware_dir


ProgressCallback = Callable[[int, int, str], None]


@dataclass
class DownloadResult:
    success_count: int
    total_count: int
    error_message: str = ""


def _curl_path() -> str:
    if sys.platform == "win32":
        system = Path(os.environ.get("SystemRoot", r"C:\Windows")) / "System32" / "curl.exe"
        if system.is_file():
            return str(system)
    found = shutil.which("curl")
    if not found:
        raise FileNotFoundError(
            "curl not found. On Windows 10+ it is built-in; on macOS/Linux install curl."
        )
    return found


def _subprocess_kwargs() -> dict:
    """Hide console windows when spawning curl from the GUI app on Windows."""
    kwargs: dict = {
        "capture_output": True,
        "text": True,
        "check": False,
    }
    if sys.platform == "win32":
        # CREATE_NO_WINDOW: curl is a console exe; without this, a DOS box flashes per file.
        kwargs["creationflags"] = subprocess.CREATE_NO_WINDOW
        kwargs["startupinfo"] = subprocess.STARTUPINFO()
        kwargs["startupinfo"].dwFlags |= subprocess.STARTF_USESHOWWINDOW
        kwargs["startupinfo"].wShowWindow = 0  # SW_HIDE
    return kwargs


def download_file(url: str, dest: Path) -> bool:
    """Download url to dest. Returns False on HTTP 404; raises on other failures."""
    dest.parent.mkdir(parents=True, exist_ok=True)
    curl = _curl_path()
    proc = subprocess.run(
        [curl, "-L", "-s", "-f", "-o", str(dest), url],
        **_subprocess_kwargs(),
    )
    if proc.returncode == 22:
        if dest.exists():
            dest.unlink(missing_ok=True)
        return False
    if proc.returncode != 0:
        err = (proc.stderr or "").strip()
        raise RuntimeError(f"curl failed (exit {proc.returncode}): {err}\nURL: {url}")
    return True


def download_version(
    version: str,
    latest: bool,
    *,
    done: int,
    total: int,
    progress: ProgressCallback | None = None,
) -> tuple[int, int, str]:
    """Download one version folder. Returns (success_delta, done, error_message)."""
    files = firmware_file_list(version)
    target = firmware_dir(version, latest)
    success = 0
    error = ""
    current = ""
    try:
        for name in files:
            current = name
            done += 1
            if progress:
                progress(done, total, name)
            url = github_firmware_url(version, latest, name)
            if download_file(url, target / name):
                success += 1
    except Exception as exc:  # noqa: BLE001 - surface to UI
        url = github_firmware_url(version, latest, current) if current else ""
        error = f"Download failed: {current}\n\n{exc}"
        if url:
            error += f"\n\nURL: {url}"
    return success, done, error


def download_all(
    version: str,
    progress: ProgressCallback | None = None,
) -> DownloadResult:
    files_per = len(firmware_file_list(version))
    total = files_per * 2
    n = 0
    done = 0
    errors: list[str] = []
    for latest in (False, True):
        got, done, err = download_version(
            version, latest, done=done, total=total, progress=progress
        )
        n += got
        if err:
            errors.append(err)
    return DownloadResult(success_count=n, total_count=total, error_message="\n\n".join(errors))
