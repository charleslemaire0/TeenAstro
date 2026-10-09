#!/usr/bin/env python3
"""Publish TeenAstroUploader / TeenAstroConfig wiki pages via IOGROUP API key.

Preserves every existing <img> tag (src must match before and after).
"""
from __future__ import annotations

import json
import os
import re
import urllib.error
import urllib.parse
import urllib.request

TOKEN = os.environ.get("IOGROUP")
if not TOKEN or len(TOKEN) != 64:
    raise SystemExit("IOGROUP missing or wrong length (need 64-char API key)")

API = "https://groups.io/api/v1"
GROUP = "TeenAstro"
UA = "TeenAstroWikiPublish/1.0"

IMG_RE = re.compile(r"<img\b[^>]*>", re.I)
SRC_RE = re.compile(r'\bsrc="([^"]*)"', re.I)


def api(method: str, path: str, form: dict | None = None) -> dict:
    url = f"{API}/{path}"
    data = None
    headers = {
        "Authorization": f"Bearer {TOKEN}",
        "User-Agent": UA,
    }
    if form is not None:
        data = urllib.parse.urlencode(form).encode("utf-8")
        headers["Content-Type"] = "application/x-www-form-urlencoded"
    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=180) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")
        raise SystemExit(f"HTTP {exc.code} {path}: {body[:500]}") from exc


def img_srcs(html: str) -> list[str]:
    out: list[str] = []
    for tag in IMG_RE.findall(html or ""):
        m = SRC_RE.search(tag)
        out.append(m.group(1) if m else tag)
    return out


def extract_imgs(html: str) -> list[str]:
    return IMG_RE.findall(html or "")


def p(text: str) -> str:
    return f"<p>{text}</p>\n"


def h(text: str) -> str:
    return f"<p><strong>{text}</strong></p>\n"


def build_uploader(old: str) -> str:
    imgs = extract_imgs(old)
    if len(imgs) != 6:
        raise SystemExit(f"expected 6 images on Uploader page, found {len(imgs)}")
    i0, i1, i2, i3, i4, i5 = imgs

    parts: list[str] = []
    parts.append(
        p(
            '<span style="font-family: arial, helvetica, sans-serif; color: #ecf0f1; '
            'background-color: #ba372a;"><strong>Save the mount (or focuser) parameters '
            "before you upgrade to a new firmware version.</strong></span>"
        )
    )
    parts.append(
        p(
            "For firmware <strong>1.5 or newer</strong>, the Uploader itself can do that with "
            "<strong>Auto!</strong> (backup → flash → restore → verify) or "
            "<strong>EEPROM</strong> (full editor). "
            'Jürgen Goldan\'s <a href="https://groups.io/g/TeenAstro/wiki/10404" '
            'rel="nofollow noopener" target="_blank">TeenAstroConfig</a> remains useful '
            "for older workflows."
        )
    )
    parts.append(h("TeenAstro Firmware Uploader for Windows"))
    parts.append(
        p(
            "These instructions describe how to upload firmware with a Windows 10 or "
            "Windows 11 PC."
        )
    )
    parts.append(
        p(
            'Download and install '
            '<a href="https://github.com/charleslemaire0/TeenAstro/raw/refs/heads/Release_1.6/'
            'Released%20data/Firmware/TeenAstroUploader.msi" rel="nofollow noopener" '
            'target="_blank">TeenAstroUploader.msi</a>. '
            "Start <strong>TeenAstro Firmware Uploader</strong> from the Start Menu "
            "(or let the installer launch it)."
        )
    )
    parts.append(
        p(
            'Further information look at '
            '<a href="https://www.youtube.com/watch?v=TwpL-wXFeuU" rel="nofollow noopener" '
            'target="_blank">YouTube</a>.'
        )
    )
    parts.append("<hr/>\n")
    parts.append(h("Download latest Firmware:"))
    parts.append(
        p(
            "Select <strong>Firmware Version</strong>, choose <strong>Stable</strong> or "
            "<strong>Latest</strong>, then press <strong>Download!</strong> and wait until "
            "the progress bar finishes. <strong>Open Folder</strong> shows the files."
        )
    )
    parts.append(f"<p>{i0}</p>\n")
    parts.append("<hr/>\n")

    parts.append(h("Telescope:"))
    parts.append(
        p(
            "Connect a USB cable to the Telescope USB port (connect only this) and "
            "switch On TeenAstro."
        )
    )
    parts.append(
        p(
            "Now Windows will install the teensy device driver, this appears as "
            "Teensyduino RawHID. Wait until it's done. This happens just for the first flash."
        )
    )
    parts.append(
        p(
            "Open the Uploader → tab <strong>Telescope</strong>. "
            "The Windows Firewall may ask — allow access."
        )
    )
    parts.append(
        p(
            "<strong>Upload!</strong> — flash the selected PCB only. Use on firmware older "
            "than 1.5, or when settings were saved another way. "
            "If you don't know which PCB it is, look on the fourth display after turn on "
            "(or the marking on the PCB)."
        )
    )
    parts.append(
        p(
            "<strong>Auto!</strong> — detect board, backup parameters to JSON, flash, "
            "restore after reboot, verify. Needs firmware <strong>1.5+</strong>. "
            "Refused on older branches (no longer supported for backup/restore). "
            "TeenAstro stores <strong>two mounts</strong> (0 and 1); Auto works on the active mount."
        )
    )
    parts.append(
        p(
            "<strong>EEPROM</strong> — edit parameters with the same groups as the Webserver: "
            "Mount, Motors, Speed, Limits, Encoders, Site, Tracking. "
            "Read / Write / Save / Load. After Write, values are re-read and checked. "
            "<strong>Activate (reboot)</strong> switches between the two stored mounts."
        )
    )
    parts.append(p("When using Upload! only: once the PCB is selected, click Upload!"))
    parts.append(p("The teensy bootloader appear, wait until programming is done. Finish!"))
    parts.append(f"<p>{i1}</p>\n")
    parts.append("<hr/>\n")

    parts.append(h("Focuser:"))
    parts.append(
        p(
            "Connect a USB cable to the Focuser USB port (connect only this) and "
            "switch On TeenAstro."
        )
    )
    parts.append(
        p(
            "Now Windows will install the teensy device driver, this appears as "
            "Teensyduino RawHID. Wait until it's done. This happens just for the first flash."
        )
    )
    parts.append(
        p(
            "Open the Uploader → tab <strong>Focuser</strong>. Same buttons as Telescope: "
            "<strong>Upload!</strong>, <strong>Auto!</strong> (1.5+), <strong>EEPROM</strong>."
        )
    )
    parts.append(p("The teensy bootloader appear, wait until programming is done. Finish!"))
    parts.append(f"<p>{i2}</p>\n")
    parts.append("<hr/>\n")

    parts.append(h("Hand Controller:"))
    parts.append(
        p(
            "Switch Off TeenAstro and remove the Wemos ESP8266 microcontroller from his socket."
        )
    )
    parts.append(
        p(
            "Connect a micro USB cable to the microcontroller. "
            "Now Windows will install the device driver. Wait until it's done."
        )
    )
    parts.append(f"<p>{i3}</p>\n")
    parts.append(
        p(
            "Open the Uploader → tab <strong>Hand controler</strong>. "
            "The Windows Firewall may ask — allow access."
        )
    )
    parts.append(
        p(
            "Select your ComPort. Look into device manager to find out the correct COM port. "
            "It appears at USB-SERIAL CH340."
        )
    )
    parts.append(p("Click <strong>Upload over COM!</strong>."))
    parts.append(p("A command prompt will appear, wait until programming is done. Finish!"))
    parts.append(f"<p>{i4}</p>\n")
    parts.append("<hr/>\n")

    parts.append(h("Flash Hand Controller about WiFi:"))
    parts.append(
        p(
            "If you have already flashed the microcontroller over serial ComPort, "
            "now it is possible to flash newer versions about the WiFi. "
            "This has a big advantage, it is no longer necessary to remove the "
            "microcontroller from his socket."
        )
    )
    parts.append(
        p("It can be flashed either with the TeenAstroUploader or about the Webserver.")
    )
    parts.append(h("TeenAstroUploader:"))
    parts.append(
        p("Power On TeenAstro, connect a Notebook or a Smartphone about the WiFi.")
    )
    parts.append(
        p(
            "Execute the TeenAstroUploader and enter the IP Adress 192.168.0.1 (default). "
            "If you are not sure, find out the correct IP Adress about the Handcontroller: "
            "Telescope Settings → Wifi → Show IP. Click <strong>Upload over WIFI!</strong>."
        )
    )
    parts.append(p("Finish!"))
    parts.append(h("Webserver:"))
    parts.append(
        p("Power On TeenAstro, connect a Notebook or a Smartphone about the WiFi.")
    )
    parts.append(
        p(
            "Open a Webbrowser and input 192.168.0.1 (default IP Adress), "
            "the Webserver will appear. If not, find out the correct IP Adress about "
            "the Handcontroller (Telescope Settings → Wifi → Show IP)."
        )
    )
    parts.append(
        p(
            'Go into WiFi section and enter the password. Default is "password". '
            "Scroll down completely."
        )
    )
    parts.append(f"<p>{i5}</p>\n")
    parts.append(p("Click Update Firmware."))
    parts.append(
        p(
            "Click search and select the matching TeenAstroSHC_*.bin file from the "
            "downloaded firmware folder. Click Update. Wait until programming is done. "
            "After the HandController will reboot. Finish!"
        )
    )
    parts.append("<hr/>\n")
    parts.append(
        p(
            "Field manuals (PDF): "
            '<a href="https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/'
            'Manuel_utilisateur_en.pdf" rel="nofollow noopener" target="_blank">English</a>, '
            '<a href="https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/'
            'Manuel_utilisateur_fr.pdf" rel="nofollow noopener" target="_blank">Français</a>, '
            '<a href="https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/'
            'Manuel_utilisateur_de.pdf" rel="nofollow noopener" target="_blank">Deutsch</a>.'
        )
    )
    return "".join(parts)


def build_config(old: str) -> str:
    imgs = extract_imgs(old)
    if len(imgs) != 1:
        raise SystemExit(f"expected 1 image on TeenAstroConfig page, found {len(imgs)}")
    img = imgs[0]
    parts: list[str] = []
    parts.append(h("TeenAstroConfig"))
    parts.append(
        p(
            "Jürgen Goldan has developed a tool to backup your mount parameters: "
            '<a href="http://www.goldan.org/juergen/downloads/TeenAstroConfig.exe" '
            'rel="nofollow noopener" target="_blank">TeenAstroConfig.exe</a>'
        )
    )
    parts.append(
        p(
            "The tool is useful when you keep several setups, and before a factory reset: "
            "it stores the mount parameters and writes them back."
        )
    )
    parts.append(p("What the buttons do:"))
    parts.append("<ul>\n")
    parts.append(
        "<li>Connect/Close — connect via USB to board</li>\n"
        "<li>Once connected, the window shows the TeenAstro version and date.</li>\n"
        "<li>Get Data/Write Data — get and write the data via USB to/from the dialog</li>\n"
        "<li>Save/Load — write all settings to a file *.tac / *.json, or read them back</li>\n"
    )
    parts.append("</ul>\n")
    parts.append(f"<p>{img}</p>\n")
    parts.append(h("TeenAstro Uploader (firmware 1.5+)"))
    parts.append(
        p(
            "From TeenAstro <strong>1.5</strong> onward, the Windows "
            '<a href="https://groups.io/g/TeenAstro/wiki/8837" rel="nofollow noopener" '
            'target="_blank">TeenAstroUploader</a> includes the same capability without '
            "a separate tool:"
        )
    )
    parts.append("<ul>\n")
    parts.append(
        "<li><strong>EEPROM</strong> on the Telescope / Focuser tabs — full parameter "
        "editor (Webserver grouping; two mounts on the MainUnit). Write verifies by "
        "re-reading.</li>\n"
        "<li><strong>Auto!</strong> — backup → flash → restore → verify during a "
        "firmware upgrade</li>\n"
    )
    parts.append("</ul>\n")
    parts.append(
        p(
            "TeenAstroConfig remains fine for older habits and for setups that already "
            "use its files."
        )
    )
    return "".join(parts)


def update_page(page_id: int, builder) -> None:
    meta = api("GET", f"getwikipageset?group_name={GROUP}&id={page_id}")
    old = meta["data"]["body"] or ""
    title = meta["page"]["title"]
    new = builder(old)
    before = img_srcs(old)
    after = img_srcs(new)
    if before != after:
        raise SystemExit(f"{page_id}: image sources changed before upload")
    if new == old:
        print(f"skip {page_id} unchanged")
        return
    # Bearer API key: csrf not required, but harmless if present
    user = api("GET", "getuser")
    form = {
        "group_name": GROUP,
        "id": str(page_id),
        "title": title,
        "body": new,
        "edit_msg": "Document Auto! / EEPROM (1.5+); keep all pictures.",
    }
    csrf = user.get("csrf_token")
    if csrf:
        form["csrf"] = csrf
    result = api("POST", "updatewikipage", form)
    if result.get("object") == "error":
        raise SystemExit(
            f"{page_id} update failed: {result.get('type')} {result.get('extra')}"
        )
    got = api("GET", f"getwikipageset?group_name={GROUP}&id={page_id}")
    got_srcs = img_srcs(got["data"]["body"] or "")
    if got_srcs != before:
        raise SystemExit(f"{page_id}: IMAGES CHANGED AFTER UPLOAD")
    print(
        f"ok {page_id} {title} imgs={len(got_srcs)} "
        f"bytes={len(got['data']['body'] or '')}"
    )


def main() -> None:
    update_page(8837, build_uploader)
    update_page(10404, build_config)
    print("done")


if __name__ == "__main__":
    main()
