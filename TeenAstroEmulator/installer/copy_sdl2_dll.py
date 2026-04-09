"""Post-link: copy SDL2.dll next to mainunit_emu.exe / shc_emu.exe (Windows runtime)."""
import os
import shutil

Import("env")


def copy_sdl2_dll(source, target, env):
    if env.get("PIOENV", "") not in ("emu_mainunit", "emu_shc"):
        return
    sdl2_root = os.environ.get("SDL2_DIR", "C:/SDL2")
    dll = os.path.join(sdl2_root, "bin", "SDL2.dll")
    if not os.path.isfile(dll):
        print("  [copy_sdl2_dll] WARNING: %s not found — install SDL2-devel to %%SDL2_DIR%% or C:/SDL2" % dll)
        return
    prog = env.subst("$PROGPATH")
    dest_dir = os.path.dirname(os.path.abspath(prog))
    dest = os.path.join(dest_dir, "SDL2.dll")
    shutil.copy2(dll, dest)
    print("  [copy_sdl2_dll] Copied SDL2.dll -> %s" % dest)


env.AddPostAction("$PROGPATH", copy_sdl2_dll)
