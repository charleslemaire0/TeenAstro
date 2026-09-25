"""Build printable A4 PDFs of the three user manuals.

Requires Google Chrome or Microsoft Edge. Images stay beside the Markdown
files; this script only writes the PDFs.
"""
import http.server
import os
import re
import socket
import subprocess
import tempfile
import threading
from pathlib import Path

import markdown

ROOT = Path(__file__).resolve().parent
MANUALS = [
    ("Manuel_utilisateur_fr.md", "Manuel_utilisateur_fr.pdf", "fr", "Manuel d'utilisation"),
    ("Manuel_utilisateur_en.md", "Manuel_utilisateur_en.pdf", "en", "User manual"),
    ("Manuel_utilisateur_de.md", "Manuel_utilisateur_de.pdf", "de", "Benutzerhandbuch"),
]

CSS = """
@page {
  size: A4;
  margin: 16mm 15mm 16mm 15mm;
  @top-left {
    content: "TeenAstro";
    font-family: "Segoe UI", Calibri, sans-serif;
    font-size: 8pt;
    color: #555;
  }
  @top-right {
    content: "%%RUNNING%%";
    font-family: "Segoe UI", Calibri, sans-serif;
    font-size: 8pt;
    color: #555;
  }
  @bottom-right {
    content: counter(page);
    font-family: "Segoe UI", Calibri, sans-serif;
    font-size: 9pt;
    color: #333;
  }
}
html { font-family: "Segoe UI", Calibri, sans-serif; }
body { font-size: 10.5pt; line-height: 1.32; color: #1a1a1a; hyphens: auto; }
h1 { font-size: 20pt; line-height: 1.15; margin: 0 0 0.35em; }
h2 {
  font-size: 13.5pt;
  line-height: 1.2;
  margin: 1.05em 0 0.3em;
  padding-top: 0.28em;
  border-top: 0.8pt solid #1a1a1a;
  break-after: avoid;
}
h3 { font-size: 11.5pt; margin: 0.75em 0 0.2em; break-after: avoid; }
p { margin: 0.25em 0 0.45em; }
ul, ol { margin: 0.2em 0 0.45em; padding-left: 1.15em; }
li { margin: 0.08em 0; }
a { color: #1a1a1a; text-decoration: none; }
code { font-family: Consolas, "Cascadia Mono", monospace; font-size: 0.92em; overflow-wrap: anywhere; }
table { border-collapse: collapse; width: 100%; margin: 0.3em 0 0.55em; font-size: 9.5pt; }
th, td { border: 1px solid #c8c8c8; padding: 1px 5px; vertical-align: middle; text-align: left; }
th { background: #f3f3f3; }
tr { break-inside: avoid; }
img { max-width: 100%; height: auto; }
td img, th img { width: auto; height: 5.2mm; border: none; margin: 0; display: inline-block; }
li img { width: 48mm; max-width: 48mm; height: auto; display: block; border: 0.4pt solid #b5b5b5; }
hr { border: none; border-top: 0.4pt solid #ccc; margin: 0.55em 0; }
.gallery, .shots, .shot-single {
  margin: 0 0 2.5mm;
  break-inside: avoid;
  page-break-inside: avoid;
}
figure { margin: 0; }
.gallery figure, .shots figure {
  display: inline-block;
  vertical-align: top;
  margin: 0 0 3mm 0;
}
.gallery figure { width: 31.5%; margin-right: 2.7%; }
.gallery figure:nth-child(3n) { margin-right: 0; }
.shots figure { width: 48%; margin-right: 4%; }
.shots figure:nth-child(2n) { margin-right: 0; }
.shot-single figure { width: 62mm; }
.gallery img, .shots img, .shot-single img {
  display: block;
  width: 100%;
  border: 0.4pt solid #b5b5b5;
}
.shots img { width: 68mm; max-width: 100%; }
.cap {
  display: block;
  font-weight: 600;
  font-size: 8.5pt;
  line-height: 1.25;
  text-align: left;
  margin-top: 0.8mm;
}
.txt {
  display: block;
  font-weight: 400;
  font-size: 8.5pt;
  line-height: 1.28;
  text-align: left;
  hyphens: auto;
  margin-top: 0.3mm;
}
"""

BLOCK = re.compile(
    r"<hr\s*/>|<(p|h[1-6]|ul|ol|table|blockquote)\b[^>]*>.*?</\1>",
    re.DOTALL,
)
SCREEN = re.compile(
    r'\s*<p><img alt="([^"]*)" src="(images/screens/[^"]+)" /></p>\s*',
    re.DOTALL,
)
PARA = re.compile(r"\s*<p>(.*)</p>\s*", re.DOTALL)


def chrome():
    candidates = [
        Path(r"C:\Program Files\Google\Chrome\Application\chrome.exe"),
        Path(r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"),
        Path(r"C:\Program Files\Microsoft\Edge\Application\msedge.exe"),
    ]
    for path in candidates:
        if path.exists():
            return path
    raise SystemExit("Chrome or Edge is required to print the manuals")


def free_port():
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def figure(alt, src, caption):
    parts = ['<figure><img alt="%s" src="%s">' % (alt, src)]
    if caption and caption != alt:
        parts.append('<figcaption><span class="cap">%s</span>' % alt)
        parts.append('<span class="txt">%s</span></figcaption>' % caption)
    elif alt:
        parts.append('<figcaption><span class="cap">%s</span></figcaption>' % alt)
    parts.append("</figure>")
    return "".join(parts)


def screen_of(block):
    match = SCREEN.fullmatch(block)
    if not match:
        return None
    return match.group(1), match.group(2)


def paragraph_of(block):
    if screen_of(block):
        return None
    match = PARA.fullmatch(block)
    if not match:
        return None
    return match.group(1)


def rows(shots, per_row, kind):
    """One block per row, so a page break falls between rows and not inside a caption."""
    blocks = []
    for start in range(0, len(shots), per_row):
        row = shots[start:start + per_row]
        blocks.append('<div class="%s">%s</div>' % (kind, "".join(row)))
    return "".join(blocks)


def layout_screens(body):
    """Place OLED shots in rows so a page holds several, with the caption beside."""
    blocks = [match.group(0) for match in BLOCK.finditer(body)]
    out = []
    index = 0
    while index < len(blocks):
        if not screen_of(blocks[index]):
            out.append(blocks[index])
            index += 1
            continue
        if index + 1 < len(blocks) and screen_of(blocks[index + 1]):
            shots = []
            while index < len(blocks) and screen_of(blocks[index]):
                alt, src = screen_of(blocks[index])
                shots.append(figure(alt, src, alt))
                index += 1
            out.append(rows(shots, 3, "gallery"))
            continue
        shots = []
        while index < len(blocks) and screen_of(blocks[index]):
            alt, src = screen_of(blocks[index])
            caption = alt
            index += 1
            text = paragraph_of(blocks[index]) if index < len(blocks) else None
            if text is not None:
                caption = text
                index += 1
            shots.append(figure(alt, src, caption))
        kind = "shots" if len(shots) > 1 else "shot-single"
        out.append(rows(shots, 2, kind))
    return "".join(out)


def to_html(md_name, lang, running):
    text = (ROOT / md_name).read_text(encoding="utf-8")
    body = markdown.markdown(
        text,
        extensions=["tables", "sane_lists"],
    )
    body = layout_screens(body)
    html = (
        "<!DOCTYPE html><html lang=\"%s\"><head><meta charset=\"utf-8\">"
        "<style>%s</style></head><body>%s</body></html>"
    ) % (lang, CSS.replace("%%RUNNING%%", running), body)
    out = ROOT / ("_print_" + lang + ".html")
    out.write_text(html, encoding="utf-8")
    return out.name


def main():
    os.chdir(ROOT)
    port = free_port()
    handler = http.server.SimpleHTTPRequestHandler
    server = http.server.ThreadingHTTPServer(("127.0.0.1", port), handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    browser = chrome()
    try:
        for md_name, pdf_name, lang, running in MANUALS:
            page = to_html(md_name, lang, running)
            pdf = ROOT / pdf_name
            url = "http://127.0.0.1:%d/%s" % (port, page)
            with tempfile.TemporaryDirectory() as profile:
                subprocess.run(
                    [
                        str(browser),
                        "--headless",
                        "--disable-gpu",
                        "--no-first-run",
                        "--user-data-dir=" + profile,
                        "--no-pdf-header-footer",
                        "--print-to-pdf=" + str(pdf),
                        url,
                    ],
                    cwd=str(ROOT),
                    check=True,
                )
            print(pdf_name, pdf.stat().st_size)
    finally:
        server.shutdown()
        for lang in ("fr", "en", "de"):
            tmp = ROOT / ("_print_" + lang + ".html")
            if tmp.exists():
                tmp.unlink()


if __name__ == "__main__":
    main()
