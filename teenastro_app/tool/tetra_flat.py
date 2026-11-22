"""Write a Tetra3 .npz database as the flat file the C++ solver reads.

  python tool/tetra_flat.py tool/tetra/phone55.npz assets/tetra/phone55.tetra

Layout, little endian:
  0   8   magic "TETRA001"
  8   u32 star count
  12  u32 pattern table rows
  16  u16 pattern size (4)
  18  u16 pattern bins
  20  f32 pattern max error
  24  f32 max fov (deg)
  28  f32 min fov (deg)
  32  u16 epoch equinox
  34  u16 verification stars per fov
  36  u8  presorted patterns
  37  27 bytes zero
  64  f32 star table, stars x 6 (ra, dec, x, y, z, mag), radians
  ..  u16 catalog ids, stars
  ..  u16 pattern table, rows x 4
  ..  f32 largest edge per row, milliradians
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

import numpy as np


def main() -> int:
    if len(sys.argv) != 3:
        print("Pass the .npz database and the output file.")
        return 2
    source = Path(sys.argv[1])
    target = Path(sys.argv[2])
    with np.load(source) as data:
        stars = np.ascontiguousarray(data["star_table"], dtype="<f4")
        ids = np.ascontiguousarray(data["star_catalog_IDs"], dtype="<u2")
        table = np.ascontiguousarray(data["pattern_catalog"], dtype="<u2")
        edges = np.ascontiguousarray(data["pattern_largest_edge"], dtype="<f4")
        props = data["props_packed"][()]
    if table.shape[1] != 4 or stars.shape[1] != 6 or ids.ndim != 1:
        print("Only 4-star bsc5 databases are supported.")
        return 1
    header = struct.pack(
        "<8sIIHHfffHHB",
        b"TETRA001",
        stars.shape[0],
        table.shape[0],
        int(props["pattern_size"]),
        int(props["pattern_bins"]),
        float(props["pattern_max_error"]),
        float(props["max_fov"]),
        float(props["min_fov"]),
        int(props["epoch_equinox"]),
        int(props["verification_stars_per_fov"]),
        1 if bool(props["presort_patterns"]) else 0,
    )
    header = header.ljust(64, b"\0")
    target.parent.mkdir(parents=True, exist_ok=True)
    with target.open("wb") as out:
        out.write(header)
        out.write(stars.tobytes())
        out.write(ids.tobytes())
        out.write(table.tobytes())
        out.write(edges.tobytes())
    print(f"{target} {target.stat().st_size} bytes, {stars.shape[0]} stars, {table.shape[0]} rows")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
