#!/usr/bin/env python3
"""
TeenAstroEfinder entry point.

UART slave for the TeenAstro main unit (Focuser @ 56000 or GNSS @ 9600).
Plate-solves with tetra3/cedar when available; Test_mode returns a fixed solution.
When on the GNSS port, optionally relays NMEA from a USB GPS dongle.
"""
from __future__ import annotations

import logging
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))

from solver.coordinates import Coordinates  # noqa: E402
from solver.gnss_relay import GnssRelay  # noqa: E402
from solver.plate_solve import PlateSolver  # noqa: E402
from solver.teenastro_link import TeenAstroLink  # noqa: E402

LOG = logging.getLogger("efinder")


def load_config(path: Path) -> dict:
    param: dict = {}
    if not path.exists():
        return param
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or ":" not in line:
            continue
        k, v = line.split(":", 1)
        param[k.strip()] = v.strip()
    return param


def save_config(path: Path, param: dict) -> None:
    lines = [f"{k}:{v}\n" for k, v in param.items()]
    path.write_text("".join(lines), encoding="utf-8")


def make_camera(param: dict, data_dir: Path):
    kind = param.get("Camera", "RPI").upper()
    test_mode = param.get("Test_mode", "0") in ("1", "True", "true")
    if kind == "SIM" or test_mode:
        from solver.sim_camera import SimCamera

        return SimCamera.from_param(param, data_dir)
    if kind == "ASI":
        from solver.asi_camera import ASICamera

        return ASICamera()
    from solver.rpi_camera import RPICamera

    return RPICamera()


def main() -> int:
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    cfg_path = Path(os.environ.get("EFINDER_CONFIG", ROOT / "efinder.config"))
    param = load_config(cfg_path)
    port = param.get("Port", "/dev/serial0")
    test_mode = param.get("Test_mode", "0") in ("1", "True", "true")
    data_dir = ROOT / "data"

    coordinates = Coordinates()
    camera = None
    solver = None
    try:
        camera = make_camera(param, data_dir)
        if camera.get_cam_type() == "not found" and not test_mode:
            LOG.warning("camera not found — solves will fail until Camera=SIM / Test_mode=1 or a camera is attached")
        solver = PlateSolver(param, coordinates, camera, data_dir)
    except Exception as exc:  # noqa: BLE001
        LOG.warning("solver init deferred: %s", exc)

    link_holder: dict = {}

    def apply_pointing() -> None:
        site = link_holder["link"].state.site
        if solver and site.pointing_valid:
            solver.set_pointing(site.ra_hours, site.dec_deg)

    def do_solve() -> None:
        link = link_holder["link"]
        site = link.state.site
        LOG.info(
            "solve requested (test_mode=%s pointing=%s)",
            test_mode,
            f"{site.ra_hours:.4f}h,{site.dec_deg:.3f}d" if site.pointing_valid else "none",
        )
        try:
            if solver is None:
                from solver.teenastro_link import SolveOutcome

                if test_mode or site.pointing_valid:
                    link.set_solved(
                        SolveOutcome(
                            ok=True,
                            ra_hours=site.ra_hours if site.pointing_valid else 5.5,
                            dec_deg=site.dec_deg if site.pointing_valid else 22.0,
                            stars=42,
                            ms=1,
                        )
                    )
                else:
                    link.set_solved(SolveOutcome(ok=False, fail_code="C"))
                return
            apply_pointing()
            solver.capture()
            outcome = solver.solve(site.lat, site.lon_east, site.utc)
            link.set_solved(outcome)
        except Exception as exc:  # noqa: BLE001
            LOG.exception("solve failed: %s", exc)
            from solver.teenastro_link import SolveOutcome

            link.set_solved(SolveOutcome(ok=False, fail_code="E"))

    def do_offset() -> None:
        link = link_holder["link"]
        site = link.state.site
        LOG.info("offset requested")
        try:
            if solver is None:
                from solver.teenastro_link import SolveOutcome

                link.set_solved(
                    SolveOutcome(ok=True, is_offset=True, offset_dx=0.0, offset_dy=0.0, offset_name="test")
                )
                return
            apply_pointing()
            outcome = solver.measure_offset(site.lat, site.lon_east, site.utc)
            save_config(cfg_path, param)
            link.set_solved(outcome)
        except Exception as exc:  # noqa: BLE001
            LOG.exception("offset failed: %s", exc)
            from solver.teenastro_link import SolveOutcome

            link.set_solved(SolveOutcome(ok=False, fail_code="O", is_offset=True))

    def do_reset_offset() -> None:
        if solver:
            solver.reset_offset()
        else:
            param["d_x"] = "0"
            param["d_y"] = "0"
        save_config(cfg_path, param)
        LOG.info("offset reset")

    def on_param(body: str) -> str:
        if "=" in body:
            k, v = body.split("=", 1)
            param[k] = v
            save_config(cfg_path, param)
            return "1#"
        return f"{param.get(body, '')}#"

    relay = GnssRelay(on_nmea=lambda line: link_holder["link"].write_nmea(line), exclude_ports=[port])

    def on_nmea_enable(on: bool) -> None:
        relay.set_enabled(on)
        LOG.info("NMEA relay %s", "on" if on else "off")

    link = TeenAstroLink(
        on_solve=do_solve,
        on_offset=do_offset,
        on_reset_offset=do_reset_offset,
        on_param=on_param,
        on_nmea_enable=on_nmea_enable,
        port=port,
    )
    link_holder["link"] = link

    def gps_present_pump() -> None:
        link.state.gps_present = relay.gps_present

    relay.start()
    LOG.info("TeenAstroEfinder starting on %s", port)
    try:
        # keep gps_present flag fresh while the link loop runs
        import threading

        def watch_gps() -> None:
            import time

            while True:
                gps_present_pump()
                time.sleep(1.0)

        threading.Thread(target=watch_gps, daemon=True).start()
        link.run_forever()
    except KeyboardInterrupt:
        LOG.info("stopped")
    finally:
        relay.stop()
        link.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
