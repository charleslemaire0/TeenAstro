#!/usr/bin/env python3
"""Publish the Android app wiki as eight pages, with screenshots."""
from __future__ import annotations

import base64
import html
import json
import os
import re
import time
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
PAGE_SIDEBAR = 7362
HUB_ID = 43283

# filename, wiki title, link token used in the drafts
PAGES = [
    ("Android_App.md", "Android App", "APP_HUB"),
    ("Android_App_Status.md", "Android App Status", "APP_STATUS"),
    ("Android_App_Control.md", "Android App Control", "APP_CONTROL"),
    ("Android_App_Goto.md", "Android App Goto", "APP_GOTO"),
    ("Android_App_Planetarium.md", "Android App Planetarium", "APP_PLANETARIUM"),
    ("Android_App_Tracking.md", "Android App Tracking", "APP_TRACKING"),
    ("Android_App_Alignment.md", "Android App Alignment", "APP_ALIGN"),
    ("Android_App_Phone_finder.md", "Android App Phone finder", "APP_FINDER"),
]


def api(method: str, path: str, form: dict | None = None) -> dict:
    url = f"{API}/{path}"
    data = None
    headers = {"Authorization": f"Bearer {TOKEN}", "User-Agent": "TeenAstroWikiPublish/2.0"}
    if form is not None:
        data = urllib.parse.urlencode(form).encode("utf-8")
        headers["Content-Type"] = "application/x-www-form-urlencoded"
    for attempt in range(6):
        req = urllib.request.Request(url, data=data, headers=headers, method=method)
        try:
            with urllib.request.urlopen(req, timeout=180) as resp:
                raw = resp.read()
                if not raw:
                    return {}
                return json.loads(raw.decode("utf-8"))
        except urllib.error.HTTPError as exc:
            body = exc.read().decode("utf-8", errors="replace")
            if exc.code == 429 and attempt < 5:
                wait = 20 * (attempt + 1)
                print(f"429 on {path}, waiting {wait}s")
                time.sleep(wait)
                continue
            raise SystemExit(f"HTTP {exc.code} {path}: {body[:600]}") from exc
    raise SystemExit(f"gave up on {path}")


def data_uri(name: str) -> str:
    path = SHOTS / name
    if not path.is_file():
        raise SystemExit(f"missing screenshot {path}")
    b64 = base64.b64encode(path.read_bytes()).decode("ascii")
    return f"data:image/png;base64,{b64}"


def img_tag(name: str, alt: str) -> str:
    return (
        f'<p><img src="{data_uri(name)}" alt="{html.escape(alt)}" '
        f'loading="lazy" class="myimg-responsive myimg-responsive"/></p>\n'
    )


def inline(text: str, ids: dict[str, int]) -> str:
    text = text.replace("https://groups.io/g/TeenAstro/wiki/43283", f"https://groups.io/g/TeenAstro/wiki/{ids['APP_HUB']}")
    for token, pid in ids.items():
        text = text.replace(f"https://groups.io/g/TeenAstro/wiki/{token}", f"https://groups.io/g/TeenAstro/wiki/{pid}")
    parts: list[str] = []
    pattern = re.compile(r"\*\*(.+?)\*\*|\[([^\]]+)\]\((https://[^)]+)\)")
    pos = 0
    for match in pattern.finditer(text):
        parts.append(html.escape(text[pos:match.start()]))
        if match.group(1) is not None:
            parts.append(f"<strong>{html.escape(match.group(1))}</strong>")
        else:
            href = html.escape(match.group(3), quote=True)
            parts.append(
                f'<a href="{href}" rel="nofollow noopener" target="_blank">'
                f"{html.escape(match.group(2))}</a>"
            )
        pos = match.end()
    parts.append(html.escape(text[pos:]))
    return "".join(parts)


def markdown_to_wiki(md: str, ids: dict[str, int]) -> str:
    lines = md.splitlines()
    body: list[str] = []
    started = False
    for line in lines:
        if not started:
            if line.startswith("# ") or line.startswith("> ") or line.strip() == "":
                continue
            started = True
        body.append(line)

    chunks: list[str] = []
    paragraph: list[str] = []
    items: list[str] = []

    def flush_paragraph() -> None:
        if not paragraph:
            return
        text = " ".join(s.strip() for s in paragraph)
        chunks.append(f"<p>{inline(text, ids)}</p>\n")
        paragraph.clear()

    def flush_items() -> None:
        if not items:
            return
        lis = "".join(f"<li>{inline(item, ids)}</li>" for item in items)
        chunks.append(f"<ul>{lis}</ul>\n")
        items.clear()

    for line in body:
        image = re.fullmatch(r"!\[([^\]]*)\]\(screenshots/([^)]+)\)", line.strip())
        if line.startswith("## "):
            flush_paragraph()
            flush_items()
            chunks.append(f"<p><strong>{inline(line[3:].strip(), ids)}</strong></p>\n")
        elif image:
            flush_paragraph()
            flush_items()
            chunks.append(img_tag(image.group(2), image.group(1)))
        elif line.startswith("- "):
            flush_paragraph()
            items.append(line[2:].strip())
        elif line.strip() == "":
            flush_paragraph()
            flush_items()
        else:
            flush_items()
            paragraph.append(line)
    flush_paragraph()
    flush_items()
    return "".join(chunks)


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
    print(f"updated {page_id} [{title}] bytes={len(body)}")
    time.sleep(12)


def find_page(title: str) -> int | None:
    token = None
    while True:
        path = f"getwikipages?group_name={GROUP}&limit=100"
        if token:
            path += f"&page_token={urllib.parse.quote(str(token))}"
        data = api("GET", path)
        for page in data.get("data") or []:
            if (page.get("title") or "") == title:
                return int(page["id"])
        if not data.get("has_more"):
            return None
        token = data.get("next_page_token")
        if not token:
            return None


def create_or_find(title: str) -> int:
    if title == "Android App":
        return HUB_ID
    found = find_page(title)
    if found is not None:
        print(f"found {found} [{title}]")
        return found
    result = api(
        "POST",
        "newwikipage",
        {"group_name": GROUP, "title": title, "body": "<p>Draft — refreshing…</p>"},
    )
    if result.get("object") == "error":
        raise SystemExit(f"newwikipage {title}: {result}")
    pid = int(result["page"]["id"])
    print(f"created {pid} [{title}]")
    return pid


def patch_sidebar(ids: dict[str, int]) -> None:
    got = api("GET", f"getwikipageset?group_name={GROUP}&id={PAGE_SIDEBAR}")
    body = got["data"]["body"] or ""
    children = [
        ("APP_STATUS", "Status"),
        ("APP_CONTROL", "Control"),
        ("APP_GOTO", "Goto"),
        ("APP_PLANETARIUM", "Planetarium"),
        ("APP_TRACKING", "Tracking"),
        ("APP_ALIGN", "Alignment"),
        ("APP_FINDER", "Phone finder"),
    ]
    sub = "<ul>\n" + "".join(
        f'<li><a href="https://groups.io/g/TeenAstro/wiki/{ids[token]}" rel="nofollow">{label}</a></li>\n'
        for token, label in children
    ) + "</ul>"
    block = (
        f'<li><a href="https://groups.io/g/TeenAstro/wiki/{ids["APP_HUB"]}" rel="nofollow">Android App</a>\n'
        f"{sub}</li>"
    )
    # Replace a previous Android App item, with or without a nested list.
    pattern = re.compile(
        r'<li><a href="https://groups\.io/g/TeenAstro/wiki/\d+" rel="nofollow">Android App</a>'
        r"(?:\s*<ul>.*?</ul>)?\s*</li>",
        re.S,
    )
    if not pattern.search(body):
        raise SystemExit("_Sidebar: Android App entry not found")
    body = pattern.sub(block, body, count=1)
    update_page(PAGE_SIDEBAR, "_Sidebar", body, "Add Android app pages to Tools")


def write_back(ids: dict[str, int]) -> None:
    for filename, _title, token in PAGES:
        path = ROOT / filename
        text = path.read_text(encoding="utf-8")
        url = f"https://groups.io/g/TeenAstro/wiki/{ids[token]}"
        lines = text.splitlines()
        for i, line in enumerate(lines):
            if line.startswith("> Groups.io:"):
                lines[i] = f"> Groups.io: {url}"
                break
        # Keep APP_* tokens in the body so the next publish can substitute ids.
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    ids: dict[str, int] = {}
    for _filename, title, token in PAGES:
        ids[token] = create_or_find(title)
    for filename, title, token in PAGES:
        md = (ROOT / filename).read_text(encoding="utf-8")
        update_page(ids[token], title, markdown_to_wiki(md, ids), "Android app pages with screenshots")
    patch_sidebar(ids)
    write_back(ids)
    for token, pid in ids.items():
        print(f"{token} https://groups.io/g/TeenAstro/wiki/{pid}")


if __name__ == "__main__":
    main()
