#
# rename_shc.py
# Append release version and board definition to firmware executable.
# For ESP32-S3, also build a single merged flash image for TeenAstroUploader.
#
Import("env")
import os
from os.path import join, isfile

try:
    import configparser
except ImportError:
    import ConfigParser as configparser

config = configparser.ConfigParser()
config.read("platformio.ini")

env.Replace(PROGNAME="TeenAstroSHC_%s_%s" % (config.get("env", "custom_option1"), env.get("PIOENV")))


def _merge_esp32_image(source, target, env):
    """Create a 0x0-based merged .bin that TeenAstroUploader can flash in one shot."""
    platform = env.get("PIOPLATFORM", "")
    if "espressif32" not in platform:
        return

    build_dir = env.subst("$BUILD_DIR")
    progname = env.subst("${PROGNAME}")
    app_bin = join(build_dir, progname + ".bin")
    bootloader = join(build_dir, "bootloader.bin")
    partitions = join(build_dir, "partitions.bin")
    boot_app0 = join(
        env.PioPlatform().get_package_dir("framework-arduinoespressif32"),
        "tools",
        "partitions",
        "boot_app0.bin",
    )
    merged = join(build_dir, progname + "_merged.bin")

    if not (isfile(app_bin) and isfile(bootloader) and isfile(partitions) and isfile(boot_app0)):
        print("rename_shc: skip merge (missing bootloader/partitions/app)")
        return

    # Prefer PlatformIO's Python + packaged esptool.py
    python = env.subst("$PYTHONEXE")
    esptool = join(env.PioPlatform().get_package_dir("tool-esptoolpy"), "esptool.py")
    cmd = [
        '"%s"' % python,
        '"%s"' % esptool,
        "--chip",
        "esp32s3",
        "merge_bin",
        "-o",
        '"%s"' % merged,
        "--flash_mode",
        "dio",
        "--flash_freq",
        "80m",
        "--flash_size",
        "4MB",
        "0x0",
        '"%s"' % bootloader,
        "0x8000",
        '"%s"' % partitions,
        "0xe000",
        '"%s"' % boot_app0,
        "0x10000",
        '"%s"' % app_bin,
    ]
    print("rename_shc: merging ESP32-S3 flash image -> %s" % merged)
    env.Execute(" ".join(cmd))


# Run after the application .bin exists (ESP32 only; harmless no-op otherwise)
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", env.VerboseAction(_merge_esp32_image, "Merging ESP32 flash image"))
