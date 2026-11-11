#!/usr/bin/env python3
"""
Standalone TeenAstroEfinder UART simulator with synthetic star fields.

Optics default to a ~1° FOV guide setup: choose an ASI camera, guider focal
length and aperture; limiting magnitude is estimated from those (AAVSO CCD
form). Catalog stars to mag 9 plus a density fill to the limiting magnitude.

Examples:
  py -3 efinder_sim.py --port COM5 --baud 9600
  py -3 efinder_sim.py --port COM5 --asi ASI220MM Mini --focal 250 --aperture 50
  py -3 efinder_sim.py --port COM5 --fov 1.0 --save-images
"""
from __future__ import annotations

import argparse
import logging
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))

from solver.optics import ASI_CAMERAS, describe_optics, optics_from_param  # noqa: E402
from solver.plate_solve import PlateSolver  # noqa: E402
from solver.sim_camera import SimCamera  # noqa: E402
from solver.teenastro_link import SolveOutcome, TeenAstroLink  # noqa: E402

LOG = logging.getLogger("efinder_sim")


def main() -> int:
    ap = argparse.ArgumentParser(description="TeenAstro eFinder UART simulator (synthetic stars)")
    ap.add_argument("--port", required=True, help="Serial port (e.g. COM5 or /dev/ttyUSB0)")
    ap.add_argument("--baud", type=int, default=9600, choices=(9600, 56000),
                    help="9600 = GNSS port, 56000 = Focuser port")
    ap.add_argument("--asi", default="ASI120MM Mini",
                    help=f"ASI camera model ({', '.join(ASI_CAMERAS)})")
    ap.add_argument("--focal", type=float, default=None,
                    help="Guider focal length in mm (default: set for --fov on the long axis)")
    ap.add_argument("--aperture", type=float, default=50.0, help="Guider aperture in mm")
    ap.add_argument("--fov", type=float, default=1.0,
                    help="Target FOV across the long sensor axis in degrees (used when --focal omitted)")
    ap.add_argument("--exposure", type=float, default=1.0, help="Exposure seconds (affects limiting mag)")
    ap.add_argument("--ra", type=float, default=None, help="Initial pointing RA hours (until :eC arrives)")
    ap.add_argument("--dec", type=float, default=None, help="Initial pointing Dec degrees")
    ap.add_argument("--save-images", action="store_true", help="Write capture.png under ./sim_images/")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
    )

    param = {
        "Camera": "SIM",
        "Test_mode": "1",
        "Sim_ASI": args.asi,
        "Sim_aperture_mm": str(args.aperture),
        "Sim_fov_deg": str(args.fov),
        "Exposure": str(args.exposure),
        "Gain": "50",
        "Ramdisk": "false",
        "d_x": "0",
        "d_y": "0",
    }
    if args.focal is not None:
        param["Sim_focal_mm"] = str(args.focal)

    optics = optics_from_param(param)
    LOG.info("%s", describe_optics(optics))

    data_dir = ROOT / "data"
    camera = SimCamera.from_param(param, data_dir)
    if args.ra is not None and args.dec is not None:
        camera.set_pointing(args.ra, args.dec)

    class _Coords:
        def j2000_to_jnow(self, ra, dec, lat, lon, utc=None):
            return ra, dec

    img_root = ROOT / "sim_images"
    if args.save_images:
        img_root.mkdir(parents=True, exist_ok=True)

    solver = PlateSolver(param, _Coords(), camera, data_dir)
    if args.save_images:
        solver.dest_path = lambda: str(img_root) + "/"  # type: ignore[method-assign]

    link_holder: dict = {}

    def apply_pointing() -> None:
        site = link_holder["link"].state.site
        if site.pointing_valid:
            camera.set_pointing(site.ra_hours, site.dec_deg)
            LOG.info("pointing from main unit: RA %.4fh Dec %.3f°", site.ra_hours, site.dec_deg)

    def do_solve() -> None:
        link = link_holder["link"]
        site = link.state.site
        try:
            apply_pointing()
            t0 = time.time()
            solver.capture()
            outcome = solver.solve(site.lat, site.lon_east, site.utc)
            if not outcome.ok and site.pointing_valid:
                ra, dec, stars = camera.solved_from_pointing()
                outcome = SolveOutcome(
                    ok=True, ra_hours=ra, dec_deg=dec, stars=stars, ms=int((time.time() - t0) * 1000)
                )
            link.set_solved(outcome)
            LOG.info(
                "solve -> ok=%s RA=%.4fh Dec=%.3f stars=%d LM=%.1f",
                outcome.ok,
                outcome.ra_hours,
                outcome.dec_deg,
                outcome.stars,
                camera.mag_limit,
            )
        except Exception as exc:  # noqa: BLE001
            LOG.exception("solve failed: %s", exc)
            link.set_solved(SolveOutcome(ok=False, fail_code="E"))

    def do_offset() -> None:
        link = link_holder["link"]
        try:
            apply_pointing()
            outcome = solver.measure_offset(link.state.site.lat, link.state.site.lon_east, link.state.site.utc)
            if not outcome.ok:
                name = camera.last_brightest.name if camera.last_brightest else "sim"
                outcome = SolveOutcome(
                    ok=True, is_offset=True, offset_dx=0.0, offset_dy=0.0, offset_name=name or "sim",
                    stars=camera.last_count,
                )
            link.set_solved(outcome)
        except Exception as exc:  # noqa: BLE001
            LOG.exception("offset failed: %s", exc)
            link.set_solved(SolveOutcome(ok=False, fail_code="O", is_offset=True))

    link = TeenAstroLink(
        on_solve=do_solve,
        on_offset=do_offset,
        on_reset_offset=lambda: solver.reset_offset(),
        on_param=lambda body: "1#" if "=" in body else "#",
        on_nmea_enable=lambda on: LOG.info("NMEA relay %s", "on" if on else "off"),
        port=args.port,
    )
    import serial

    link._ser = serial.Serial(args.port, baudrate=args.baud, timeout=0.05)
    link._baud_locked = True
    link_holder["link"] = link
    LOG.info(
        "eFinder star-field simulator on %s @ %s baud (%d catalog + faint fill to mag %.1f)",
        args.port,
        args.baud,
        len(camera.stars),
        camera.mag_limit,
    )
    try:
        link.run_forever()
    except KeyboardInterrupt:
        LOG.info("stopped")
    finally:
        link.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
