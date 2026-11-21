"""Identify star-pick clicks with the Tetra3 plate solver.

The clicks are already measured. This ranks them by brightness in the photo
and calls Tetra3.solve_from_centroids. One JSON object is written to stdout.

  python tool/tetra_solve.py path/to/photo.stars.json
"""

from __future__ import annotations

import json
import logging
import math
import shutil
import sys
import time
import urllib.request
from pathlib import Path

import numpy as np

# NumPy 2 dropped np.math. Tetra3 still calls np.math.factorial while solving.
if not hasattr(np, "math"):
    np.math = math

HERE = Path(__file__).resolve().parent / "tetra"
CATALOG = HERE / "bsc5"
DATABASE = HERE / "phone90.npz"
BSC5_URL = "http://tdc-www.harvard.edu/catalogs/BSC5"
# Patterns may span the diagonal of a phone frame whose long side is about 70°.
MAX_FOV_DEG = 90.0
STAR_MAX_MAG = 7.0


def main() -> None:
    if len(sys.argv) not in (2, 3, 4):
        print(json.dumps({"ok": False, "reason": "Pass the stars sidecar."}))
        return
    pattern = 12
    timeout_s = 10.0
    if len(sys.argv) >= 3:
        try:
            pattern = int(sys.argv[2])
        except ValueError:
            print(json.dumps({"ok": False, "reason": "The pattern star count is not a number."}))
            return
    if len(sys.argv) == 4:
        try:
            timeout_s = float(sys.argv[3])
        except ValueError:
            print(json.dumps({"ok": False, "reason": "The solve time is not a number."}))
            return
    try:
        payload = solve_sidecar(Path(sys.argv[1]), pattern, timeout_s)
    except Exception as exc:  # noqa: BLE001 — the app shows this reason
        payload = {"ok": False, "reason": str(exc)}
    print(json.dumps(payload))


def solve_sidecar(path: Path, pattern_stars: int = 12, timeout_s: float = 10.0) -> dict:
    data = json.loads(path.read_text(encoding="utf-8"))
    stars = data.get("stars") or []
    if len(stars) < 4:
        return {"ok": False, "reason": "Tetra needs at least 4 clicks."}
    width = float(data["width"])
    height = float(data["height"])
    fov_long = float(str(data.get("fov", "")).replace(",", "."))
    if fov_long <= 0:
        return {"ok": False, "reason": "The long-side field is missing."}
    photo = data.get("photo")
    gray = load_gray(photo, width, height) if photo else None
    ranked = rank_clicks(stars, gray)
    centroids = np.array([[item["y"], item["x"]] for item in ranked], dtype=np.float64)
    fov_horizontal = fov_long * width / max(width, height)
    solution = solve_centroids(
        centroids, (height, width), fov_horizontal, pattern_stars, timeout_s
    )
    if solution is None or solution.get("RA") is None:
        reason = (
            "Tetra stopped at the time limit before a pattern matched."
            if solution is not None and solution.get("timed_out")
            else "Tetra found no pattern in these clicks."
        )
        return {"ok": False, "reason": reason}
    return pack_solution(solution, ranked)


def load_gray(photo: str, width: float, height: float):
    from PIL import Image, ImageOps

    raw = Image.open(photo)
    turned = ImageOps.exif_transpose(raw)
    if turned.size == (int(width), int(height)):
        image = turned
    elif raw.size == (int(width), int(height)):
        image = raw
    else:
        raise RuntimeError(
            f"The photo is {raw.size[0]}×{raw.size[1]} and the clicks use {int(width)}×{int(height)}."
        )
    return np.asarray(image.convert("L"))


def rank_clicks(stars: list, gray) -> list[dict]:
    ranked = []
    for index, star in enumerate(stars):
        x = float(star["x"])
        y = float(star["y"])
        ranked.append(
            {
                "index": index,
                "x": x,
                "y": y,
                "flux": 0.0 if gray is None else aperture_flux(gray, x, y),
            }
        )
    ranked.sort(key=lambda item: item["flux"], reverse=True)
    return ranked


def aperture_flux(gray, x: float, y: float, radius: int = 8) -> float:
    height, width = gray.shape
    cx = int(round(x))
    cy = int(round(y))
    x0 = max(0, cx - radius)
    x1 = min(width, cx + radius + 1)
    y0 = max(0, cy - radius)
    y1 = min(height, cy + radius + 1)
    patch = gray[y0:y1, x0:x1].astype(np.float64)
    if patch.size == 0:
        return 0.0
    return float(patch.sum() - np.median(patch) * patch.size)


def solve_centroids(
    centroids, size, fov_horizontal: float, pattern_stars: int = 12, timeout_s: float = 10.0
):
    t3 = load_tetra()
    attempts = (
        {"distortion": (-0.25, 0.05), "fov_max_error": 12},
        {"distortion": None, "fov_max_error": 15},
    )
    best = None
    checking = min(max(4, min(pattern_stars, 30)), len(centroids))
    budget = min(30.0, max(1.0, float(timeout_s)))
    deadline = time.perf_counter() + budget
    for attempt in attempts:
        remaining_ms = (deadline - time.perf_counter()) * 1000
        if remaining_ms < 50:
            break
        solution = t3.solve_from_centroids(
            centroids,
            size,
            fov_estimate=fov_horizontal,
            fov_max_error=attempt["fov_max_error"],
            pattern_checking_stars=checking,
            match_radius=0.02,
            distortion=attempt["distortion"],
            solve_timeout=remaining_ms,
            return_matches=True,
        )
        if solution.get("RA") is None:
            continue
        if best is None or (solution["Matches"], -solution["RMSE"]) > (
            best["Matches"],
            -best["RMSE"],
        ):
            best = solution
        if solution["Matches"] >= 8 and solution["RMSE"] < 300:
            break
    if best is None and time.perf_counter() >= deadline - 0.05:
        return {"RA": None, "timed_out": True}
    return best


def load_tetra():
    path = DATABASE
    ensure_database(path, 40)
    from tetra3 import Tetra3

    solver = Tetra3(path)
    logging.getLogger("tetra3.Tetra3").setLevel(logging.ERROR)
    return solver


def ensure_database(path: Path, verification: int) -> None:
    if path.exists():
        return
    HERE.mkdir(parents=True, exist_ok=True)
    if not CATALOG.exists() or CATALOG.stat().st_size < 100_000:
        urllib.request.urlretrieve(BSC5_URL, CATALOG)
    import tetra3

    destination = Path(tetra3.__file__).resolve().parent / "bsc5"
    if not destination.exists() or destination.stat().st_size != CATALOG.stat().st_size:
        shutil.copyfile(CATALOG, destination)
    from tetra3 import Tetra3

    print(
        f"Building the Tetra catalog for a 90° field, {verification} verification stars…",
        file=sys.stderr,
    )
    solver = Tetra3(load_database=None)
    logging.getLogger("tetra3.Tetra3").setLevel(logging.INFO)
    temporary = path.with_name(path.name + ".building")
    solver.generate_database(
        max_fov=MAX_FOV_DEG,
        star_catalog="bsc5",
        star_max_magnitude=STAR_MAX_MAG,
        pattern_stars_per_fov=12,
        verification_stars_per_fov=verification,
        save_as=temporary,
    )
    if path.exists():
        temporary.unlink(missing_ok=True)
        return
    temporary.replace(path)


def pack_solution(solution: dict, ranked: list[dict]) -> dict:
    sent = [(item["y"], item["x"], item["index"]) for item in ranked]
    matched_centroids = solution.get("matched_centroids") or []
    matched_stars = solution.get("matched_stars") or []
    matched_ids = solution.get("matched_catID") or []
    stars = []
    used = set()
    for row, sky, cat in zip(matched_centroids, matched_stars, matched_ids):
        y, x = float(row[0]), float(row[1])
        nearest = None
        nearest_d = 9.0
        for i, (sy, sx, index) in enumerate(sent):
            if i in used:
                continue
            distance = (sy - y) ** 2 + (sx - x) ** 2
            if distance < nearest_d:
                nearest_d = distance
                nearest = i
                click = index
        if nearest is None:
            continue
        used.add(nearest)
        stars.append(
            {
                "index": click,
                "x": sent[nearest][1],
                "y": sent[nearest][0],
                "ra_deg": float(sky[0]),
                "dec_deg": float(sky[1]),
                "mag": float(sky[2]),
                "cat": int(cat),
            }
        )
    return {
        "ok": True,
        "ra_deg": float(solution["RA"]),
        "dec_deg": float(solution["Dec"]),
        "roll_deg": float(solution["Roll"]),
        "fov_deg": float(solution["FOV"]),
        "distortion": None if solution["distortion"] is None else float(solution["distortion"]),
        "rmse_arcsec": float(solution["RMSE"]),
        "matches": int(solution["Matches"]),
        "prob": float(solution["Prob"]),
        "ms": float(solution["T_solve"]),
        "epoch_equinox": int(round(float(solution["epoch_equinox"]))),
        "stars": stars,
    }


if __name__ == "__main__":
    main()
