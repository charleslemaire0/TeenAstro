"""PlatformIO pre-script: ensure the bundled MinGW toolchain is on PATH.

Used by the verify_alpaca env which only needs g++ + ld + winsock — not
SDL2 or any of the other SDL-related plumbing the emu_mainunit/emu_shc
envs rely on.
"""
import os
Import("env")

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
