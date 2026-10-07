"""HTTP OTA upload to ESP8266HTTPUpdateServer /update with slow streaming."""
from __future__ import annotations

import argparse
import mimetypes
import os
import socket
import sys
import time
import uuid


def upload(host: str, bin_path: str, chunk: int = 1024, pause: float = 0.02) -> int:
    size = os.path.getsize(bin_path)
    boundary = f"----TeenAstro{uuid.uuid4().hex}"
    filename = os.path.basename(bin_path)
    preamble = (
        f"--{boundary}\r\n"
        f'Content-Disposition: form-data; name="firmware"; filename="{filename}"\r\n'
        f"Content-Type: application/octet-stream\r\n"
        f"\r\n"
    ).encode()
    epilogue = f"\r\n--{boundary}--\r\n".encode()
    body_len = len(preamble) + size + len(epilogue)

    header = (
        f"POST /update HTTP/1.0\r\n"
        f"Host: {host}\r\n"
        f"Connection: close\r\n"
        f"Content-Type: multipart/form-data; boundary={boundary}\r\n"
        f"Content-Length: {body_len}\r\n"
        f"\r\n"
    ).encode()

    print(f"Uploading {bin_path} ({size} bytes) to http://{host}/update")
    print(f"chunk={chunk} pause={pause}s total_body={body_len}")

    sock = socket.create_connection((host, 80), timeout=30)
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    sock.settimeout(60)
    sock.sendall(header)
    sock.sendall(preamble)

    sent = 0
    t0 = time.time()
    with open(bin_path, "rb") as f:
        while True:
            data = f.read(chunk)
            if not data:
                break
            view = memoryview(data)
            while view:
                try:
                    n = sock.send(view)
                except (BrokenPipeError, ConnectionResetError, OSError) as e:
                    print(f"\nSend failed at {sent}/{size}: {e}")
                    return 2
                view = view[n:]
                sent += n
            if pause:
                time.sleep(pause)
            if sent % (chunk * 32) == 0 or sent == size:
                elapsed = max(time.time() - t0, 0.001)
                print(f"  {sent}/{size} ({100*sent/size:.1f}%) {sent/elapsed/1024:.1f} KiB/s")

    sock.sendall(epilogue)
    print("Waiting for response...")
    try:
        resp = b""
        while True:
            part = sock.recv(4096)
            if not part:
                break
            resp += part
    except socket.timeout:
        print("Response timed out (often OK if ESP rebooted)")
        return 0
    finally:
        sock.close()

    text = resp.decode("utf-8", "replace")
    print(text[:500])
    if "Update Success" in text or "OK" in text or "200" in text.split("\r\n", 1)[0]:
        return 0
    # Connection closed without body after success is common
    if not text:
        print("Empty response after full send — assuming reboot")
        return 0
    return 1


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("bin")
    ap.add_argument("--host", default="192.168.1.15")
    ap.add_argument("--chunk", type=int, default=1024)
    ap.add_argument("--pause", type=float, default=0.05)
    args = ap.parse_args()
    return upload(args.host, args.bin, args.chunk, args.pause)


if __name__ == "__main__":
    sys.exit(main())
