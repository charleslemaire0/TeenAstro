"""Startup check for a newer uploader release and newer published firmware."""

from __future__ import annotations

import json
from dataclasses import dataclass
from datetime import datetime, timedelta, timezone
from urllib.parse import quote

from .download import http_get_text
from .paths import firmware_base_path
from .version_util import parse_firmware_version

_REPO = "charleslemaire0/TeenAstro"
_API = f"https://api.github.com/repos/{_REPO}"
_RELEASES = "https://github.com/charleslemaire0/TeenAstro/releases/latest"
# A download in the same minute as the publish is not "older".
_FRESH_SLACK = timedelta(seconds=120)


@dataclass(frozen=True)
class UpdateInfo:
    uploader_newer: str | None = None
    uploader_url: str = ""
    firmware_notes: tuple[str, ...] = ()


def uploader_version_from_release(payload: dict) -> tuple[str, str] | None:
    """Return (version, html url) when the release tag is a newer-style version."""
    tag = str(payload.get("tag_name") or "").lstrip("vV")
    if parse_firmware_version(tag) is None:
        return None
    url = str(payload.get("html_url") or _RELEASES)
    return tag, url


def commit_time(payload: object) -> datetime | None:
    """Committer time from a GitHub commits list, or None when the path is absent."""
    if not isinstance(payload, list) or not payload:
        return None
    try:
        raw = payload[0]["commit"]["committer"]["date"]
    except (KeyError, IndexError, TypeError):
        return None
    text = str(raw).replace("Z", "+00:00")
    try:
        when = datetime.fromisoformat(text)
    except ValueError:
        return None
    if when.tzinfo is None:
        when = when.replace(tzinfo=timezone.utc)
    return when.astimezone(timezone.utc)


def release_lines_from_refs(payload: object) -> tuple[str, ...]:
    """X.Y names from matching-refs entries such as refs/heads/Release_1.6."""
    if not isinstance(payload, list):
        return ()
    found: list[tuple[int, int, str]] = []
    for item in payload:
        if not isinstance(item, dict):
            continue
        ref = str(item.get("ref") or "")
        name = ref.rsplit("/", 1)[-1]
        if not name.startswith("Release_"):
            continue
        ver = parse_firmware_version(name[len("Release_"):])
        if ver is None:
            continue
        found.append((ver[0], ver[1], f"{ver[0]}.{ver[1]}"))
    found.sort()
    # One entry per X.Y. A Release_1.6.3 ref still names the 1.6 line.
    lines: list[str] = []
    for _major, _minor, label in found:
        if label not in lines:
            lines.append(label)
    return tuple(lines)


def firmware_note(version: str, channel: str, remote: datetime | None, local: datetime | None) -> str | None:
    """One sentence when published firmware is missing locally or newer than the download."""
    if remote is None:
        return None
    label = f"{version} {channel}"
    if local is None:
        return f"{label} is published and not downloaded yet."
    if remote > local + _FRESH_SLACK:
        return f"{label} was updated after your download."
    return None


def higher_release_note(offered: tuple[str, ...], remote_lines: tuple[str, ...]) -> str | None:
    """A release line above the versions this uploader can select."""
    offered_ver = [parse_firmware_version(v) for v in offered]
    offered_ver = [v for v in offered_ver if v is not None]
    if not offered_ver:
        return None
    ceiling = max(offered_ver)
    newer = []
    for line in remote_lines:
        ver = parse_firmware_version(line)
        if ver is not None and (ver[0], ver[1], 0) > ceiling:
            newer.append(line)
    if not newer:
        return None
    newest = max(newer, key=lambda line: parse_firmware_version(line) or (0, 0, 0))
    offered_label = f"{ceiling[0]}.{ceiling[1]}"
    return f"Firmware {newest} is published. This uploader offers up to {offered_label}."


def format_notice(local_uploader: str, info: UpdateInfo) -> str | None:
    lines: list[str] = []
    if info.uploader_newer:
        lines.append(f"A newer uploader is available: {info.uploader_newer}")
        lines.append(f"This copy is {local_uploader}.")
        if info.uploader_url:
            lines.append(info.uploader_url)
    if info.firmware_notes:
        if lines:
            lines.append("")
        lines.append("Newer firmware:")
        lines.extend(info.firmware_notes)
    if not lines:
        return None
    return "\n".join(lines)


def local_firmware_time(version: str, latest: bool) -> datetime | None:
    """Newest downloaded hex or bin in the version folder, or None when the folder is empty."""
    name = f"{version}_latest" if latest else version
    folder = firmware_base_path() / name
    if not folder.is_dir():
        return None
    times = [
        path.stat().st_mtime
        for path in folder.iterdir()
        if path.is_file() and path.suffix.lower() in {".hex", ".bin"}
    ]
    if not times:
        return None
    return datetime.fromtimestamp(max(times), tz=timezone.utc)


def _get_json(url: str) -> object | None:
    text = http_get_text(url)
    if not text:
        return None
    try:
        return json.loads(text)
    except json.JSONDecodeError:
        return None


def _firmware_commit_url(version: str, latest: bool) -> str:
    folder = f"{version}_latest" if latest else version
    path = f"TeenAstroUploader/TeenAstroUploader/{folder}"
    return (
        f"{_API}/commits?sha={quote(f'Release_{version}', safe='')}"
        f"&path={quote(path, safe='/')}&per_page=1"
    )


def collect_update(local_uploader: str, offered_firmware: tuple[str, ...]) -> UpdateInfo:
    """Ask GitHub. A failed request leaves that part out of the notice."""
    uploader_newer = None
    uploader_url = ""
    local_ver = parse_firmware_version(local_uploader)
    release = _get_json(f"{_API}/releases/latest")
    if isinstance(release, dict):
        parsed = uploader_version_from_release(release)
        if parsed is not None and local_ver is not None:
            remote_ver = parse_firmware_version(parsed[0])
            if remote_ver is not None and remote_ver > local_ver:
                uploader_newer, uploader_url = parsed

    notes: list[str] = []
    refs = _get_json(f"{_API}/git/matching-refs/heads/Release_")
    higher = higher_release_note(offered_firmware, release_lines_from_refs(refs))
    if higher:
        notes.append(higher)

    for version in offered_firmware:
        if parse_firmware_version(version) is None:
            continue
        for latest, channel in ((False, "Stable"), (True, "Latest")):
            remote = commit_time(_get_json(_firmware_commit_url(version, latest)))
            note = firmware_note(version, channel, remote, local_firmware_time(version, latest))
            if note:
                notes.append(note)

    return UpdateInfo(uploader_newer, uploader_url, tuple(notes))


def startup_notice(local_uploader: str, offered_firmware: tuple[str, ...]) -> str | None:
    return format_notice(local_uploader, collect_update(local_uploader, offered_firmware))
