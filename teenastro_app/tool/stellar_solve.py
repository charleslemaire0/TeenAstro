"""Solve one photo with the StellarSolver library.

StellarSolver extracts its own stars and plate-solves the picture. This does
not read the Cedar sidecar. One JSON object is written to stdout.

  python tool/stellar_solve.py photo.jpg index-folder scale-low scale-high downsample seconds

scale-low and scale-high are the image width in degrees. When
tool/stellar/stellarsolver-json.exe is present it is used. Otherwise the
photo is matched to the Astrometry.net index files in the folder.
"""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path


def fail(reason: str) -> None:
    json.dump({"ok": False, "reason": reason}, sys.stdout)
    sys.stdout.write("\n")


def _tiled_median(image, tile: int):
    import numpy as np
    from scipy.ndimage import zoom

    height, width = image.shape
    tile = max(1, min(tile, height, width))
    tiles_y = max(1, height // tile)
    tiles_x = max(1, width // tile)
    trimmed = image[: tiles_y * tile, : tiles_x * tile]
    blocks = trimmed.reshape(tiles_y, tile, tiles_x, tile)
    coarse = np.median(blocks, axis=(1, 3))
    background = zoom(coarse, (height / tiles_y, width / tiles_x), order=1)
    return background[:height, :width]


def extract_stars(image: Path, downsample: int, limit: int = 80) -> list[dict]:
    """SEP-style peaks on the downsampled photo. Coordinates are full-image pixels."""
    import numpy as np
    from PIL import Image
    from scipy.ndimage import maximum_filter

    gray = np.asarray(Image.open(image).convert("L"), dtype=np.float32)
    height, width = gray.shape
    factor = max(1, int(downsample))
    if factor > 1:
        trimmed_h = height // factor * factor
        trimmed_w = width // factor * factor
        blocks = gray[:trimmed_h, :trimmed_w].reshape(
            trimmed_h // factor, factor, trimmed_w // factor, factor
        )
        small = blocks.mean(axis=(1, 3))
    else:
        small = gray
    background = _tiled_median(small, 48)
    residual = small - background
    deviation = np.median(np.abs(residual - np.median(residual)))
    sigma = max(float(1.4826 * deviation), 0.5)
    peaks = residual >= 6.0 * sigma
    peaks &= residual == maximum_filter(residual, size=7)
    ys, xs = np.nonzero(peaks)
    found = []
    radius = 4
    small_h, small_w = small.shape
    for y, x in zip(ys.tolist(), xs.tolist()):
        peak = float(residual[y, x])
        span = 5
        sy0 = max(0, y - span)
        sy1 = min(small_h, y + span + 1)
        sx0 = max(0, x - span)
        sx1 = min(small_w, x + span + 1)
        neighborhood = residual[sy0:sy1, sx0:sx1]
        # A star is a compact peak. Horizon lights and the ground fill the window.
        if int(np.count_nonzero(neighborhood > 0.5 * peak)) > 10:
            continue
        y0 = max(0, y - radius)
        y1 = min(small_h, y + radius + 1)
        x0 = max(0, x - radius)
        x1 = min(small_w, x + radius + 1)
        weight = np.clip(residual[y0:y1, x0:x1], 0, None)
        total = float(weight.sum())
        if total <= 0:
            continue
        yy, xx = np.mgrid[y0:y1, x0:x1]
        cx = float((xx * weight).sum() / total)
        cy = float((yy * weight).sum() / total)
        full_x = (cx + 0.5) * factor - 0.5
        full_y = (cy + 0.5) * factor - 0.5
        found.append((total, full_x, full_y))
    found.sort(reverse=True)
    return [
        {"x": x, "y": y}
        for _flux, x, y in found[:limit]
    ]


def _star_coords(star, center):
    import numpy as np

    denom = float(np.dot(star, center))
    if denom <= 0:
        return None
    ix, iy = float(center[1]), float(-center[0])
    norm = (ix * ix + iy * iy) ** 0.5
    if norm < 1e-12:
        return None
    ix /= norm
    iy /= norm
    jx = iy * float(center[2])
    jy = -ix * float(center[2])
    jz = ix * float(center[1]) - iy * float(center[0])
    sx = float(star[0]) * ix + float(star[1]) * iy
    sy = float(star[0]) * jx + float(star[1]) * jy + float(star[2]) * jz
    # The Astrometry.net caller stores the axes swapped.
    return sy / denom, sx / denom


def _triangle_code(xyz):
    import numpy as np

    mid = xyz[0] + xyz[1]
    mid = mid / np.linalg.norm(mid)
    projected = [_star_coords(star, mid) for star in xyz]
    if any(item is None for item in projected):
        return None
    (ay, ax), (by, bx), (cy, cx) = projected
    abx = bx - ax
    aby = by - ay
    scale = abx * abx + aby * aby
    if scale <= 0:
        return None
    inv = 1.0 / scale
    cost = (aby + abx) * inv
    sint = (aby - abx) * inv
    adx = cx - ax
    ady = cy - ay
    x = adx * cost + ady * sint
    y = -adx * sint + ady * cost
    if x > 0.5:
        x, y = 1.0 - x, 1.0 - y
    return x, y


def _load_indexes(folder: Path) -> list[dict]:
    import numpy as np
    from astropy.io import fits

    loaded = []
    for path in sorted(folder.glob("index-*.fits")):
        with fits.open(path) as hdus:
            header = hdus[0].header
            quads = np.frombuffer(
                np.ascontiguousarray(hdus[1].data["quads"]).tobytes(), dtype="<u4"
            ).reshape(-1, 3)
            raw_codes = np.frombuffer(
                np.ascontiguousarray(hdus[6].data["kdtree_data_codes"]).tobytes(),
                dtype="<u2",
            ).reshape(-1, 2).astype(np.float64)
            raw_stars = np.frombuffer(
                np.ascontiguousarray(hdus[11].data["kdtree_data_stars"]).tobytes(),
                dtype="<u4",
            ).reshape(-1, 3).astype(np.float64)
        low = -0.20710678118654752
        high = 1.2071067811865475
        codes = low + raw_codes * (high - low) / 65535.0
        stars = -1.0 + 2.0 * raw_stars / 2**32
        step = 0.01
        buckets: dict[tuple[int, int], list[int]] = {}
        for index, code in enumerate(codes):
            key = (int(np.floor(code[0] / step)), int(np.floor(code[1] / step)))
            buckets.setdefault(key, []).append(index)
        loaded.append(
            {
                "name": path.name,
                "scale_l": float(header["SCALE_L"]),
                "scale_u": float(header["SCALE_U"]),
                "quads": quads,
                "codes": codes,
                "stars": stars,
                "buckets": buckets,
                "step": step,
            }
        )
    return loaded


def _lookup_codes(index: dict, code, tolerance: float = 0.006) -> list[tuple[float, int]]:
    import numpy as np

    step = index["step"]
    ix = int(np.floor(code[0] / step))
    iy = int(np.floor(code[1] / step))
    hits = []
    codes = index["codes"]
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            for quad in index["buckets"].get((ix + dx, iy + dy), ()):
                stored = codes[quad]
                distance = max(abs(stored[0] - code[0]), abs(stored[1] - code[1]))
                if distance <= tolerance:
                    hits.append((distance, quad))
    return hits


def _rotation(camera, catalog):
    import numpy as np

    # Columns are vectors. The result maps a camera direction onto the catalog.
    matrix = catalog @ camera.T
    left, _, right = np.linalg.svd(matrix)
    rotation = left @ right
    if np.linalg.det(rotation) < 0:
        left = left.copy()
        left[:, -1] *= -1
        rotation = left @ right
    return rotation


def _solve_indexes(stars, width, height, low_deg, high_deg, indexes, deadline) -> dict | None:
    import numpy as np

    xy = np.array([[star["x"], star["y"]] for star in stars], dtype=np.float64)
    mid = (low_deg + high_deg) / 2.0
    best = None
    for flip in (True, False):
        for scale in (mid, low_deg, high_deg):
            if time.perf_counter() > deadline:
                return best
            outcome = _search_scale(xy, width, height, scale, flip, indexes, deadline)
            if outcome is None:
                continue
            outcome = _refine(outcome, indexes)
            if best is None or (outcome["matches"], -outcome["rmse"]) > (
                best["matches"],
                -best["rmse"],
            ):
                best = outcome
            if outcome["matches"] >= 12 and outcome["rmse"] < 600:
                return best
    return best


def _camera_vectors(xy, width, height, width_deg, flip_y: bool):
    import numpy as np

    center_x = (width - 1) / 2.0
    center_y = (height - 1) / 2.0
    focal = (width / 2.0) / np.tan(np.deg2rad(width_deg) / 2.0)
    horizontal = (xy[:, 0] - center_x) / focal
    vertical = ((center_y - xy[:, 1]) if flip_y else (xy[:, 1] - center_y)) / focal
    forward = np.ones(len(xy))
    vectors = np.stack([horizontal, vertical, forward], axis=1)
    return vectors / np.linalg.norm(vectors, axis=1, keepdims=True)


def _search_scale(xy, width, height, width_deg, flip_y, indexes, deadline):
    import numpy as np

    vectors = _camera_vectors(xy, width, height, width_deg, flip_y)
    pool = min(len(vectors), 26)
    hypotheses = []
    for i in range(pool):
        for j in range(i + 1, pool):
            if time.perf_counter() > deadline:
                break
            separation = float(np.arccos(np.clip(np.dot(vectors[i], vectors[j]), -1, 1)))
            usable = [
                index
                for index in indexes
                if index["scale_l"] * 0.85 <= separation <= index["scale_u"] * 1.15
            ]
            if not usable:
                continue
            for k in range(pool):
                if k == i or k == j:
                    continue
                code = _triangle_code(vectors[[i, j, k]])
                if code is None:
                    continue
                for index in usable:
                    for distance, quad in _lookup_codes(index, code):
                        hypotheses.append((distance, index, quad, (i, j, k)))
    hypotheses.sort(key=lambda item: item[0])
    best = None
    checked = 0
    for distance, index, quad, (i, j, k) in hypotheses:
        if time.perf_counter() > deadline or checked >= 40:
            break
        if distance > 0.004:
            continue
        checked += 1
        stars = index["stars"]
        ids = index["quads"][quad]
        camera = np.column_stack([vectors[i], vectors[j], vectors[k]])
        catalog = np.column_stack([stars[ids[0]], stars[ids[1]], stars[ids[2]]])
        rotation = _rotation(camera, catalog)
        predicted = vectors @ rotation.T
        dots = predicted @ stars.T
        nearest = np.argmax(dots, axis=1)
        cosine = dots[np.arange(len(vectors)), nearest]
        matched = cosine > np.cos(np.deg2rad(0.35))
        count = int(matched.sum())
        if count < 6:
            continue
        angles = np.degrees(np.arccos(np.clip(cosine[matched], -1, 1))) * 3600.0
        rmse = float(np.sqrt(np.mean(angles * angles)))
        if best is None or (count, -rmse) > (best["matches"], -best["rmse"]):
            best = {
                "rotation": rotation,
                "matches": count,
                "rmse": rmse,
                "vectors": vectors,
                "flip_y": flip_y,
            }
            if count >= 12 and rmse < 600:
                break
    return best


def _refine(solution: dict, indexes: list[dict]) -> dict:
    """Refit the rotation on every star the triangle already placed."""
    import numpy as np

    catalog = np.vstack([index["stars"] for index in indexes])
    vectors = solution["vectors"]
    rotation = solution["rotation"]
    matched = None
    nearest = None
    for gate in (0.4, 0.25):
        predicted = vectors @ rotation.T
        dots = predicted @ catalog.T
        nearest = np.argmax(dots, axis=1)
        cosine = dots[np.arange(len(vectors)), nearest]
        matched = cosine > np.cos(np.deg2rad(gate))
        if int(matched.sum()) < 6:
            return solution
        rotation = _rotation(vectors[matched].T, catalog[nearest[matched]].T)
    predicted = vectors @ rotation.T
    dots = predicted @ catalog.T
    nearest = np.argmax(dots, axis=1)
    cosine = dots[np.arange(len(vectors)), nearest]
    matched = cosine > np.cos(np.deg2rad(0.25))
    count = int(matched.sum())
    if count < 6:
        return solution
    angles = np.degrees(np.arccos(np.clip(cosine[matched], -1, 1))) * 3600.0
    refined = dict(solution)
    refined["rotation"] = rotation
    refined["matches"] = count
    refined["rmse"] = float(np.sqrt(np.mean(angles * angles)))
    return refined


def _pack_solution(stars, solution) -> dict:
    import numpy as np

    rotation = solution["rotation"]
    vectors = solution["vectors"]
    center = rotation @ np.array([0.0, 0.0, 1.0])
    up = rotation @ np.array([0.0, 1.0, 0.0])
    up = up - center * float(np.dot(up, center))
    ra = float(np.arctan2(center[1], center[0]))
    if ra < 0:
        ra += 2 * np.pi
    dec = float(np.arcsin(np.clip(center[2], -1, 1)))
    north = np.array(
        [-np.sin(dec) * np.cos(ra), -np.sin(dec) * np.sin(ra), np.cos(dec)]
    )
    east = np.array([-np.sin(ra), np.cos(ra), 0.0])
    roll = float(np.degrees(np.arctan2(np.dot(up, east), np.dot(up, north))))
    sky = vectors @ rotation.T
    rows = []
    for star, direction in zip(stars, sky):
        star_ra = float(np.degrees(np.arctan2(direction[1], direction[0])))
        if star_ra < 0:
            star_ra += 360.0
        star_dec = float(np.degrees(np.arcsin(np.clip(direction[2], -1, 1))))
        rows.append(
            {
                "x": star["x"],
                "y": star["y"],
                "ra_deg": star_ra,
                "dec_deg": star_dec,
            }
        )
    return {
        "ra_deg": float(np.degrees(ra)),
        "dec_deg": float(np.degrees(dec)),
        "roll_deg": roll,
        "rmse_arcsec": solution["rmse"],
        "matches": solution["matches"],
        "stars": rows,
    }


def find_exe() -> Path | None:
    env = os.environ.get("STELLARSOLVER_JSON", "").strip()
    if env and Path(env).is_file():
        return Path(env)
    here = Path(__file__).resolve().parent / "stellar"
    for name in ("stellarsolver-json.exe", "stellarsolver-json"):
        candidate = here / name
        if candidate.is_file():
            return candidate
    found = shutil.which("stellarsolver-json")
    if found:
        return Path(found)
    return None


def parse_json(text: str) -> dict | None:
    raw = text.strip()
    if not raw:
        return None
    try:
        payload = json.loads(raw)
    except json.JSONDecodeError:
        payload = None
    else:
        if isinstance(payload, dict):
            return payload
    for line in reversed(raw.splitlines()):
        line = line.strip()
        if not line.startswith("{"):
            continue
        try:
            payload = json.loads(line)
        except json.JSONDecodeError:
            continue
        if isinstance(payload, dict):
            return payload
    return None


def main() -> None:
    if len(sys.argv) != 7:
        fail(
            "StellarSolver needs the photo, the index folder, the scale range, the downsample, and the time limit."
        )
        return
    image = Path(sys.argv[1])
    index = Path(sys.argv[2])
    try:
        low = float(sys.argv[3])
        high = float(sys.argv[4])
        downsample = int(sys.argv[5])
        seconds = int(sys.argv[6])
    except ValueError:
        fail("The scale, downsample, or time limit is not a number.")
        return
    if not image.is_file():
        fail("The photo is not there.")
        return
    if not index.is_dir():
        fail("The index folder is not there.")
        return
    if low <= 0 or high <= low:
        fail("The scale range is not usable.")
        return
    if downsample < 1:
        fail("The downsample is not usable.")
        return
    if seconds < 1:
        fail("The solve time is not usable.")
        return
    started = time.perf_counter()
    exe = find_exe()
    if exe is None:
        try:
            stars = extract_stars(image, downsample)
        except Exception as exc:  # noqa: BLE001 — the window shows this reason
            fail(f"StellarSolver could not extract stars ({exc}).")
            return
        if len(stars) < 4:
            fail(f"StellarSolver found {len(stars)} stars.")
            return
        from PIL import Image

        with Image.open(image) as photo:
            width, height = photo.size
        indexes = _load_indexes(index)
        solution = (
            _solve_indexes(stars, width, height, low, high, indexes, started + seconds)
            if indexes
            else None
        )
        payload = {
            "ok": True,
            "stars": stars,
            "ms": (time.perf_counter() - started) * 1000.0,
            "epoch_equinox": 2000,
        }
        if solution is not None:
            payload.update(_pack_solution(stars, solution))
            payload["ms"] = (time.perf_counter() - started) * 1000.0
        json.dump(payload, sys.stdout)
        sys.stdout.write("\n")
        return
    try:
        result = subprocess.run(
            [
                str(exe),
                str(image),
                str(index),
                str(low),
                str(high),
                str(downsample),
                str(seconds),
            ],
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=seconds + 30,
        )
    except subprocess.TimeoutExpired:
        fail("StellarSolver stopped at the time limit.")
        return
    except OSError as exc:
        fail(f"StellarSolver could not be started ({exc}).")
        return
    payload = parse_json(result.stdout or "")
    if payload is None:
        err = (result.stderr or "").strip()
        detail = err or (result.stdout or "").strip()
        fail(
            "StellarSolver did not return a solution."
            if not detail
            else f"StellarSolver did not return a solution. {detail}"
        )
        return
    if payload.get("ok") is True and payload.get("ms") is None:
        payload["ms"] = (time.perf_counter() - started) * 1000.0
    json.dump(payload, sys.stdout)
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
