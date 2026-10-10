#!/usr/bin/env python3
"""Publish Webserver + WiFi Interface wiki pages with fresh screenshots."""
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
SHOTS = Path(__file__).resolve().parent / "screenshots"

PAGE_WEBSERVER = 8105
PAGE_WIFI = 14592

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


def p(html: str) -> str:
    return f"<p>{html}</p>\n"


def h(text: str) -> str:
    return f"<p><strong>{text}</strong></p>\n"


def wiki_a(page_id: int | str, text: str) -> str:
    return f'<a href="https://groups.io/g/TeenAstro/wiki/{page_id}" rel="nofollow">{text}</a>'


def a(href: str, text: str) -> str:
    return f'<a href="{href}" rel="nofollow noopener" target="_blank">{text}</a>'


def update_page(page_id: int, title: str, body: str, edit_msg: str) -> None:
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


def build_webserver() -> str:
    parts: list[str] = []
    parts.append(h("Web server"))
    parts.append(
        p(
            "The web server is where you set the site, the mount, the motors, the limits "
            "and the Wi‑Fi. Connect a computer or a phone to the TeenAstro network, then "
            "open a browser at <strong>192.168.0.1</strong> (the default Access Point address)."
        )
    )
    parts.append(
        p(
            "If that address does not answer, read the one in use on the hand controller: "
            "<strong>Telescope Settings → Wifi → Show IP</strong>. The two Wi‑Fi modes "
            f"(Access Point and Station) are described on the {wiki_a(PAGE_WIFI, 'WiFi Interface')} page."
        )
    )
    parts.append(
        p(
            "Screenshots below are from firmware <strong>1.6</strong> (dark theme). "
            "The navigation bar lists Status, Control, Speed, Tracking, Site, Mount, Motors, "
            "Limits and WiFi. Encoders and Focuser appear when those options are enabled."
        )
    )

    parts.append(h("Status"))
    parts.append(p("Live mount state: time, coordinates, pier side, alignment, tracking and parking."))
    parts.append(img_tag("web_status.png", "Webserver Status"))

    parts.append(h("Control"))
    parts.append(p("Manual moves and common night actions from a browser."))
    parts.append(img_tag("web_control.png", "Webserver Control"))

    parts.append(h("Site"))
    parts.append(p("Latitude, longitude, elevation and time / time zone."))
    parts.append(img_tag("web_site.png", "Webserver Site"))

    parts.append(h("Mount"))
    parts.append(
        p(
            "Mount type, motors and encoders. <strong>Mount error</strong> sits above "
            "Refraction: <strong>Off</strong> / <strong>On</strong> (“Hold CH and NP”), "
            "then CH and NP in degrees (±5, Wallace’s signs). Same meaning as on the hand controller."
        )
    )
    parts.append(img_tag("web_mount.png", "Webserver Mount — Mount error"))

    parts.append(h("Motors"))
    parts.append(p("Gear ratios, steps, microsteps, currents and backlash for each axis."))
    parts.append(img_tag("web_motors.png", "Webserver Motors"))

    parts.append(h("Speed"))
    parts.append(p("Guiding and slew rates."))
    parts.append(img_tag("web_speed.png", "Webserver Speed"))

    parts.append(h("Tracking"))
    parts.append(p("Tracking rates and corrections."))
    parts.append(img_tag("web_tracking.png", "Webserver Tracking"))

    parts.append(h("Limits"))
    parts.append(
        p(
            "Horizon, under-pole, meridian and axis limits. Each limit has a "
            "<strong>Default</strong> button that restores the factory value for that limit only."
        )
    )
    parts.append(img_tag("web_limits.png", "Webserver Limits"))

    parts.append(h("WiFi"))
    parts.append(
        p(
            "Enter the password (<strong>password</strong> by default) to change networks, "
            "Access Point settings and firmware update. Step-by-step Station mode setup is on "
            f"the {wiki_a(PAGE_WIFI, 'WiFi Interface')} page."
        )
    )
    parts.append(img_tag("web_wifi_login.png", "Webserver WiFi login"))
    parts.append(img_tag("web_wifi_config.png", "Webserver WiFi configuration"))

    parts.append(h("Related"))
    parts.append(
        p(
            f"{wiki_a(PAGE_WIFI, 'WiFi Interface')} · "
            f"Field manuals: {a(MANUALS['en'], 'EN')} · {a(MANUALS['fr'], 'FR')} · {a(MANUALS['de'], 'DE')}"
        )
    )
    return "".join(parts)


def build_wifi() -> str:
    parts: list[str] = []
    parts.append(h("WiFi Interface"))
    parts.append(
        p(
            "The Wi‑Fi offers two operation modes: <strong>Access Point</strong> and "
            "<strong>Station</strong>. You switch between them on the hand controller "
            "(<strong>Telescope Settings → Wifi → Select Mode</strong>). Network names and "
            "passwords are set in the "
            f"{wiki_a(PAGE_WEBSERVER, 'Webserver')} WiFi tab "
            "(password default <strong>password</strong>)."
        )
    )

    parts.append(h("Access Point"))
    parts.append(
        p(
            "TeenAstro works as a wireless access point. Connect a notebook, phone or tablet "
            "to the TeenAstro WLAN, then use SkySafari, ASCOM, or open the "
            f"{wiki_a(PAGE_WEBSERVER, 'Webserver')} (default <strong>192.168.0.1</strong>)."
        )
    )
    parts.append(
        p("While you are connected to TeenAstro you usually have no Internet on that device.")
    )

    parts.append(h("Station mode"))
    parts.append(
        p(
            "TeenAstro joins your home or observatory router. A notebook stays on the usual "
            "WLAN and can reach both TeenAstro and the Internet. Configure the router SSID "
            "first in the Webserver, then select that profile on the hand controller."
        )
    )

    parts.append(h("Example configuration"))
    parts.append(
        "<ol>\n"
        "<li>Connect to the TeenAstro WLAN with a notebook (the password is <strong>password</strong>).</li>\n"
        f"<li>Open the {wiki_a(PAGE_WEBSERVER, 'Webserver')} and go to the <strong>WiFi</strong> tab. "
        "Enter <strong>password</strong> to unlock the configuration.</li>\n"
        "<li>Configure <strong>Station mode 0</strong>:<br/>"
        "SSID — name of your router network.<br/>"
        "Password — password of your router network.<br/>"
        "Leave <strong>Enable DHCP</strong> checked unless you need a fixed address.</li>\n"
        "</ol>\n"
    )
    parts.append(img_tag("web_wifi_station0.png", "Station mode 0 on the Webserver"))
    parts.append(
        "<ol start='4'>\n"
        "<li>Click <strong>Upload</strong>.</li>\n"
        "<li>Leave the Webserver and disconnect from the TeenAstro WLAN.</li>\n"
        "<li>Power TeenAstro off and on.</li>\n"
        "<li>On the hand controller: <strong>Telescope Settings</strong> (Shift + West).</li>\n"
        "<li><strong>Wifi → Select Mode</strong>. The configured SSID should appear.</li>\n"
        "<li>Select that SSID and confirm. The hand controller reboots.</li>\n"
        "<li>When the Wi‑Fi icon shows a solid link, TeenAstro is connected to the router. "
        "If not, the signal is too weak or out of range.</li>\n"
        "<li>Read the address: <strong>Telescope Settings → Wifi → Show IP</strong>.</li>\n"
        "<li>Connect your notebook to the router as usual and open the Webserver at that IP.</li>\n"
        "</ol>\n"
    )

    parts.append(h("Related"))
    parts.append(p(wiki_a(PAGE_WEBSERVER, "Webserver")))
    return "".join(parts)


def main() -> None:
    update_page(
        PAGE_WEBSERVER,
        "Webserver",
        build_webserver(),
        "1.6 screenshots: Mount error, Limits Default, dark theme",
    )
    update_page(
        PAGE_WIFI,
        "WiFi Interface",
        build_wifi(),
        "Refresh Station-mode guide with 1.6 Webserver screenshots",
    )
    print(f"https://groups.io/g/TeenAstro/wiki/{PAGE_WEBSERVER}")
    print(f"https://groups.io/g/TeenAstro/wiki/{PAGE_WIFI}")


if __name__ == "__main__":
    main()
