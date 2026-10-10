#!/usr/bin/env python3
"""Capture TeenAstro webserver screenshots for the groups.io wiki."""
from __future__ import annotations

from pathlib import Path

from PIL import Image
from playwright.sync_api import sync_playwright

BASE = "http://192.168.1.18"
OUT = Path(__file__).resolve().parent / "screenshots"
OUT.mkdir(exist_ok=True)
WIFI_PASSWORD = "password"
MAX_WIDTH = 720
PAD = 14

PAGES = [
    ("index.htm", "web_status.png"),
    ("control.htm", "web_control.png"),
    ("configuration_site.htm", "web_site.png"),
    ("configuration_mount.htm", "web_mount.png"),
    ("configuration_motors.htm", "web_motors.png"),
    ("configuration_speed.htm", "web_speed.png"),
    ("configuration_tracking.htm", "web_tracking.png"),
    ("configuration_limits.htm", "web_limits.png"),
]

# Measure only real UI (header, nav, controls, labels) — not empty dark cards.
CLIP_JS = """() => {
  const sel = [
    '.hdr', '.hdr *', 'nav', 'nav a', 'nav label',
    '.content input', '.content button', '.content select',
    '.content .bt', '.content b', '.content table',
    '.content td', '.content th', '.content span',
    '.content a', '.content label', '.gb', '.bb', '.bbh', '.panel'
  ].join(',');
  const els = [...document.querySelectorAll(sel)];
  let minX = Infinity, minY = Infinity, maxX = 0, maxY = 0;
  let found = false;
  for (const el of els) {
    const r = el.getBoundingClientRect();
    if (r.width < 1 || r.height < 1) continue;
    const tag = el.tagName;
    const text = (el.innerText || el.value || '').trim();
    if (!['INPUT', 'BUTTON', 'SELECT', 'IMG', 'TABLE'].includes(tag) && !text)
      continue;
    found = true;
    minX = Math.min(minX, r.left + window.scrollX);
    minY = Math.min(minY, r.top + window.scrollY);
    maxX = Math.max(maxX, r.right + window.scrollX);
    maxY = Math.max(maxY, r.bottom + window.scrollY);
  }
  if (!found) {
    const b = document.body.getBoundingClientRect();
    return { x: 0, y: 0, width: Math.ceil(b.width), height: Math.ceil(b.height) };
  }
  const pad = %d;
  const x = Math.max(0, Math.floor(minX - pad));
  const y = Math.max(0, Math.floor(minY - pad));
  return {
    x, y,
    width: Math.ceil(maxX - minX + 2 * pad),
    height: Math.ceil(maxY - minY + 2 * pad)
  };
}""" % PAD


def shrink(path: Path, max_width: int = MAX_WIDTH) -> None:
    img = Image.open(path).convert("RGB")
    if img.width > max_width:
        ratio = max_width / img.width
        img = img.resize((max_width, int(img.height * ratio)), Image.Resampling.LANCZOS)
    img = img.convert("P", palette=Image.Palette.ADAPTIVE, colors=160)
    img.save(path, optimize=True)
    print(f"wrote {path.name} {img.size[0]}x{img.size[1]} ({path.stat().st_size} bytes)")


def shot_page(page, out: Path) -> None:
    page.evaluate("window.scrollTo(0, 0)")
    page.wait_for_timeout(300)
    # Expand viewport so long pages lay out fully before measuring.
    rough = page.evaluate(
        """() => Math.max(
          document.body.scrollHeight, document.documentElement.scrollHeight,
          ...[...document.querySelectorAll('body *')].map(el => {
            const r = el.getBoundingClientRect();
            return r.bottom + window.scrollY;
          })
        )"""
    )
    width = page.viewport_size["width"]
    page.set_viewport_size({"width": width, "height": max(900, min(int(rough) + 40, 5000))})
    page.wait_for_timeout(150)
    page.evaluate("window.scrollTo(0, 0)")
    clip = page.evaluate(CLIP_JS)
    # Keep clip inside the current viewport bitmap.
    vh = page.viewport_size["height"]
    vw = page.viewport_size["width"]
    clip["x"] = max(0, min(clip["x"], vw - 1))
    clip["y"] = max(0, min(clip["y"], vh - 1))
    clip["width"] = max(1, min(clip["width"], vw - clip["x"]))
    clip["height"] = max(1, min(clip["height"], vh - clip["y"]))
    page.screenshot(path=str(out), clip=clip)
    shrink(out)
    page.set_viewport_size({"width": width, "height": 900})


def main() -> None:
    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        page = browser.new_page(viewport={"width": 720, "height": 900})

        for path, name in PAGES:
            page.goto(f"{BASE}/{path}", wait_until="networkidle", timeout=30000)
            shot_page(page, OUT / name)

        page.goto(f"{BASE}/wifi.htm", wait_until="networkidle", timeout=30000)
        page.wait_for_timeout(300)
        if page.locator("button[name='logout']").count():
            page.locator("button[name='logout']").click()
            page.wait_for_load_state("networkidle")
            page.wait_for_timeout(400)
        shot_page(page, OUT / "web_wifi_login.png")

        pwd = page.locator("input[name='login']")
        if pwd.count():
            pwd.fill(WIFI_PASSWORD)
            page.locator("form").filter(has=page.locator("input[name='login']")).locator(
                "button[type='submit']"
            ).click()
            page.wait_for_load_state("networkidle")
            page.wait_for_timeout(500)
        shot_page(page, OUT / "web_wifi_config.png")

        page.evaluate("window.scrollTo(0, 0)")
        page.wait_for_timeout(200)
        station = page.locator("b").filter(has_text="Station mode 0").first
        if not station.count():
            station = page.locator("text=Station mode 0").first
        if station.count():
            # Bounding box of the Station mode 0 heading through its Upload form.
            clip = page.evaluate(
                """() => {
                  const heads = [...document.querySelectorAll('b')];
                  const head = heads.find(b => /Station mode 0/i.test(b.textContent || ''));
                  if (!head) return null;
                  let form = head.parentElement;
                  while (form && form.tagName !== 'FORM') {
                    form = form.nextElementSibling;
                  }
                  if (!form) form = head.closest('div')?.querySelector('form');
                  const r0 = head.getBoundingClientRect();
                  const r1 = (form || head).getBoundingClientRect();
                  const content = document.querySelector('.content')?.getBoundingClientRect();
                  const left = content ? content.left : Math.min(r0.left, r1.left);
                  const right = content ? content.right : Math.max(r0.right, r1.right);
                  const top = Math.min(r0.top, r1.top) - 8;
                  const bottom = Math.max(r0.bottom, r1.bottom) + 12;
                  return {
                    x: Math.max(0, Math.floor(left)),
                    y: Math.max(0, Math.floor(top)),
                    width: Math.ceil(right - left),
                    height: Math.ceil(bottom - top)
                  };
                }"""
            )
            if clip:
                out = OUT / "web_wifi_station0.png"
                page.screenshot(path=str(out), clip=clip)
                shrink(out)

        browser.close()


if __name__ == "__main__":
    main()
