"""Probe SHC configuration_tracking.htm reload / busyGuard behavior on live device."""
import re
import time
import urllib.error
import urllib.request

BASE = "http://192.168.1.15"
PATH = "/configuration_tracking.htm"


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


opener = urllib.request.build_opener(NoRedirect)


def fetch(url, timeout=10):
    req = urllib.request.Request(
        url, headers={"Connection": "close", "Cache-Control": "no-cache", "Pragma": "no-cache"}
    )
    t0 = time.perf_counter()
    try:
        with opener.open(req, timeout=timeout) as r:
            body = r.read().decode("utf-8", "replace")
            return r.status, dict(r.headers), body, (time.perf_counter() - t0) * 1000
    except urllib.error.HTTPError as e:
        body = e.read().decode("utf-8", "replace")
        return e.code, dict(e.headers), body, (time.perf_counter() - t0) * 1000


def classify(status, headers, body):
    loc = headers.get("Location") or headers.get("location")
    low = body.lower()
    if status in (301, 302, 303, 307, 308):
        return f"REDIRECT {status} -> {loc}"
    if "loading" in low and "refresh" in low:
        return "LOADING_PAGE"
    if "consider refraction" in low or "tracking options" in low:
        return "FULL_PAGE"
    return f"OTHER status={status} len={len(body)} head={body[:100]!r}"


def main():
    print("=== 1) Clean GET (establish cooldown) ===")
    s, h, b, ms = fetch(BASE + PATH)
    print(f"  {classify(s, h, b)}  ({ms:.0f} ms)")

    m = re.search(r"name='trackr'.*?selected value='(\d)'", b, re.S)
    cur = m.group(1) if m else None
    toggle = "2" if cur == "1" else "1"
    print(f"  current trackr selected~={cur}, will submit trackr={toggle}")

    print("\n=== 2) Mutation IMMEDIATELY after load (<800ms cooldown window) ===")
    gap_start = time.perf_counter()
    s2, h2, b2, ms2 = fetch(f"{BASE}{PATH}?trackr={toggle}")
    gap_ms = (time.perf_counter() - gap_start) * 1000
    print(f"  gap_from_prev_request_start~={gap_ms:.0f} ms (includes request 1 duration)")
    print(f"  {classify(s2, h2, b2)}  ({ms2:.0f} ms)")
    print(f"  Cache-Control={h2.get('Cache-Control') or h2.get('cache-control')}")
    if "Loading" in b2 or "loading" in b2.lower():
        print(f"  body snippet: {b2[:240]!r}")

    print("\n=== 3) Wait 1000ms then same mutation again ===")
    time.sleep(1.0)
    s3, h3, b3, ms3 = fetch(f"{BASE}{PATH}?trackr={toggle}")
    print(f"  {classify(s3, h3, b3)}  ({ms3:.0f} ms)")
    print(f"  Location={h3.get('Location') or h3.get('location')}")

    print("\n=== 4) Clean GET after mutation path ===")
    time.sleep(0.05)
    s4, h4, b4, ms4 = fetch(BASE + PATH)
    print(f"  {classify(s4, h4, b4)}  ({ms4:.0f} ms)")

    print("\n=== 5) Rapid double clean GET (cooldown on plain navigation) ===")
    # Wait so we're outside any prior cooldown, then hit twice quickly
    time.sleep(1.0)
    s5, h5, b5, ms5 = fetch(BASE + PATH)
    print(f"  first:  {classify(s5, h5, b5)}  ({ms5:.0f} ms)")
    s6, h6, b6, ms6 = fetch(BASE + PATH)
    print(f"  second: {classify(s6, h6, b6)}  ({ms6:.0f} ms)")
    if "Loading" in b6 or "loading" in b6.lower():
        print(f"  second body snippet: {b6[:240]!r}")

    print("\n=== 6) AJAX track.txt (buttons) — expect empty 200, no page reload ===")
    s7, h7, b7, ms7 = fetch(BASE + "/track.txt?dt=Ts")
    print(f"  status={s7} len={len(b7)} body={b7!r}  ({ms7:.0f} ms)")

    print("\n=== 7) Timed mutation: wait only 200ms after clean GET completes ===")
    time.sleep(1.0)
    s8, h8, b8, ms8 = fetch(BASE + PATH)
    print(f"  clean: {classify(s8, h8, b8)}  ({ms8:.0f} ms)")
    time.sleep(0.2)
    # Use opposite toggle from current page if we can parse it
    m2 = re.search(r"name='trackr'.*?selected value='(\d)'", b8, re.S)
    cur2 = m2.group(1) if m2 else toggle
    toggle2 = "2" if cur2 == "1" else "1"
    s9, h9, b9, ms9 = fetch(f"{BASE}{PATH}?trackr={toggle2}")
    print(f"  mutate after 200ms: {classify(s9, h9, b9)}  ({ms9:.0f} ms)")
    if "Loading" in b9 or "loading" in b9.lower():
        print(f"  body snippet: {b9[:240]!r}")


if __name__ == "__main__":
    main()
