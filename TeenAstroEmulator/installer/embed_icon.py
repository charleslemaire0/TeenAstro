"""PlatformIO pre-script: compile app_icon.rc with windres and link the
resulting .res into the final executable so the EXE carries the TeenAstro icon.

Usage in platformio.ini (emu_shc env only):
    extra_scripts = pre:installer/embed_icon.py
"""

import os
import subprocess
Import("env")

project_dir = env.subst("$PROJECT_DIR")  # TeenAstroEmulator/
installer_dir = os.path.join(project_dir, "installer")
rc_file = os.path.join(installer_dir, "app_icon.rc")
build_dir = env.subst("$BUILD_DIR")
res_file = os.path.join(build_dir, "app_icon.res")

toolchain_prefix = ""
for p in env.get("ENV", {}).get("PATH", "").split(os.pathsep):
    candidate = os.path.join(p, "windres.exe")
    if os.path.isfile(candidate):
        toolchain_prefix = ""
        break
else:
    gcc = env.subst("$CC")
    if gcc and gcc.endswith("gcc.exe"):
        toolchain_prefix = gcc.replace("gcc.exe", "")

windres = toolchain_prefix + "windres"

os.makedirs(build_dir, exist_ok=True)

print(f"  [embed_icon] {rc_file} -> {res_file}")
subprocess.check_call(
    [windres, rc_file, "-O", "coff", "-o", res_file],
    cwd=installer_dir,
)

env.Append(LINKFLAGS=[res_file])
