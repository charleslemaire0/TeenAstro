"""Time all TeenAstroWifi SHC pages on the live device."""
import time
import urllib.error
import urllib.request

BASE = "http://192.168.1.15"
PAGES = [
    ("/", "index"),
    ("/index.htm", "index.htm"),
    ("/control.htm", "control"),
    ("/configuration_speed.htm", "speed"),
    ("/configuration_tracking.htm", "tracking"),
    ("/configuration_site.htm", "site"),
    ("/configuration_mount.htm", "mount"),
    ("/configuration_motors.htm", "motors"),
    ("/configuration_limits.htm", "limits"),
    ("/configuration_encoders.htm", "encoders"),
    ("/configuration_focuser.htm", "focuser"),
    ("/wifi.htm", "wifi"),
]


def fetch(path, timeout=60):
    t0 = time.perf_counter()
    try:
        with urllib.request.urlopen(BASE + path, timeout=timeout) as r:
            body = r.read()
            ms = (time.perf_counter() - t0) * 1000
            return ms, r.status, body, None
    except Exception as e:
        ms = (time.perf_counter() - t0) * 1000
        return ms, 0, b"", str(e)


def main():
    # ensure up
    for i in range(20):
        ms, st, body, err = fetch("/", timeout=3)
        if st == 200:
            break
        time.sleep(1)
    else:
        raise SystemExit("device not up")

    print(f"{'page':12} {'cold_ms':>8} {'warm_ms':>8} {'bytes':>7} notes")
    print("-" * 60)
    results = []
    for path, name in PAGES:
        time.sleep(0.6)  # clear 400ms cooldown
        cold, st1, b1, e1 = fetch(path)
        note = ""
        if e1:
            note = e1
        elif b"Loading" in b1:
            note = "LOADING"
        time.sleep(0.5)
        warm, st2, b2, e2 = fetch(path)
        if e2:
            note = (note + " " + e2).strip()
        elif b"Loading" in b2:
            note = (note + " warm=LOADING").strip()
        print(f"{name:12} {cold:8.0f} {warm:8.0f} {len(b1) if b1 else 0:7} {note}")
        results.append((name, cold, warm, len(b1) if b1 else 0))

    print()
    ranked = sorted(results, key=lambda x: -x[1])
    print("Slowest cold loads:")
    for name, cold, warm, n in ranked[:6]:
        print(f"  {name:12} {cold:.0f} ms")


if __name__ == "__main__":
    main()
