"""Drive the SHC emulator and save the OLED area of each screen."""
import ctypes
import subprocess
import time
from ctypes import wintypes
from pathlib import Path

from PIL import Image, ImageGrab

ROOT = Path(__file__).resolve().parents[3]
EMU = ROOT / "TeenAstroEmulator" / ".pio" / "build" / "emu"
OUT = Path(__file__).resolve().parent / "screens"
OUT.mkdir(parents=True, exist_ok=True)

user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32
user32.SetProcessDPIAware()
user32.SetWindowPos.argtypes = [
    wintypes.HWND, wintypes.HWND, ctypes.c_int, ctypes.c_int,
    ctypes.c_int, ctypes.c_int, wintypes.UINT,
]
user32.SetWindowPos.restype = wintypes.BOOL
user32.ShowWindow.argtypes = [wintypes.HWND, ctypes.c_int]
user32.SetForegroundWindow.argtypes = [wintypes.HWND]
user32.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
user32.GetWindowThreadProcessId.restype = wintypes.DWORD
user32.AttachThreadInput.argtypes = [wintypes.DWORD, wintypes.DWORD, wintypes.BOOL]
user32.BringWindowToTop.argtypes = [wintypes.HWND]

class RECT(ctypes.Structure):
    _fields_ = [("l", ctypes.c_long), ("t", ctypes.c_long),
                ("r", ctypes.c_long), ("b", ctypes.c_long)]

def find_hwnd():
    hwnd = user32.FindWindowW(None, "TeenAstro SHC Emulator")
    return hwnd or None

def client_oled_box(hwnd):
    rect = RECT()
    user32.GetClientRect(hwnd, ctypes.byref(rect))
    pt = wintypes.POINT(0, 0)
    user32.ClientToScreen(hwnd, ctypes.byref(pt))
    # OLED is the top 128*4 pixels of the client area.
    return (pt.x, pt.y, pt.x + 128 * 4, pt.y + 64 * 4)

REQ = EMU / "shot.req"
FRAME = EMU / "oled_frame.bmp"

def grab_frame():
    """Ask the emulator to write the OLED framebuffer. Independent of the desktop."""
    if REQ.exists():
        REQ.unlink()
    REQ.write_text("1", encoding="ascii")
    for _ in range(80):
        if FRAME.exists() and not REQ.exists():
            im = Image.open(FRAME)
            im.load()
            return im.copy()
        time.sleep(0.05)
    raise SystemExit("emulator did not write a frame")

def shot(hwnd, name):
    im = grab_frame()
    im.save(OUT / f"{name}.png")
    print("saved", name, hash(im.tobytes()[::97]), flush=True)

def release_stuck_keys():
    # Earlier captures used keybd_event. A lost key-up keeps Shift or an arrow
    # down and the emulator opens a menu as soon as it is focused.
    for vk in (0x20, 0x25, 0x26, 0x27, 0x28, 0x31, 0x33, 0x12):
        user32.keybd_event(vk, 0, 2, 0)

user32.FindWindowW.restype = wintypes.HWND

# Button order matches TeenAstroPad: Shift, N, S, E, W, F, f.
INJECT = EMU / "pad_inject.txt"
IDLE = [0, 0, 0, 0, 0, 0, 0]
B_SHIFT, B_NORTH, B_SOUTH, B_EAST, B_WEST, B_F, B_f = range(7)

def pad_cmd(text, after=0.45):
    """One-shot command. The emulator deletes the file once it is consumed."""
    INJECT.write_text(text + "\n", encoding="ascii")
    for _ in range(80):
        if not INJECT.exists():
            break
        time.sleep(0.05)
    else:
        print("command not consumed:", text, flush=True)
        raise SystemExit(f"emulator did not read {text}")
    time.sleep(after)

def tap_btn(index, after=0.45):
    pad_cmd(f"click {index}", after=after)

def open_menu(index):
    pad_cmd(f"menu {index}", after=0.55)

def main():
    release_stuck_keys()
    if INJECT.exists():
        INJECT.unlink()
    # A blank EEPROM is 0xFF, and the firmware treats that as visitor mode.
    # Force administrator so Telescope Settings shows the full list.
    eeprom = EMU / "teenastro_shc_eeprom.bin"
    if eeprom.exists():
        data = bytearray(eeprom.read_bytes())
    else:
        data = bytearray(b"\xFF" * 4096)
    if len(data) < 4096:
        data.extend(b"\xFF" * (4096 - len(data)))
    data[26] = 0  # EEPROM_VISITOR
    eeprom.write_bytes(data)
    subprocess.run(["taskkill", "/IM", "shc_emu.exe", "/F"], capture_output=True)
    subprocess.run(["taskkill", "/IM", "mainunit_emu.exe", "/F"], capture_output=True)
    time.sleep(0.6)
    mu = subprocess.Popen(
        [str(EMU / "mainunit_emu.exe")],
        cwd=str(EMU),
        creationflags=subprocess.CREATE_NEW_CONSOLE,
    )
    time.sleep(2.0)
    proc = subprocess.Popen(
        [str(EMU / "shc_emu.exe")],
        cwd=str(EMU),
        creationflags=subprocess.CREATE_NEW_CONSOLE,
    )
    # Frame dumps do not need the window in front. Grab as soon as the OLED is drawn
    # so the startup logo is not missed.
    last = None
    boot_n = 0
    t0 = time.time()
    while time.time() - t0 < 16:
        try:
            im = grab_frame()
        except SystemExit:
            time.sleep(0.2)
            continue
        sig = im.tobytes()
        if sig != last:
            im.save(OUT / f"boot_{boot_n:02d}.png")
            print("boot", boot_n, flush=True)
            boot_n += 1
            last = sig
        time.sleep(0.05)

    def page_sig(im):
        return im.crop((0, 70, 180, 250)).tobytes()

    def shot_changed(name, prev_sig):
        im = grab_frame()
        if prev_sig is not None and page_sig(im) == prev_sig:
            tap_btn(B_SHIFT, after=0.8)
            im = grab_frame()
        im.save(OUT / f"{name}.png")
        print("saved", name, flush=True)
        return page_sig(im)

    pages = ["radec", "hadec", "altaz", "push", "time", "axis_steps", "axis_deg", "focuser"]
    prev = None
    for i, name in enumerate(pages):
        prev = shot_changed(name, prev if i else None)
        if i + 1 < len(pages):
            tap_btn(B_SHIFT, after=0.8)

    open_menu(B_EAST)
    shot(None, "menu_action")
    tap_btn(B_EAST, after=0.6)

    open_menu(B_NORTH)
    shot(None, "menu_speed")
    tap_btn(B_EAST, after=0.6)

    open_menu(B_SOUTH)
    shot(None, "menu_display")
    tap_btn(B_EAST, after=0.6)

    open_menu(B_WEST)
    shot(None, "menu_settings")
    tap_btn(B_WEST, after=0.6)
    shot(None, "menu_shc")
    tap_btn(B_EAST, after=0.5)
    tap_btn(B_EAST, after=0.5)

    if INJECT.exists():
        INJECT.unlink()

    proc.terminate()
    mu.terminate()
    time.sleep(0.4)
    subprocess.run(["taskkill", "/IM", "mainunit_emu.exe", "/F"], capture_output=True)
    subprocess.run(["taskkill", "/IM", "shc_emu.exe", "/F"], capture_output=True)

if __name__ == "__main__":
    main()
