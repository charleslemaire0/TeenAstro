"""Ask the Cedar detector for star centroids on one photo.

Cedar listens on localhost:50051. The photo is turned to the same orientation
as the star picker, then sent as grayscale pixels. Centroids come back in that
frame, brightest first.

    python tool/cedar_extract.py photo.stars.json sigma binning max_stars

Prints one JSON object on stdout. Logs stay on stderr.
"""

import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
SERVER = HERE / "cedar" / "cedar-detect-server.exe"
sys.path.insert(0, str(HERE / "cedar"))


def _cedar_ready(grpc, options, timeout: float) -> bool:
    try:
        with grpc.insecure_channel("localhost:50051", options=options) as channel:
            grpc.channel_ready_future(channel).result(timeout=timeout)
        return True
    except grpc.FutureTimeoutError:
        return False


def _start_cedar(grpc, options) -> bool:
    if not SERVER.exists():
        return False
    subprocess.Popen(
        [str(SERVER)],
        cwd=str(SERVER.parent),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        creationflags=0x08000000,
    )
    for _ in range(40):
        if _cedar_ready(grpc, options, 0.25):
            return True
    return False


def fail(reason: str) -> None:
    json.dump({"ok": False, "reason": reason}, sys.stdout)
    sys.stdout.write("\n")


def load_gray(photo: Path, width: int, height: int):
    from PIL import Image, ImageOps

    raw = Image.open(photo)
    turned = ImageOps.exif_transpose(raw)
    if turned.size == (width, height):
        image = turned
    elif raw.size == (width, height):
        image = raw
    else:
        raise RuntimeError(
            f"The photo is {raw.size[0]}×{raw.size[1]} and the picker uses {width}×{height}."
        )
    return image.convert("L")


def main() -> int:
    if len(sys.argv) != 5:
        fail("Cedar needs the photo, sigma, binning, and a star limit.")
        return 2
    sidecar = Path(sys.argv[1])
    sigma = float(sys.argv[2])
    binning = int(sys.argv[3])
    limit = int(sys.argv[4])
    if binning not in (1, 2, 4):
        fail("Cedar binning is 1, 2, or 4.")
        return 2
    try:
        side = json.loads(sidecar.read_text(encoding="utf-8"))
        photo = Path(side["photo"])
        gray = load_gray(photo, int(side["width"]), int(side["height"]))
    except Exception as error:
        fail(str(error))
        return 1

    try:
        import grpc

        import cedar_detect_pb2
        import cedar_detect_pb2_grpc
    except ImportError as error:
        fail(f"Cedar's client is not installed ({error}).")
        return 1

    request = cedar_detect_pb2.CentroidsRequest(
        input_image=cedar_detect_pb2.Image(
            width=gray.size[0],
            height=gray.size[1],
            image_data=gray.tobytes(),
        ),
        sigma=sigma,
        use_binned_for_star_candidates=binning != 1,
        return_binned=False,
    )
    if binning != 1:
        request.binning = binning

    options = [
        ("grpc.max_send_message_length", 64 * 1024 * 1024),
        ("grpc.max_receive_message_length", 64 * 1024 * 1024),
    ]
    if not _cedar_ready(grpc, options, 0.4) and not _start_cedar(grpc, options):
        fail("Cedar is not running on localhost port 50051.")
        return 1
    try:
        with grpc.insecure_channel("localhost:50051", options=options) as channel:
            grpc.channel_ready_future(channel).result(timeout=5)
            stub = cedar_detect_pb2_grpc.CedarDetectStub(channel)
            result = stub.ExtractCentroids(request, timeout=60)
    except grpc.RpcError as error:
        detail = error.details() or error.code().name
        fail(f"Cedar refused the photo ({detail}).")
        return 1

    stars = []
    for star in result.star_candidates:
        pos = star.centroid_position
        if pos is None:
            continue
        stars.append({"x": pos.x, "y": pos.y, "brightness": star.brightness})
        if len(stars) >= limit:
            break
    json.dump(
        {
            "ok": True,
            "count": len(stars),
            "found": len(result.star_candidates),
            "noise": result.noise_estimate,
            "stars": stars,
        },
        sys.stdout,
    )
    sys.stdout.write("\n")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        fail(str(error))
        raise SystemExit(1)
