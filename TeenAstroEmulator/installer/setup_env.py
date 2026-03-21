"""PlatformIO pre-script: ensure build tools are on PATH.

1. Adds toolchain-gccmingw32/bin to PATH so g++ and windres are found.
2. Downloads SDL2 development libraries to C:/SDL2 if missing.

Runs automatically as a pre-script for both emulator environments.
"""
import os
import sys
Import("env")

# --- 1. MinGW toolchain on PATH ---
try:
    pio_home = env.subst("$PIOHOME_DIR")
except Exception:
    pio_home = ""
if not pio_home or not os.path.isabs(pio_home):
    pio_home = os.path.join(os.path.expanduser("~"), ".platformio")
pio_home = os.path.abspath(pio_home)
toolchain_bin = os.path.join(pio_home, "packages", "toolchain-gccmingw32", "bin")

if os.path.isfile(os.path.join(toolchain_bin, "g++.exe")):
    build_env = env.get("ENV", {})
    if toolchain_bin not in build_env.get("PATH", ""):
        build_env["PATH"] = toolchain_bin + os.pathsep + build_env.get("PATH", "")
        env["ENV"] = build_env
    if toolchain_bin not in os.environ.get("PATH", ""):
        os.environ["PATH"] = toolchain_bin + os.pathsep + os.environ.get("PATH", "")

# --- 2. SDL2 at C:/SDL2 ---
sdl2_dir = os.environ.get("SDL2_DIR", "C:/SDL2")
sdl2_header = os.path.join(sdl2_dir, "include", "SDL2", "SDL.h")

if not os.path.isfile(sdl2_header):
    print("  [setup_env] SDL2 not found at %s — downloading..." % sdl2_dir)
    import zipfile
    if sys.version_info >= (3,):
        from urllib.request import urlretrieve
    else:
        from urllib import urlretrieve

    url = "https://github.com/libsdl-org/SDL/releases/download/release-2.30.12/SDL2-devel-2.30.12-mingw.zip"
    zip_path = os.path.join(os.environ.get("TEMP", "."), "SDL2-devel-mingw.zip")
    try:
        print("  [setup_env] Downloading %s ..." % url)
        urlretrieve(url, zip_path)
        print("  [setup_env] Extracting to %s ..." % sdl2_dir)
        with zipfile.ZipFile(zip_path, 'r') as zf:
            # The zip contains SDL2-2.30.12/i686-w64-mingw32/{include,lib,bin}
            # Extract the 32-bit variant to sdl2_dir
            prefix_32 = None
            prefix_64 = None
            for name in zf.namelist():
                if "/i686-w64-mingw32/" in name:
                    prefix_32 = name.split("/i686-w64-mingw32/")[0] + "/i686-w64-mingw32/"
                    break
            if not prefix_32:
                for name in zf.namelist():
                    if "/x86_64-w64-mingw32/" in name:
                        prefix_64 = name.split("/x86_64-w64-mingw32/")[0] + "/x86_64-w64-mingw32/"
                        break
            prefix = prefix_32 or prefix_64
            if prefix:
                for info in zf.infolist():
                    if info.filename.startswith(prefix) and not info.is_dir():
                        rel = info.filename[len(prefix):]
                        if rel:
                            dest = os.path.join(sdl2_dir, rel.replace("/", os.sep))
                            os.makedirs(os.path.dirname(dest), exist_ok=True)
                            with zf.open(info) as src, open(dest, "wb") as dst:
                                dst.write(src.read())
        os.remove(zip_path)
        print("  [setup_env] SDL2 installed to %s" % sdl2_dir)
    except Exception as e:
        print("  [setup_env] Auto-download failed: %s" % e)
        print("  [setup_env] Manual fix: download SDL2-devel-2.30.12-mingw.zip from")
        print("              https://github.com/libsdl-org/SDL/releases/tag/release-2.30.12")
        print("              Extract the i686-w64-mingw32 folder contents to C:\\SDL2")
        env.Exit(1)
else:
    pass  # SDL2 already present

# --- 3. Shared build dir and output exe name ---
# Both emu_mainunit and emu_shc build into .pio/build/emu with distinct exe names.
pioenv = env.get("PIOENV", "")
build_emu = os.path.join(env.subst("$PROJECT_DIR"), ".pio", "build", "emu")
env.Replace(BUILD_DIR=build_emu)
if pioenv == "emu_mainunit":
    env.Replace(PROGNAME="mainunit_emu")
elif pioenv == "emu_shc":
    env.Replace(PROGNAME="shc_emu")
