#!/usr/bin/env python3
"""Publish TeenAstroUploader wiki as several pages with fresh screenshots."""
from __future__ import annotations

import base64
import json
import os
import re
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

TOKEN = os.environ.get("IOGROUP")
if not TOKEN or len(TOKEN) != 64:
    raise SystemExit("IOGROUP missing or wrong length")

API = "https://groups.io/api/v1"
GROUP = "TeenAstro"
SHOTS = Path(__file__).resolve().parent / "screenshots"

WIKIIMAGE_WEMOS = "https://groups.io/g/TeenAstro/wikiimage/1768"
WIKIIMAGE_WEBSERVER = "https://groups.io/g/TeenAstro/wikiimage/2289"

MSI = (
    "https://github.com/charleslemaire0/TeenAstro/raw/refs/heads/Release_1.6/"
    "Released%20data/Firmware/TeenAstroUploader.msi"
)
MANUALS = {
    "en": "https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/Manuel_utilisateur_en.pdf",
    "fr": "https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/Manuel_utilisateur_fr.pdf",
    "de": "https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/Manuel_utilisateur_de.pdf",
}


def api(method: str, path: str, form: dict | None = None) -> dict:
    url = f"{API}/{path}"
    data = None
    headers = {"Authorization": f"Bearer {TOKEN}", "User-Agent": "TeenAstroWikiPublish/2.0"}
    if form is not None:
        data = urllib.parse.urlencode(form).encode("utf-8")
        headers["Content-Type"] = "application/x-www-form-urlencoded"
    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=180) as resp:
            raw = resp.read()
            if not raw:
                return {}
            return json.loads(raw.decode("utf-8"))
    except urllib.error.HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")
        raise SystemExit(f"HTTP {exc.code} {path}: {body[:600]}") from exc


def data_uri(name: str) -> str:
    path = SHOTS / name
    if not path.is_file():
        raise SystemExit(f"missing screenshot {path}")
    b64 = base64.b64encode(path.read_bytes()).decode("ascii")
    return f"data:image/png;base64,{b64}"


def img_tag(name: str, alt: str) -> str:
    return (
        f'<p><img src="{data_uri(name)}" alt="{alt}" '
        f'loading="lazy" class="myimg-responsive myimg-responsive"/></p>\n'
    )


def img_url(url: str, alt: str = "", width: int | None = None) -> str:
    w = f' width="{width}"' if width else ""
    return (
        f'<p><img src="{url}" alt="{alt}"{w} '
        f'loading="lazy" class="myimg-responsive myimg-responsive"/></p>\n'
    )


def p(html: str) -> str:
    return f"<p>{html}</p>\n"


def h(text: str) -> str:
    return f"<p><strong>{text}</strong></p>\n"


def ul(items: list[str]) -> str:
    return "<ul>\n" + "".join(f"<li>{i}</li>\n" for i in items) + "</ul>\n"


def a(href: str, text: str) -> str:
    return f'<a href="{href}" rel="nofollow noopener" target="_blank">{text}</a>'


def wiki_a(page_id: int | str, text: str) -> str:
    return f'<a href="https://groups.io/g/TeenAstro/wiki/{page_id}" rel="nofollow">{text}</a>'


def update_page(page_id: int, title: str, body: str, edit_msg: str) -> dict:
    form = {
        "group_name": GROUP,
        "id": str(page_id),
        "title": title,
        "body": body,
        "edit_msg": edit_msg,
    }
    result = api("POST", "updatewikipage", form)
    if result.get("object") == "error":
        raise SystemExit(f"update {page_id}: {result}")
    got = api("GET", f"getwikipageset?group_name={GROUP}&id={page_id}")
    print(f"updated {page_id} [{got['page']['title']}] bytes={len(got['data']['body'] or '')}")
    return got


def create_or_find(title: str, body: str) -> int:
    """Return page id for title; create if missing."""
    pages: list[dict] = []
    token = None
    while True:
        path = f"getwikipages?group_name={GROUP}&limit=100"
        if token:
            path += f"&page_token={urllib.parse.quote(str(token))}"
        d = api("GET", path)
        pages.extend(d.get("data") or [])
        if not d.get("has_more"):
            break
        token = d.get("next_page_token")
        if not token:
            break
    for page in pages:
        if page.get("title") == title:
            pid = int(page["id"])
            update_page(pid, title, body, "Refresh TeenAstroUploader docs + screenshots")
            return pid
    result = api(
        "POST",
        "newwikipage",
        {"group_name": GROUP, "title": title, "body": body},
    )
    pid = int(result["page"]["id"])
    print(f"created {pid} [{title}]")
    return pid


def delete_page(page_id: int) -> None:
    try:
        api("POST", "deletewikipage", {"group_name": GROUP, "id": str(page_id)})
        print(f"deleted {page_id}")
    except SystemExit as exc:
        print(f"delete {page_id}: {exc}")


def build_hub(ids: dict[str, int]) -> str:
    parts: list[str] = []
    parts.append(
        p(
            '<span style="font-family: arial, helvetica, sans-serif; color: #ecf0f1; '
            'background-color: #ba372a;"><strong>Save mount / focuser parameters before '
            "upgrading firmware.</strong></span>"
        )
    )
    parts.append(
        p(
            "On TeenAstro <strong>1.5+</strong>, the Windows Uploader can do that itself with "
            f"<strong>Auto!</strong> and <strong>EEPROM</strong> — see "
            f"{wiki_a(ids['auto'], 'Auto! and EEPROM')}."
        )
    )
    parts.append(h("TeenAstro Firmware Uploader for Windows"))
    parts.append(
        p(
            "These pages describe the current Windows uploader (MSI). "
            "Windows 10 and Windows 11."
        )
    )
    parts.append(h("Install"))
    parts.append(
        p(
            f"Download and install {a(MSI, 'TeenAstroUploader.msi')}. "
            "Start <strong>TeenAstro Firmware Uploader</strong> from the Start Menu "
            "(or let the installer launch it)."
        )
    )
    parts.append(img_tag("uploader_telescope.png", "TeenAstroUploader — Telescope tab"))
    parts.append(h("Pages in this guide"))
    parts.append(
        ul(
            [
                f"{wiki_a(ids['hub'], 'Overview (this page)')} — install and download",
                f"{wiki_a(ids['telescope'], 'Telescope')} — Upload!, Auto!, EEPROM",
                f"{wiki_a(ids['focuser'], 'Focuser')} — same buttons for the focuser board",
                f"{wiki_a(ids['shc'], 'Hand controller')} — COM and Wi‑Fi flash",
                f"{wiki_a(ids['auto'], 'Auto! and EEPROM')} — backup / restore / parameter editor",
            ]
        )
    )
    parts.append(h("Download firmware"))
    parts.append(
        p(
            "Select <strong>Firmware Version</strong>, choose <strong>Stable</strong> or "
            "<strong>Latest</strong>, then press <strong>Download!</strong>. "
            "<strong>Open Folder</strong> shows the files."
        )
    )
    parts.append(h("Requirements"))
    parts.append(
        ul(
            [
                "Hand controller and MainUnit from the <strong>same release</strong>",
                "<strong>Auto!</strong> / <strong>EEPROM</strong> need firmware <strong>1.5+</strong>",
                "Parameter JSON backups: <code>%LocalAppData%\\TeenAstro\\Config\\</code>",
            ]
        )
    )
    parts.append(
        p(
            "Field manuals (PDF): "
            f"{a(MANUALS['en'], 'English')}, "
            f"{a(MANUALS['fr'], 'Français')}, "
            f"{a(MANUALS['de'], 'Deutsch')}."
        )
    )
    parts.append(
        p(
            f"Linux notes: {wiki_a(30679, 'Uploaders for Linux')}."
        )
    )
    return "".join(parts)


def build_telescope(ids: dict[str, int]) -> str:
    parts: list[str] = []
    parts.append(h("TeenAstroUploader — Telescope"))
    parts.append(p(f"Part of {wiki_a(ids['hub'], 'TeenAstroUploader for Windows')}."))
    parts.append(img_tag("uploader_telescope.png", "Telescope tab"))
    parts.append(
        p(
            "Connect USB to the <strong>Telescope</strong> port only and power on. "
            "The first time, Windows installs the Teensy driver (Teensyduino RawHID)."
        )
    )
    parts.append(h("Buttons"))
    parts.append(
        ul(
            [
                "<strong>Upload!</strong> — flash the selected PCB only. Use on firmware "
                "older than 1.5, or when settings were saved another way. "
                "PCB type is on the fourth screen after power-on (or the board marking).",
                "<strong>Auto!</strong> — detect board, backup parameters to JSON, flash, "
                "restore after reboot, verify. Needs <strong>1.5+</strong>. "
                "Works on the <strong>active</strong> of the two stored mounts. "
                f"Details: {wiki_a(ids['auto'], 'Auto! and EEPROM')}.",
                "<strong>EEPROM</strong> — full parameter editor (Webserver groups). "
                f"See {wiki_a(ids['auto'], 'Auto! and EEPROM')}.",
            ]
        )
    )
    parts.append(
        p(
            f"Next: {wiki_a(ids['focuser'], 'Focuser')} · "
            f"{wiki_a(ids['shc'], 'Hand controller')} · "
            f"{wiki_a(ids['hub'], 'Overview')}"
        )
    )
    return "".join(parts)


def build_focuser(ids: dict[str, int]) -> str:
    parts: list[str] = []
    parts.append(h("TeenAstroUploader — Focuser"))
    parts.append(p(f"Part of {wiki_a(ids['hub'], 'TeenAstroUploader for Windows')}."))
    parts.append(img_tag("uploader_focuser.png", "Focuser tab"))
    parts.append(
        p(
            "Connect USB to the <strong>Focuser</strong> port only and power on. "
            "First connection installs the Teensy RawHID driver."
        )
    )
    parts.append(
        p(
            "Same actions as Telescope: <strong>Upload!</strong>, "
            "<strong>Auto!</strong> (1.5+), <strong>EEPROM</strong>."
        )
    )
    parts.append(
        p(
            f"Parameter details: {wiki_a(ids['auto'], 'Auto! and EEPROM')}. "
            f"Back: {wiki_a(ids['hub'], 'Overview')}."
        )
    )
    return "".join(parts)


def build_shc(ids: dict[str, int]) -> str:
    parts: list[str] = []
    parts.append(h("TeenAstroUploader — Hand controller"))
    parts.append(p(f"Part of {wiki_a(ids['hub'], 'TeenAstroUploader for Windows')}."))
    parts.append(img_tag("uploader_handcontroller.png", "Hand controller tab"))
    parts.append(h("First flash over COM"))
    parts.append(
        p(
            "Power off, remove the Wemos ESP8266 from its socket (or use your board’s "
            "programming cable), connect micro‑USB. Windows installs the CH340 (or similar) COM port."
        )
    )
    parts.append(img_url(WIKIIMAGE_WEMOS, "Wemos USB", width=200))
    parts.append(
        p(
            "Uploader → <strong>Hand controler</strong>: language, <strong>ComPort</strong>, "
            "<strong>Upload over COM!</strong>."
        )
    )
    parts.append(h("Later updates over Wi‑Fi"))
    parts.append(
        p(
            "Power on, join TeenAstro Wi‑Fi (or your station network). "
            "IP: <strong>Telescope Settings → Wifi → Show IP</strong> "
            "(AP default often <code>192.168.0.1</code>). "
            "Enter <strong>IP Adress</strong>, then <strong>Upload over WIFI!</strong>."
        )
    )
    parts.append(h("Webserver alternative"))
    parts.append(
        p(
            "In the Webserver Wi‑Fi section (password default <code>password</code>), "
            "use <strong>Update Firmware</strong> with the matching "
            "<code>TeenAstroSHC_*.bin</code> from the downloaded folder."
        )
    )
    parts.append(img_url(WIKIIMAGE_WEBSERVER, "Webserver firmware update"))
    parts.append(p(f"Back: {wiki_a(ids['hub'], 'Overview')}."))
    return "".join(parts)


def build_auto(ids: dict[str, int]) -> str:
    parts: list[str] = []
    parts.append(h("TeenAstroUploader — Auto! and EEPROM"))
    parts.append(
        p(
            "Firmware <strong>1.5 or newer</strong>. Older branches are not supported for "
            "backup/restore — use <strong>Upload!</strong> only after saving settings another way. "
            f"Main guide: {wiki_a(ids['hub'], 'TeenAstroUploader for Windows')}."
        )
    )
    parts.append(h("Auto!"))
    parts.append(
        p(
            "On the Telescope or Focuser tab: detect board → backup parameters to JSON → "
            "flash → restore after reboot → verify. "
            "TeenAstro stores <strong>two mounts</strong> (0 and 1); Auto! works on the "
            "<strong>active</strong> mount."
        )
    )
    parts.append(img_tag("uploader_telescope.png", "Auto! on Telescope tab"))
    parts.append(h("EEPROM editor"))
    parts.append(
        p(
            "Opens a dialog with the same groups as the Webserver: "
            "Mount, Motors, Speed, Limits, Encoders, Site, Tracking."
        )
    )
    parts.append(img_tag("uploader_eeprom_mount.png", "EEPROM — Mount"))
    parts.append(img_tag("uploader_eeprom_motors.png", "EEPROM — Motors"))
    parts.append(
        ul(
            [
                "<strong>Read</strong> / <strong>Write to EEPROM</strong> — after Write, "
                "values are re-read and checked",
                "<strong>Save…</strong> / <strong>Load…</strong> — JSON under "
                "<code>%LocalAppData%\\TeenAstro\\Config\\</code> by default",
                "<strong>Activate (reboot)</strong> — switch between the two stored mounts",
            ]
        )
    )
    parts.append(h("Legacy: TeenAstroConfig.exe"))
    parts.append(
        p(
            "Jürgen Goldan’s "
            + a(
                "http://www.goldan.org/juergen/downloads/TeenAstroConfig.exe",
                "TeenAstroConfig.exe",
            )
            + " remains available if you already use its files. "
            "For new setups on 1.5+, prefer Auto! / EEPROM in the Uploader."
        )
    )
    parts.append(
        p(
            f"Also: {wiki_a(ids['telescope'], 'Telescope')} · "
            f"{wiki_a(ids['focuser'], 'Focuser')}."
        )
    )
    return "".join(parts)


def update_sidebar(ids: dict[str, int]) -> None:
    d = api("GET", f"getwikipageset?group_name={GROUP}&id=7362")
    body = d["data"]["body"] or ""
    # Replace Backup & Config link text/target to Auto! and EEPROM page
    new_body = body
    # path-based link
    new_body = re.sub(
        r'<a href="https://groups\.io/g/TeenAstro/wiki/TeenAstroConfig"[^>]*>Backup &amp; Config</a>',
        f'<a href="https://groups.io/g/TeenAstro/wiki/{ids["auto"]}" rel="nofollow">'
        f"Uploader Auto! &amp; EEPROM</a>",
        new_body,
    )
    # also id form if present
    new_body = re.sub(
        r'<a href="https://groups\.io/g/TeenAstro/wiki/10404"[^>]*>[^<]*</a>',
        f'<a href="https://groups.io/g/TeenAstro/wiki/{ids["auto"]}" rel="nofollow">'
        f"Uploader Auto! &amp; EEPROM</a>",
        new_body,
    )
    # Expand Firmware Uploader Windows entry with sub-links if not already present
    marker = "<!-- ta-uploader-subpages -->"
    if marker not in new_body:
        sub = (
            f"{marker}\n"
            f"<ul>\n"
            f'<li>{wiki_a(ids["telescope"], "… Telescope")}</li>\n'
            f'<li>{wiki_a(ids["focuser"], "… Focuser")}</li>\n'
            f'<li>{wiki_a(ids["shc"], "… Hand controller")}</li>\n'
            f'<li>{wiki_a(ids["auto"], "… Auto! &amp; EEPROM")}</li>\n'
            f"</ul>\n"
        )
        # insert after Firmware Uploader Windows link
        new_body = re.sub(
            r'(<a href="https://groups\.io/g/TeenAstro/wiki/8837"[^>]*>Firmware Uploader Windows</a>)',
            r"\1\n" + sub,
            new_body,
            count=1,
        )
    else:
        # refresh ids inside marker block
        new_body = re.sub(
            marker + r".*?</ul>",
            marker
            + "\n<ul>\n"
            + f'<li>{wiki_a(ids["telescope"], "… Telescope")}</li>\n'
            + f'<li>{wiki_a(ids["focuser"], "… Focuser")}</li>\n'
            + f'<li>{wiki_a(ids["shc"], "… Hand controller")}</li>\n'
            + f'<li>{wiki_a(ids["auto"], "… Auto! &amp; EEPROM")}</li>\n'
            + "</ul>",
            new_body,
            count=1,
            flags=re.S,
        )

    if new_body == body:
        print("sidebar unchanged")
        return
    update_page(7362, "_Sidebar", new_body, "Link TeenAstroUploader multi-page guide")


def main() -> None:
    # Remove accidental probe page if still present
    delete_page(43069)

    # Fixed IDs we repurpose
    ids: dict[str, int] = {
        "hub": 8837,
        "auto": 10404,
    }

    # Create/update child pages first (body with placeholders, then refresh with real ids)
    # Use temporary bodies, then second pass once all ids known.
    titles = {
        "telescope": "TeenAstroUploader — Telescope",
        "focuser": "TeenAstroUploader — Focuser",
        "shc": "TeenAstroUploader — Hand controller",
    }
    for key, title in titles.items():
        ids[key] = create_or_find(title, p("Updating…"))

    # Second pass: full content with cross-links
    update_page(
        ids["hub"],
        "TeenAstroUploader for Windows",
        build_hub(ids),
        "Multi-page guide + recent screenshots",
    )
    update_page(
        ids["auto"],
        "TeenAstroUploader — Auto! and EEPROM",
        build_auto(ids),
        "Replace TeenAstroConfig page with Uploader Auto/EEPROM docs",
    )
    update_page(
        ids["telescope"],
        titles["telescope"],
        build_telescope(ids),
        "Uploader Telescope page",
    )
    update_page(
        ids["focuser"],
        titles["focuser"],
        build_focuser(ids),
        "Uploader Focuser page",
    )
    update_page(
        ids["shc"],
        titles["shc"],
        build_shc(ids),
        "Uploader Hand controller page",
    )
    update_sidebar(ids)

    print("IDS", json.dumps(ids))
    for key, pid in ids.items():
        print(f"  https://groups.io/g/TeenAstro/wiki/{pid}  ({key})")


if __name__ == "__main__":
    main()
