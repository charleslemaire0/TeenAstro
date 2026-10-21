#!/usr/bin/env python3
"""Publish ASCOM Alpaca + ESP32-S3 upgrade wiki pages; add Alpaca logo on Home."""
from __future__ import annotations

import base64
import json
import os
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

TOKEN = os.environ.get("IOGROUP")
if not TOKEN or len(TOKEN) != 64:
    raise SystemExit("IOGROUP missing or wrong length")

API = "https://groups.io/api/v1"
GROUP = "TeenAstro"
ROOT = Path(__file__).resolve().parent
SHOTS = ROOT / "screenshots"

MANUALS = {
    "en": "https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/Manuel_utilisateur_en.pdf",
    "fr": "https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/Manuel_utilisateur_fr.pdf",
    "de": "https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/Manuel_utilisateur_de.pdf",
}

ASCOM_STANDARDS = "https://ascom-standards.org/"
ALPACA_SITE = "https://www.ascom-alpaca.org/"

PAGE_WIFI = 14592
PAGE_WEBSERVER = 8105
PAGE_ASCOM_DRIVER = 8102
PAGE_UPLOADER_SHC = 43072
PAGE_START = 24430
PAGE_HOME = 7361


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
        raise SystemExit(f"missing {path}")
    b64 = base64.b64encode(path.read_bytes()).decode("ascii")
    suffix = path.suffix.lower().lstrip(".")
    mime = "jpeg" if suffix in ("jpg", "jpeg") else suffix
    return f"data:image/{mime};base64,{b64}"


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


def img_data(name: str, alt: str, width: int | None = None) -> str:
    w = f' width="{width}"' if width else ""
    return (
        f'<p><img src="{data_uri(name)}" alt="{alt}"{w} '
        f'loading="lazy" class="myimg-responsive myimg-responsive"/></p>\n'
    )


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


def create_or_find(title: str, body: str, edit_msg: str) -> int:
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
            update_page(pid, title, body, edit_msg)
            return pid
    result = api(
        "POST",
        "newwikipage",
        {"group_name": GROUP, "title": title, "body": body},
    )
    if result.get("object") == "error":
        raise SystemExit(f"newwikipage {title}: {result}")
    pid = int(result["page"]["id"])
    print(f"created {pid} [{title}]")
    return pid


def manuals_line() -> str:
    return (
        "Field manuals: "
        f"{a(MANUALS['en'], 'EN')} · {a(MANUALS['fr'], 'FR')} · {a(MANUALS['de'], 'DE')}"
    )


def build_alpaca(page_upgrade: int) -> str:
    parts: list[str] = []
    parts.append(h("ASCOM Alpaca"))
    parts.append(
        p(
            "TeenAstro can speak <strong>ASCOM Alpaca</strong> (HTTP/REST + JSON) over Wi‑Fi. "
            "Compatible apps (NINA, Stellarium, KStars, ASCOM Remote, custom scripts, …) "
            "discover the mount on the LAN and drive it without the classic Windows COM driver "
            "or the LX200 TCP bridge on port 9999."
        )
    )
    parts.append(
        f'<p><a href="{ASCOM_STANDARDS}" rel="nofollow noopener" target="_blank">'
        f'<img src="{data_uri("ASCOM_Alpaca_logo.png")}" alt="ASCOM Alpaca" width="128" '
        f'loading="lazy" class="myimg-responsive myimg-responsive"/></a></p>\n'
    )
    parts.append(h("What you need"))
    parts.append(
        ul(
            [
                "<strong>Hand controller with a Lolin S3 Mini (ESP32‑S3)</strong>. "
                "The production Wemos D1 Mini (ESP8266) image has no room for Alpaca. "
                f"Swap the module and flash the S3 firmware — see "
                f"{wiki_a(page_upgrade, 'Upgrade SHC to ESP32‑S3')}.",
                "<strong>Firmware that includes Alpaca</strong>. "
                "SHC builds named <code>TeenAstroSHC_*_S3_English.bin</code> "
                "(or French / German). The web UI stays on port <strong>80</strong>; "
                "Alpaca listens on TCP <strong>11111</strong>.",
                "<strong>Wi‑Fi on</strong>. "
                "<strong>Telescope Settings → Wifi → Turn Wifi on</strong>, then reboot. "
                f"Access Point or Station: {wiki_a(PAGE_WIFI, 'WiFi Interface')}. "
                "Read the address with <strong>Show IP</strong>.",
                "<strong>Same network as the computer or phone</strong>. "
                "In Access Point mode join <strong>TeenAstro</strong> "
                "(default <strong>192.168.0.1</strong>). "
                "In Station mode use the address from <strong>Show IP</strong>.",
                "<strong>An Alpaca‑aware client</strong>. "
                "Discovery uses UDP <strong>32227</strong> "
                "(<code>alpacadiscovery1</code>). Most clients find the device automatically; "
                "otherwise point them at the IP and port <strong>11111</strong>.",
            ]
        )
    )
    parts.append(h("Ports"))
    parts.append(
        "<table border=\"1\" cellpadding=\"4\" cellspacing=\"0\">"
        "<tr><th>Service</th><th>Port</th></tr>"
        "<tr><td>Web configuration UI</td><td>TCP 80</td></tr>"
        "<tr><td>LX200 TCP bridge</td><td>TCP 9999</td></tr>"
        "<tr><td>ASCOM Alpaca API</td><td>TCP 11111</td></tr>"
        "<tr><td>Alpaca discovery</td><td>UDP 32227</td></tr>"
        "</table>\n"
    )
    parts.append(
        p(
            f"The classic {wiki_a(PAGE_ASCOM_DRIVER, 'ASCOM Driver')} "
            "(COM / IP on 9999) remains available. Alpaca is the direct network path "
            "for apps that speak the Alpaca REST API."
        )
    )
    parts.append(h("Devices exposed"))
    parts.append(
        ul(
            [
                "<strong>Telescope</strong> — goto, sync, tracking, park/home, "
                "pulse guide, move axis, site/time.",
                "<strong>Focuser</strong> — when a focuser is attached to the main unit.",
            ]
        )
    )
    parts.append(h("Related"))
    parts.append(
        ul(
            [
                wiki_a(page_upgrade, "Upgrade SHC to ESP32‑S3"),
                wiki_a(PAGE_WIFI, "WiFi Interface"),
                wiki_a(PAGE_WEBSERVER, "Webserver"),
                wiki_a(PAGE_ASCOM_DRIVER, "ASCOM Driver"),
                manuals_line(),
            ]
        )
    )
    return "".join(parts)


def build_upgrade(page_alpaca: int) -> str:
    parts: list[str] = []
    parts.append(h("Upgrade SHC from Wemos / Lolin to ESP32‑S3"))
    parts.append(
        p(
            "The hand‑controller PCB that takes a <strong>Wemos D1 Mini</strong> "
            "(ESP8266, often sold as Lolin) also accepts a <strong>Lolin S3 Mini</strong> "
            "(ESP32‑S3). The S3 module is required for "
            f"{wiki_a(page_alpaca, 'ASCOM Alpaca')} "
            "(the ESP8266 1&nbsp;MB image has no room for it). "
            "The rest of the PCB, OLED and buttons stay the same."
        )
    )
    parts.append(h("What you need"))
    parts.append(
        ul(
            [
                "A <strong>Lolin S3 Mini</strong> (ESP32‑S3), USB‑C.",
                "The TeenAstro <strong>S3</strong> hand‑controller firmware: "
                "<code>TeenAstroSHC_&lt;version&gt;_S3_English.bin</code> "
                "(or <code>_S3_French</code> / <code>_S3_German</code>). "
                "That file is a <strong>merged</strong> flash image "
                "(bootloader + partitions + app at address <code>0x0</code>).",
                "A USB cable and a computer with esptool or PlatformIO.",
            ]
        )
    )
    parts.append(h("Hardware swap"))
    parts.append(
        ul(
            [
                "Power the mount <strong>off</strong>.",
                "Remove the Wemos D1 Mini / Lolin ESP8266 from its socket.",
                "Insert the <strong>Lolin S3 Mini</strong>. "
                "The module has a dual‑row header; <strong>only the outer row</strong> "
                "mates with the D1 Mini socket (USB at the bottom of the module, "
                "antenna at the top — same orientation as the Wemos). "
                "Do not force the inner row into the shield.",
                "Leave the OLED and button wiring unchanged. "
                "The firmware already maps the S3 outer pins to the same roles as the Wemos.",
            ]
        )
    )
    parts.append(h("First flash (USB)"))
    parts.append(
        p(
            "The Lolin S3 Mini enumerates as a USB serial device (CDC). "
            "Prefer a direct flash over COM for the first image."
        )
    )
    parts.append(
        p(
            "With esptool (replace <code>COMx</code> and the bin path):"
        )
    )
    parts.append(
        "<pre>esptool --chip esp32s3 --port COMx --baud 921600 "
        "write_flash 0x0 TeenAstroSHC_&lt;version&gt;_S3_English.bin</pre>\n"
    )
    parts.append(p("Or from the repo with PlatformIO:"))
    parts.append("<pre>pio run -d TeenAstroSHC -e esp32s3 -t upload</pre>\n")
    parts.append(
        p(
            "(Use <code>esp32s3_FRENCH</code> or <code>esp32s3_GERMAN</code> "
            "for the other languages.)"
        )
    )
    parts.append(
        p(
            f"<strong>TeenAstroUploader</strong> "
            f"({wiki_a(PAGE_UPLOADER_SHC, 'Hand controller')} tab) "
            "“Upload over COM!” still targets the ESP8266 chip by default. "
            "Until an S3 board choice appears there, use esptool or PlatformIO "
            "for the first S3 flash. Later updates can use the Webserver "
            "<strong>Update Firmware</strong> page with the "
            "<strong>application</strong> <code>.bin</code> "
            "(not the merged image)."
        )
    )
    parts.append(h("After the upgrade"))
    parts.append(
        ul(
            [
                "Power on. Confirm the usual TeenAstro splash and menus.",
                "Turn Wi‑Fi on (<strong>Telescope Settings → Wifi</strong>), "
                "reboot, read <strong>Show IP</strong>.",
                f"Alpaca is available on TCP <strong>11111</strong> "
                f"(discovery UDP <strong>32227</strong>). "
                f"See {wiki_a(page_alpaca, 'ASCOM Alpaca')}.",
                "The LX200 TCP bridge on port <strong>9999</strong> and the classic "
                "ASCOM driver keep working as before.",
            ]
        )
    )
    parts.append(h("Related"))
    parts.append(
        ul(
            [
                wiki_a(page_alpaca, "ASCOM Alpaca"),
                wiki_a(PAGE_UPLOADER_SHC, "TeenAstroUploader — Hand controller"),
                wiki_a(PAGE_WIFI, "WiFi Interface"),
                wiki_a(PAGE_START, "Start here"),
            ]
        )
    )
    return "".join(parts)


def patch_home(page_alpaca: int) -> None:
    got = api("GET", f"getwikipageset?group_name={GROUP}&id={PAGE_HOME}")
    body = got["data"]["body"] or ""
    if "ASCOM_Alpaca_logo" in body or "ASCOM Alpaca" in body and "11111" in body:
        # Prefer an idempotent logo insert even if text already mentions Alpaca.
        pass
    logo = (
        f'<a href="https://groups.io/g/TeenAstro/wiki/{page_alpaca}" rel="nofollow">'
        f'<img src="{data_uri("ASCOM_Alpaca_logo.png")}" alt="ASCOM Alpaca" '
        f'width="96" height="69" loading="lazy" '
        f'class="myimg-responsive myimg-responsive"/></a>'
    )
    marker = (
        '<a href="https://forum.astronomie.de/threads/teenastro-ein-onstep-projekt.259256/page-33" '
        'rel="nofollow noopener" target="_blank">Astronomie.de</a>'
    )
    if "ASCOM_Alpaca_logo.png" in body or 'alt="ASCOM Alpaca"' in body:
        print("home already has Alpaca logo — refreshing body logo only if needed")
        # Replace existing logo block if present
        import re

        body2, n = re.subn(
            r'<a href="https://groups\.io/g/TeenAstro/wiki/\d+" rel="nofollow">'
            r'<img[^>]*alt="ASCOM Alpaca"[^>]*/></a>\s*',
            logo + "   ",
            body,
            count=1,
        )
        if n:
            body = body2
        else:
            print("could not locate previous logo for refresh; leaving home as-is")
            return
    elif marker in body:
        body = body.replace(
            marker,
            logo + "   " + marker,
            1,
        )
    else:
        raise SystemExit("home page: could not find Astronomie.de marker for logo insert")
    update_page(PAGE_HOME, "Home", body, "Add ASCOM Alpaca symbol on Home")


def patch_ascom_driver(page_alpaca: int, page_upgrade: int) -> None:
    got = api("GET", f"getwikipageset?group_name={GROUP}&id={PAGE_ASCOM_DRIVER}")
    body = got["data"]["body"] or ""
    note = (
        "<p><strong>ASCOM Alpaca (Wi‑Fi REST)</strong> — "
        "With a Lolin S3 Mini hand controller, apps can also use the built‑in "
        f"Alpaca server (TCP 11111) instead of this Windows driver. "
        f"See {wiki_a(page_alpaca, 'ASCOM Alpaca')} and "
        f"{wiki_a(page_upgrade, 'Upgrade SHC to ESP32‑S3')}.</p>\n"
    )
    if "ASCOM Alpaca (Wi‑Fi REST)" in body:
        print("ASCOM Driver page already mentions Alpaca")
        return
    # Insert after the opening paragraph if present, else prepend.
    first_close = body.find("</p>")
    if first_close > 0:
        body = body[: first_close + 4] + "\n" + note + body[first_close + 4 :]
    else:
        body = note + body
    update_page(
        PAGE_ASCOM_DRIVER,
        got["page"]["title"],
        body,
        "Link ASCOM Alpaca requirements",
    )


def patch_start_here(page_upgrade: int) -> None:
    got = api("GET", f"getwikipageset?group_name={GROUP}&id={PAGE_START}")
    body = got["data"]["body"] or ""
    if "ESP32‑S3" in body or "ESP32-S3" in body or str(page_upgrade) in body:
        print("Start here already links S3 upgrade")
        return
    needle = "An older layout with a joystick is"
    insert = (
        f"<p>To enable <strong>ASCOM Alpaca</strong>, replace the Wemos / Lolin ESP8266 "
        f"with a Lolin S3 Mini: {wiki_a(page_upgrade, 'Upgrade SHC to ESP32‑S3')}.</p>\n"
    )
    idx = body.find(needle)
    if idx < 0:
        print("Start here: joystick sentence not found — skip")
        return
    # Insert after the paragraph that contains the needle.
    end = body.find("</p>", idx)
    if end < 0:
        return
    body = body[: end + 4] + "\n" + insert + body[end + 4 :]
    update_page(PAGE_START, got["page"]["title"], body, "Link ESP32-S3 SHC upgrade")


def main() -> None:
    # Create/update with placeholders first so each page can link the other.
    page_alpaca = create_or_find(
        "ASCOM Alpaca",
        p("Draft — refreshing…"),
        "Create ASCOM Alpaca page",
    )
    page_upgrade = create_or_find(
        "Upgrade SHC to ESP32-S3",
        p("Draft — refreshing…"),
        "Create ESP32-S3 upgrade page",
    )
    update_page(
        page_alpaca,
        "ASCOM Alpaca",
        build_alpaca(page_upgrade),
        "Document ASCOM Alpaca requirements",
    )
    update_page(
        page_upgrade,
        "Upgrade SHC to ESP32-S3",
        build_upgrade(page_alpaca),
        "Document Wemos/Lolin to ESP32-S3 upgrade",
    )
    patch_home(page_alpaca)
    patch_ascom_driver(page_alpaca, page_upgrade)
    patch_start_here(page_upgrade)
    print(f"https://groups.io/g/TeenAstro/wiki/{page_alpaca}")
    print(f"https://groups.io/g/TeenAstro/wiki/{page_upgrade}")
    # Write ids back into local drafts for the next edit.
    for name, pid in (("ASCOM_Alpaca.md", page_alpaca), ("Upgrade_SHC_to_ESP32S3.md", page_upgrade)):
        path = ROOT / name
        text = path.read_text(encoding="utf-8")
        lines = text.splitlines()
        for i, line in enumerate(lines):
            if line.startswith("> Groups.io:"):
                lines[i] = f"> Groups.io: https://groups.io/g/TeenAstro/wiki/{pid}"
                break
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
