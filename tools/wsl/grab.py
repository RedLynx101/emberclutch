"""The trailer's frames at the emulator's internal resolution (docs/plan/trailer.md), read off the virtual display.

    python3 grab.py --display :N --film <the run's sdmc film folder> --size WxH --out <Windows path prefix>
                    --ffmpeg <ffmpeg.exe, a WSL path>

tools/wsl/autotest.sh --grab starts it beside the emulator. The game, filming (autotest `film start`), finishes a
frame, lets it reach the screens, writes film/ready ("<reel> <frame>") and waits; this reads the whole display,
streams it into ffmpeg (<out><reel>.mkv), and answers with film/ack (the same "<reel> <frame>"). film/grab.end
closes the last reel. Only libX11 (ctypes): nothing to install.

The display is exactly the layout's size at the internal resolution (2880x960 for side by side at 4x), and
Azahar's window is made to fill it: with no window manager its fullscreen request does nothing and it opens
at two thirds of the display, so this moves and sizes it, and checks again before every frame. Then every
3DS pixel is a square of the internal resolution's, drawn 1:1. Leaving, this takes film/grab.on away, so the
game stops waiting for it.
"""
import argparse
import ctypes
import os
import subprocess
import sys
import time


class XImage(ctypes.Structure):
    _fields_ = [("width", ctypes.c_int), ("height", ctypes.c_int), ("xoffset", ctypes.c_int), ("format", ctypes.c_int),
                ("data", ctypes.POINTER(ctypes.c_ubyte)), ("byte_order", ctypes.c_int), ("bitmap_unit", ctypes.c_int),
                ("bitmap_bit_order", ctypes.c_int), ("bitmap_pad", ctypes.c_int), ("depth", ctypes.c_int),
                ("bytes_per_line", ctypes.c_int), ("bits_per_pixel", ctypes.c_int)]


class XWindowAttributes(ctypes.Structure):
    _fields_ = [("x", ctypes.c_int), ("y", ctypes.c_int), ("width", ctypes.c_int), ("height", ctypes.c_int),
                ("border_width", ctypes.c_int), ("depth", ctypes.c_int), ("visual", ctypes.c_void_p),
                ("root", ctypes.c_ulong), ("class_", ctypes.c_int), ("bit_gravity", ctypes.c_int),
                ("win_gravity", ctypes.c_int), ("backing_store", ctypes.c_int), ("backing_planes", ctypes.c_ulong),
                ("backing_pixel", ctypes.c_ulong), ("save_under", ctypes.c_int), ("colormap", ctypes.c_ulong),
                ("map_installed", ctypes.c_int), ("map_state", ctypes.c_int), ("all_event_masks", ctypes.c_long),
                ("your_event_mask", ctypes.c_long), ("do_not_propagate_mask", ctypes.c_long),
                ("override_redirect", ctypes.c_int), ("screen", ctypes.c_void_p)]


def xlib():
    x = ctypes.cdll.LoadLibrary("libX11.so.6")
    x.XOpenDisplay.restype = ctypes.c_void_p
    x.XDefaultRootWindow.restype = ctypes.c_ulong
    x.XDefaultRootWindow.argtypes = [ctypes.c_void_p]
    x.XGetImage.restype = ctypes.POINTER(XImage)
    x.XGetImage.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int, ctypes.c_int, ctypes.c_uint, ctypes.c_uint,
                            ctypes.c_ulong, ctypes.c_int]
    # (one image, refilled each frame: XGetSubImage into it; XDestroyImage is only a C macro)
    x.XGetSubImage.restype = ctypes.POINTER(XImage)
    x.XGetSubImage.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int, ctypes.c_int, ctypes.c_uint, ctypes.c_uint,
                               ctypes.c_ulong, ctypes.c_int, ctypes.POINTER(XImage), ctypes.c_int, ctypes.c_int]
    x.XQueryTree.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.POINTER(ctypes.c_ulong), ctypes.POINTER(ctypes.c_ulong),
                             ctypes.POINTER(ctypes.POINTER(ctypes.c_ulong)), ctypes.POINTER(ctypes.c_uint)]
    x.XGetWindowAttributes.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.POINTER(XWindowAttributes)]
    x.XFetchName.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.POINTER(ctypes.c_void_p)]
    x.XFree.argtypes = [ctypes.c_void_p]
    x.XMoveResizeWindow.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int, ctypes.c_int, ctypes.c_uint, ctypes.c_uint]
    x.XSync.argtypes = [ctypes.c_void_p, ctypes.c_int]
    return x


def emulator_window(x, display, root):
    """Azahar's main window (a mapped child of the root named "Azahar ..."), or 0."""
    r, p, kids, n = ctypes.c_ulong(), ctypes.c_ulong(), ctypes.POINTER(ctypes.c_ulong)(), ctypes.c_uint()
    if not x.XQueryTree(display, root, ctypes.byref(r), ctypes.byref(p), ctypes.byref(kids), ctypes.byref(n)):
        return 0
    found = 0
    for i in range(n.value):
        a = XWindowAttributes()
        x.XGetWindowAttributes(display, kids[i], ctypes.byref(a))
        name = ctypes.c_void_p()
        if x.XFetchName(display, kids[i], ctypes.byref(name)) and name.value:
            if a.map_state == 2 and ctypes.string_at(name.value).startswith(b"Azahar"):
                found = found or kids[i]
            x.XFree(name)
    if kids:
        x.XFree(kids)
    return found


def fit(x, display, root, w, h):
    """Makes Azahar's window fill the display; True if it had to (the next frame then needs a moment)."""
    win = emulator_window(x, display, root)
    if not win:
        return False
    a = XWindowAttributes()
    x.XGetWindowAttributes(display, win, ctypes.byref(a))
    if (a.x, a.y, a.width, a.height) == (0, 0, w, h):
        return False
    x.XMoveResizeWindow(display, win, 0, 0, w, h)
    x.XSync(display, 0)
    print(f"[grab] window {a.width}x{a.height} at {a.x},{a.y}: made {w}x{h} at 0,0", flush=True)
    return True


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--display", required=True)
    ap.add_argument("--film", required=True)
    ap.add_argument("--size", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--ffmpeg", required=True)
    a = ap.parse_args()
    w, h = (int(v) for v in a.size.split("x"))
    on, ready, ack, end = (os.path.join(a.film, n) for n in ("grab.on", "ready", "ack", "grab.end"))
    try:
        run(a, w, h, ready, ack, end)
    finally:
        try:
            os.remove(on)  # (the game stops waiting)
        except OSError:
            pass


def run(a, w, h, ready, ack, end) -> None:
    x = xlib()
    display = None
    for _ in range(100):  # (the server may still be starting)
        display = x.XOpenDisplay(a.display.encode())
        if display:
            break
        time.sleep(0.1)
    if not display:
        sys.exit(f"[grab] no display {a.display}")
    root = x.XDefaultRootWindow(display)

    reel, enc, last, frames, t0, image, next_fit = None, None, None, 0, time.time(), None, 0.0

    def close():
        nonlocal enc
        if enc:
            enc.stdin.close()
            enc.wait()
            print(f"[grab] {reel}: {frames} frames", flush=True)
            enc = None

    while True:
        try:
            with open(ready) as f:
                got = f.read().split()
        except OSError:
            got = []
        if len(got) == 2 and (got[0], got[1]) != last:
            name, index = got[0], got[1]
            if fit(x, display, root, w, h):
                time.sleep(0.5)  # (the emulator lays its screens out again and draws them at the new size)
            if name != reel:
                close()
                reel, frames = name, 0
                # (full colour, nearly lossless: an intermediate. Frames come a few a second, so the fastest
                # preset on four threads: x264's defaults hold dozens of frames, gigabytes at this size)
                enc = subprocess.Popen([a.ffmpeg, "-loglevel", "error", "-y", "-f", "rawvideo", "-pix_fmt", "bgr0", "-s", f"{w}x{h}",
                                        "-r", "60", "-i", "-", "-c:v", "libx264", "-preset", "ultrafast", "-threads", "4",
                                        "-crf", "8", "-pix_fmt", "yuv444p", a.out + name + ".mkv"], stdin=subprocess.PIPE)
            if image is None:
                image = x.XGetImage(display, root, 0, 0, w, h, 0xFFFFFFFF, 2)
            else:
                x.XGetSubImage(display, root, 0, 0, w, h, 0xFFFFFFFF, 2, image, 0, 0)
            if image:
                im = image.contents
                data = ctypes.string_at(im.data, im.bytes_per_line * h)
                if im.bytes_per_line != w * 4:  # (rows padded: take each row's pixels)
                    data = b"".join(data[y * im.bytes_per_line:y * im.bytes_per_line + w * 4] for y in range(h))
                enc.stdin.write(data)
                frames += 1
            with open(ack + ".tmp", "w") as f:
                f.write(f"{name} {index}\n")  # (the game's own line back: this reel, this frame)
            os.replace(ack + ".tmp", ack)
            last = (name, index)
            continue
        if os.path.exists(end):
            close()
            print(f"[grab] done in {time.time() - t0:.0f} s", flush=True)
            return
        if reel is None and time.time() >= next_fit:  # (early too, twice a second, while the game is starting)
            fit(x, display, root, w, h)
            next_fit = time.time() + 0.5
        time.sleep(0.002)


if __name__ == "__main__":
    main()
