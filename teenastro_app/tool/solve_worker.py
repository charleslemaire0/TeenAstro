"""Cedar and Tetra, kept running.

The photo window writes one JSON job per line and reads one JSON reply.
Python, the Cedar client, and the Tetra catalog are loaded once.

  {"cmd": "extract", "sidecar": "...", "sigma": 8, "binning": 2, "limit": 80}
  {"cmd": "solve", "sidecar": "...", "pattern": 8, "timeout_s": 0.4}
  {"cmd": "solve_frames", "fov": 69.4, "width": 3060, "height": 4080,
   "frames": [{"dx": 0, "width": 2754, "height": 4080, "stars": [{"x", "y"}]}]}

The photo window extracts stars itself with tool/cedar/cedar_native.dll.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path


def serve() -> None:
    import grpc  # noqa: F401 — paid once, before the first photo
    import cedar_extract  # noqa: F401
    import tetra_solve

    tetra_solve.load_tetra()
    sys.stdout.write(json.dumps({"ok": True, "ready": True}) + "\n")
    sys.stdout.flush()
    for line in sys.stdin:
        text = line.strip()
        if not text:
            continue
        try:
            job = json.loads(text)
        except json.JSONDecodeError:
            payload = {"ok": False, "reason": "The job was not JSON."}
        else:
            payload = _run(job)
        sys.stdout.write(json.dumps(payload) + "\n")
        sys.stdout.flush()


def _run(job: dict) -> dict:
    command = job.get("cmd")
    try:
        if command == "extract":
            import cedar_extract

            return cedar_extract.extract_sidecar(
                Path(job["sidecar"]),
                float(job["sigma"]),
                int(job["binning"]),
                int(job["limit"]),
            )
        if command == "solve":
            import tetra_solve

            return tetra_solve.solve_sidecar(
                Path(job["sidecar"]),
                int(job.get("pattern", 8)),
                float(job.get("timeout_s", 0.4)),
            )
        if command == "solve_frames":
            import tetra_solve

            return tetra_solve.solve_frames(
                job.get("frames") or [],
                float(job["fov"]),
                float(job["width"]),
                float(job["height"]),
                int(job.get("pattern", 8)),
                float(job.get("timeout_s", 0.8)),
            )
    except Exception as exc:  # noqa: BLE001 — the window shows this reason
        return {"ok": False, "reason": str(exc)}
    return {"ok": False, "reason": "The job is not a Cedar extract or a Tetra solve."}


if __name__ == "__main__":
    serve()
