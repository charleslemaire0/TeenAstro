# Pre-script to build a DLL instead of an executable
# The windows_x86 platform defaults to -static and PROGSUFFIX=.exe; we must override.
Import("env")

env.Replace(
    BUILD_PROGNAME="TeenAstroAscomNative",
    BUILD_TYPE="shared",
    PROGSUFFIX=".dll",
    # Build a real DLL with exports; replace platform's -static flags
    LINKFLAGS=[
        "-shared",
        "-static-libgcc",
        "-static-libstdc++",
    ],
)
env.Append(LIBS=["ws2_32"])

def copy_dll(source, target, env):
    import shutil
    src = target[0].get_abspath()
    build_dir = os.path.dirname(src)
    dll_path = os.path.join(build_dir, "TeenAstroAscomNative.dll")
    shutil.copy(src, dll_path)
    print("Created TeenAstroAscomNative.dll")

import os
env.AddPostAction("$BUILD_DIR/$PROGNAME$PROGSUFFIX", copy_dll)
