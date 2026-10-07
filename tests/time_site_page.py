import time
import urllib.error
import urllib.request

BASE = "http://192.168.1.15"


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


opener = urllib.request.build_opener(NoRedirect)


def fetch(url, follow=True):
    t0 = time.perf_counter()
    if follow:
        with urllib.request.urlopen(url, timeout=60) as r:
            body = r.read()
            return (time.perf_counter() - t0) * 1000, r.status, dict(r.headers), body
    try:
        r = opener.open(urllib.request.Request(url, headers={"Connection": "close"}), timeout=60)
        body = r.read()
        return (time.perf_counter() - t0) * 1000, r.status, dict(r.headers), body
    except urllib.error.HTTPError as e:
        body = e.read()
        return (time.perf_counter() - t0) * 1000, e.code, dict(e.headers), body


def main():
    time.sleep(1.2)
    ms, st, h, b = fetch(BASE + "/configuration_site.htm")
    print(f"GET site clean:      {ms:7.0f} ms  status={st} bytes={len(b)}")

    ms2, st2, h2, b2 = fetch(BASE + "/configuration_site.htm")
    loading = b"Loading" in b2
    print(f"GET site immediate:  {ms2:7.0f} ms  status={st2} loading={loading}")

    time.sleep(1.0)
    ms3, st3, h3, b3 = fetch(BASE + "/configuration_site.htm?TimeZ=1", follow=False)
    loc = h3.get("Location") or h3.get("location")
    print(f"MUTATE TimeZ:        {ms3:7.0f} ms  status={st3} loc={loc}")

    ms4, st4, h4, b4 = fetch(BASE + "/configuration_site.htm")
    print(f"GET after redirect:  {ms4:7.0f} ms  status={st4} bytes={len(b4)}")
    print(f"TOTAL mutate+reload: {ms3 + ms4:7.0f} ms")

    time.sleep(1.0)
    ms5, st5, h5, b5 = fetch(BASE + "/configuration_tracking.htm")
    print(f"GET tracking:        {ms5:7.0f} ms  status={st5} bytes={len(b5)}")


if __name__ == "__main__":
    main()
