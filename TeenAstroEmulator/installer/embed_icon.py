"""PlatformIO pre-script: compile app_icon.rc with windres and link the
resulting .res into the final executable so the EXE carries the TeenAstro icon.

Usage in platformio.ini (emu_shc env only):
    extra_scripts = pre:installer/embed_icon.py
"""

import os
import shutil
import subprocess
Import("env")

project_dir = env.subst("$PROJECT_DIR")  # TeenAstroEmulator/
installer_dir = os.path.join(project_dir, "installer")
rc_file = os.path.join(installer_dir, "app_icon.rc")
build_dir = env.subst("$BUILD_DIR")
res_file = os.path.join(build_dir, "app_icon.res")

windres = None

gcc = env.subst("$CC")
if gcc:
    gcc_path = os.path.abspath(gcc) if not os.path.isabs(gcc) else gcc
    candidate = os.path.join(os.path.dirname(gcc_path), "windres.exe")
    if os.path.isfile(candidate):
        windres = candidate

if not windres:
    windres = shutil.which("windres", path=env.get("ENV", {}).get("PATH"))

if not windres:
    windres = shutil.which("windres")

if not windres:
    print("  [embed_icon] windres not found: skipping icon (EXE will have default icon)")
    print("  [embed_icon] To embed the icon, install MinGW with windres or use toolchain-gccmingw32.")
else:
    os.makedirs(build_dir, exist_ok=True)
    print(f"  [embed_icon] {rc_file} -> {res_file}")
    subprocess.check_call(
        [windres, rc_file, "-O", "coff", "-o", res_file],
        cwd=installer_dir,
    )
    env.Append(LINKFLAGS=[res_file])
