"""The trailer's edit (docs/plan/trailer.md, V4-V5): the 6x reels (tools/film/capture.ps1) cut to the game's
music, Lily's narration (tools/film/narration.py), the game's own sounds from each reel's cue sheet, and the
titles and end card in the game's look, into build/film/edit/trailer.mp4 (1920x1080, 60 fps, -14 LUFS).

    py -3.12 tools/film/edit.py [--preview] [--from S] [--to S] [--video-only | --audio-only] [--check]
                                [--sheet] [--out NAME]

--preview renders at half size; --from/--to a stretch of it; --check prints the timeline as resolved (each
shot's source frames, its cue anchor) without rendering; --sheet a contact sheet of the cut (a frame a
second). The timeline below is the whole edit: shots on the music's bar lines, the narration on its
beats, captions, the music's three movements and a few sounds of the edit's own.
"""
import argparse
import json
import math
import os
import subprocess
import sys
from dataclasses import dataclass, field
from fractions import Fraction

import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont
from scipy import signal

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FILM = os.path.join(ROOT, "build", "film")
GRAB = os.path.join(FILM, "grab")
RUNS = os.path.join(FILM, "runs")
NARR = os.path.join(FILM, "narration")
OUT = os.path.join(FILM, "edit")
MUSIC = os.path.join(ROOT, "assets", "audio", "music", "source")
SFX = os.path.join(ROOT, "romfs", "sfx")
VOICE = os.path.join(ROOT, "romfs", "voice")
FONT_TITLE = os.path.join(ROOT, "assets", "fonts", "cinzel-decorative", "CinzelDecorative-Bold.ttf")
FONT_BODY = os.path.join(ROOT, "assets", "fonts", "nunito", "Nunito-SemiBold.ttf")
FPS = 60
RATE = 48000

# The palette (src/app/theme.hpp)
GOLD = (245, 196, 81)
SHELL = (255, 243, 220)
EMBER = (232, 102, 43)
PLUM = (52, 35, 63)
DUSK = (94, 68, 102)
TRACK = (28, 17, 34)


# ---------------------------------------------------------------------------------------------- the music's grid
# Bar lines found by tools' music map (onset phase at the loops.json tempo; the strings' entry and the
# drums' land exactly on them). title-theme: 88.99 BPM from 1.7635 s; skyreach: 129.89 BPM from 0.215 s.
TT_BAR, SK_BAR = 2.69693, 1.84772
# title-theme starts 0.621 s into itself (the quiet before its first note): so its bar 4 lands on the egg's burst
# (the hatch reel's frame 179, from its cue sheet; the egg's reel is too short to start the hatch any later).
TT0 = -0.621


def tt_track(bar):
    """title-theme's bar line in the track itself."""
    return 1.7635 + (bar - 1) * TT_BAR


def tt(bar):
    """title-theme's bar line, on the timeline."""
    return TT0 + tt_track(bar)


SK_AT = tt(13) - 0.215  # skyreach's 0 s on the timeline: its first downbeat on title-theme's bar 13


def sk(bar):
    return SK_AT + 0.215 + (bar - 1) * SK_BAR


TE_SRC = 182.46          # title-theme's last section (its bar 68), the close
TE_AT = sk(17) + TT_BAR  # entering a bar after skyreach stops: the lanterns' hush first


def te(bar):
    """The close's bar lines (bar 68 is where it enters)."""
    return TE_AT + (bar - 68) * TT_BAR


# ---------------------------------------------------------------------------------------------- the timeline
@dataclass
class Shot:
    reel: str
    t: float                    # timeline start (s); it runs to the next shot's start
    src: int = 2                # first source frame (the first frame or two can still carry the interface)
    layout: str = "full"        # full: the top screen, 16:9, edge to edge; ds: both screens, as on a 3DS
    y: float = 0.5              # full: where the 16:9 crop sits in the top screen's height (0 top, 1 bottom)
    kb: tuple = None            # full: a slow push (zoom from, zoom to), about the crop's centre
    fade: float = 0.0           # a crossfade from the shot before (s)
    dip: str = ""               # "white" / "black": a dip through that colour at its start
    anchor: tuple = None        # (cue file, timeline s, fallback frame): src chosen so that sound lands then
    sound: float = 0.0          # its cue sheet's sounds, trimmed (dB); None: silent
    beds: float = -9.0          # its cue sheet's beds (the ambience), trimmed (dB)
    ui: bool = False            # keep the interface's own sounds (taps): only where tapping is what's shown
    mute: tuple = ()            # its cue sheet's sounds left out (file name starts)
    lift: float = 0.0           # the shadows raised a little (a dark night shot, after the crop and the vignette)
    end: float = None           # (set from the next shot)


OPEN = 0.8  # black, then the egg fading up
SKIP_SOUNDS = ("ui-", "mail-")  # the menus' blips and the post: not the game's world

SHOTS = [
    # The egg by firelight; the hatching (its burst on title-theme's bar 4); the title on bar 5, the strings' entry.
    # (a slow push toward the egg; then closer as it cracks, the knocks rising, snapping wide on the burst's flash)
    Shot("s01_egg", OPEN, src=2, y=0.62, kb=[(0.0, 1.0, 0.5, 0.56), (5.9, 1.14, 0.5, 0.575)]),
    Shot("s02_hatch", 6.3, y=0.62, anchor=("egg-hatch", tt(4), 179), fade=0.45,
         kb=[(0.0, 1.16, 0.5, 0.575), (tt(4) - 6.3 - 0.02, 1.42, 0.5, 0.58), (tt(4) - 6.3 + 0.02, 1.0, 0.5, 0.55),
             (8.3, 1.07, 0.5, 0.57)]),
    # Care, on the bar lines: pet, feed, then brush and bathe a half bar each (both screens: the stylus's work).
    Shot("s03_pet", tt(6), src=24, layout="ds"),
    Shot("s04a_feed", tt(7), src=20, layout="ds"),
    Shot("s04b_brush", tt(8), src=36, layout="ds"),
    Shot("s04c_bath", tt(8) + TT_BAR / 2, src=40, layout="ds"),
    # Growing: day, evening, night, morning, dissolving a half bar apart.
    Shot("s05a_day", tt(9), src=6, y=0.55),
    Shot("s05b_evening", tt(9) + TT_BAR / 2, src=6, y=0.55, fade=0.3),
    Shot("s05c_night", tt(10), src=6, y=0.55, fade=0.3),
    Shot("s05d_morning", tt(10) + TT_BAR / 2, src=10, y=0.55, fade=0.3),
    # The door opens: through a white bloom into the valley, the waterfall's crane.
    Shot("s06_valley", tt(11), src=8, y=0.45, dip="white"),
    # skyreach's intro: the Market on its lead; the take-off's lift on skyreach's drums (its bar 5).
    Shot("s07_market", tt(13), src=2, y=0.5, kb=[(0.0, 1.0, 0.5, 0.5), (6.3, 1.1, 0.56, 0.6)]),  # (toward you and it)
    # (cut on the action: the hop on from the side, frames 0-33, then the rider's view once the game's camera has
    # swung round, from frame 44: its frames 34-43 are the camera on its way, empty grass then the back of a head)
    Shot("s08_takeoff", sk(5) - (75 - 44 + 34) / FPS, src=0, y=0.4),
    Shot("s08_takeoff", sk(5) - (75 - 44) / FPS, anchor=("takeoff", sk(5), 75), y=0.4),
    Shot("s09_flight", sk(6), src=60, y=1.0),  # (low: an island's tip hangs into the top)
    Shot("s09b_isles", sk(8), src=24, y=0.75),
    # The montage: battle, show, the Dragondex, rings and the catch on the section's last downbeat.
    Shot("s10_battle", sk(10), src=120, y=0.5),
    Shot("s11_show", sk(12), src=2, y=0.4),
    Shot("s15_dex", sk(13) + SK_BAR / 2, src=50, layout="ds", ui=True),  # (the Curlstone, then the tap: the rare Crestwing)
    Shot("s12_rings", sk(14) + SK_BAR / 2, src=60, y=0.4),
    Shot("s13_fish", sk(15) + SK_BAR / 2, src=674, y=0.5, sound=-2.0),
    # The hush, then the close: the village's lanterns, home's lantern, asleep by the hearth.
    Shot("s14_lanterns", sk(17), src=30, y=0.55, beds=-4.0, mute=("soft-snore",), lift=0.18),  # (a villager dozing: not in the hush)
    Shot("s14b_home", te(68), src=8, y=0.5, fade=0.5),
    Shot("s16_sleep", te(69), src=4, y=0.55, fade=0.6,
         kb=[(0.0, 1.0, 0.5, 0.55), (6.8, 1.16, 0.56, 0.56)]),  # (in close to it, on under the end card's fade)
]
END_AT = te(71)            # the end card (over the sleeping dragon, which runs on beneath it)
LENGTH = END_AT + 13.2     # the close's last chord (its bar 72, under the end card) rung out, and a breath of black

NARRATION = [  # (piece, timeline s): build/film/narration/<piece>_t<pick>.wav
    # (the pause of "one morning..." on the burst; "someone says hello." just before the hatchling's own first cry)
    ("n01", 1.4), ("n02a", tt(4) - 2.85), ("n02b", tt(4) + 0.57),
    ("n03a", tt(6) + 0.12), ("n03b", tt(7) + 0.12), ("n03c", tt(8) + 0.08), ("n03d", tt(8) + TT_BAR / 2 + 0.08),
    ("n04", tt(9) + 0.3),
    ("n05", tt(11) + 0.35),
    ("n06a", tt(13) + 0.4), ("n06b", tt(13) + 2.1), ("n07", tt(13) + 4.77), ("n08", sk(5) - 1.05),  # (done before the drums)
    ("n09", sk(10) + 0.2), ("n10", sk(12) + 0.15), ("n11", sk(14) + SK_BAR / 2 + 0.12), ("n12", sk(15) + SK_BAR / 2 + 0.12),
    ("n13", sk(17) + 0.45),
    ("n14a", te(69) + 0.35), ("n14b", te(69) + 2.75),
    ("n15a", END_AT + 1.4), ("n15b", END_AT + 4.4),
]

CAPTIONS = [  # (text, from, to)
    ("A valley to explore", tt(12) - 0.2, tt(13) - 0.35),
    ("Raise it. Ride it.", sk(6) + 0.25, sk(8) - 0.3),
    ("Fourteen kinds of dragon", sk(13) + SK_BAR / 2 + 0.15, sk(14) + SK_BAR / 2 - 0.2),
]
TITLE = (tt(5), tt(6) - 0.25)  # the wordmark over the hatchling

MUSIC_PLAN = [  # (track, its s, timeline s, timeline end, fade in, fade out)
    # title-theme under the egg, the care, the valley and the Market; dropping away as "And when it's grown..."
    ("title-theme", -TT0, 0.0, tt(13) + 4.62, 0.0, 0.85),
    # skyreach from its bar 4, the one-bar build (its drums swelling) under "climb on", into its full theme at the
    # lift; its quiet intro before that was too faint to carry
    ("skyreach", 5.7587 - 0.05, sk(4) - 0.05, sk(17) + 0.12, 0.3, 0.12),  # its last downbeat struck, ringing on (below)
    ("title-theme", TE_SRC, TE_AT, None, 0.6, 0.0),
]
MUSIC_DB, VOICE_DB = -1.0, -0.5  # the buses, before the whole is brought to -14 LUFS
# The edit's own ambience, from the game's beds (romfs/sfx): the free camera mutes the valley's own, so the
# waterfall's crane would otherwise be silent. (bed file, from, to, dB, fade in, fade out)
AMBIENCE = [
    ("amb-waterfall", tt(11) + 0.15, tt(13), -13.0, 0.8, 0.5),
    ("amb-meadow", tt(11) + 0.15, tt(13) + 0.3, -17.0, 0.8, 0.6),
    ("amb-village", tt(13), sk(5) - 0.6, -16.0, 0.5, 0.8),
    ("amb-night", sk(17), te(69) + 0.8, -15.0, 0.4, 1.2),
]

EXTRA_SOUNDS = [  # (romfs/sfx file, timeline s, dB): the edit's own
    ("star-shimmer", tt(5) - 0.05, -9.0),        # the title
    ("travel-whoosh", tt(11) - 0.35, -10.0),     # the door's bloom
    ("lantern-light", te(68) + 0.1, -12.0),
    ("star-shimmer", END_AT + 1.3, -12.0),       # the end card
]
CANVAS = "wide"  # 1920x1080; "tall": the vertical cut's 1080x1920


# ---------------------------------------------------------------------------------------------- the vertical cut
# About 33 s for Shorts (1080x1920), from the same reels, lines and sounds: the hatching as the hook (its burst on
# title-theme's bar 4), care, the lift on skyreach's drums, the montage a bar a shot, the end card on title-theme's
# last chord (its bar 72). The top screen whole, mid-frame; the narration as captions beneath it.
TALL_TT = tt_track(4) - 0.5  # title-theme's second at the cut's 0 s: its bar 4 at 0.5 s


def ttv(bar):
    return tt_track(bar) - TALL_TT


TALL_SK = 9.05 - 0.215 - 3 * SK_BAR  # skyreach's 0 s on this timeline: its bar 4 (the build) at 9.05 s


def skv(bar):
    return TALL_SK + 0.215 + (bar - 1) * SK_BAR


def tall_cut():
    end_at = skv(11)
    return dict(
        open=0.0,
        shots=[
            Shot("s02_hatch", 0.0, anchor=("egg-hatch", ttv(4), 179)),
            Shot("s03_pet", ttv(5), src=24, layout="ds"),
            Shot("s04a_feed", ttv(6), src=40, layout="ds"),
            Shot("s04c_bath", skv(5) - (75 - 44 + 34) / FPS - 2.3, src=2, layout="ds"),  # (its whole reel, to the hop on)
            Shot("s08_takeoff", skv(5) - (75 - 44 + 34) / FPS, src=0),
            Shot("s08_takeoff", skv(5) - (75 - 44) / FPS, anchor=("takeoff", skv(5), 75)),
            Shot("s09_flight", skv(6), src=150),
            Shot("s10_battle", skv(7), src=150),
            Shot("s11_show", skv(8), src=2),
            Shot("s12_rings", skv(9), src=80),
            Shot("s13_fish", skv(10), src=729, sound=-2.0),
        ],
        end_at=end_at,
        length=end_at + 11.3,
        narration=[("n02b", 1.0), ("n03a", ttv(5) + 0.12), ("n03b", ttv(6) + 0.12), ("n03d", skv(5) - (75 - 44 + 34) / FPS - 2.2),
                   ("n07", 8.4), ("n08", skv(5) - 1.05), ("n09", skv(7) + 0.15), ("n10", skv(8) + 0.1),
                   ("n11", skv(9) + 0.2), ("n12", skv(10) + 0.2), ("n15a", end_at + 0.9), ("n15b", end_at + 3.8)],
        captions=[("Raise it. Ride it.", skv(6) + 0.2, skv(7) - 0.25)],
        title=(0.95, ttv(5) - 0.2),
        music=[("title-theme", TALL_TT, 0.0, 9.1, 0.0, 0.6),
               ("skyreach", 5.7587 - 0.05, skv(4) - 0.05, end_at + 0.12, 0.25, 0.12),
               ("title-theme", 193.25 - 0.4, end_at + 0.22, None, 0.4, 0.0)],
        extra=[("star-shimmer", 0.9, -9.0), ("star-shimmer", end_at + 0.8, -12.0)],
    )


def use_cut(name):
    """The timeline to render: the trailer (wide) or the vertical cut (tall), whose captions are its lines."""
    global SHOTS, NARRATION, CAPTIONS, TITLE, MUSIC_PLAN, EXTRA_SOUNDS, END_AT, LENGTH, OPEN, CANVAS, AMBIENCE
    if name != "tall":
        return
    c = tall_cut()
    SHOTS, NARRATION, TITLE, MUSIC_PLAN, EXTRA_SOUNDS = c["shots"], c["narration"], c["title"], c["music"], c["extra"]
    AMBIENCE = c.get("ambience", [])
    END_AT, LENGTH, OPEN, CANVAS = c["end_at"], c["length"], c["open"], "tall"
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import narration
    words = {piece: w for _, pieces in narration.PASSAGES for piece, w in pieces}
    takes = json.load(open(os.path.join(NARR, "takes.json")))
    dur = {piece: d for p in takes.values() for piece, d in p["takes"][p["pick"] - 1]["pieces"].items()}
    caps = sorted(c["captions"] + [(words[p], at, at + dur[p] - 0.1) for p, at in NARRATION if not p.startswith("n15")],
                  key=lambda x: x[1])
    # (in one place under the screens: each one gone, its fade too, before the next fades in)
    CAPTIONS = [(text, c0, min(c1, caps[i + 1][1] - 0.4) if i + 1 < len(caps) else c1) for i, (text, c0, c1) in enumerate(caps)]


# ---------------------------------------------------------------------------------------------- reels and cues
def probe(path):
    out = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-count_packets", "-show_entries",
                          "stream=width,height,nb_read_packets", "-of", "csv=p=0", path],
                         capture_output=True, text=True, check=True).stdout.strip().split(",")
    return int(out[0]), int(out[1]), int(out[2])


REELS = {}


def reel(name):
    """(path, width, height, frames, cues) for a reel; cues: [(frame, kind, file, pitch, gain, lowpass)]."""
    if name not in REELS:
        path = os.path.join(GRAB, name + ".mkv")
        w, h, n = probe(path) if os.path.exists(path) and os.path.getsize(path) > 0 else (0, 0, 0)
        cues = []
        for d in os.listdir(RUNS):
            cf = os.path.join(RUNS, d, "film", name + ".cues")
            if os.path.exists(cf):
                for line in open(cf):
                    p = line.split()
                    if len(p) == 6:
                        cues.append((int(p[0]), p[1], p[2], float(p[3]), float(p[4]), float(p[5])))
        REELS[name] = (path, w, h, n, cues)
    return REELS[name]


def resolve(shots):
    """Each shot's end and, for an anchored one, its source in-point (the cue's frame on its beat)."""
    for i, s in enumerate(shots):
        s.end = shots[i + 1].t if i + 1 < len(shots) else END_AT + 3.0
        if s.anchor:
            name, at, fallback = s.anchor
            _, _, _, n, cues = reel(s.reel)
            frames = [c[0] for c in cues if c[1] == "sfx" and c[2].split("-")[0] == name.split("-")[0]
                      and c[2].rstrip("-0123456789") == name]
            f = frames[0] if frames else fallback
            s.src = max(0, int(round(f - (at - s.t) * FPS)))
            s.anchor = (name, at, fallback, f)


# ---------------------------------------------------------------------------------------------- the picture
class Decoder:
    """A shot's frames, in order, from its reel: already cropped and scaled where the framing holds still."""

    def __init__(self, shot, W, H, scale):
        path, w, h, n, _ = reel(shot.reel)
        self.shot, self.n, self.next, self.last = shot, n, shot.src, None
        side = w > h * 2  # both screens side by side (a two-screen script)
        top = f"crop=2400:1440:0:0," if side else ""
        tall = CANVAS == "tall"
        if shot.layout == "ds":
            tw, th, bw = (int(1080 * scale), int(648 * scale), int(864 * scale)) if tall else \
                         (int(800 * scale), int(480 * scale), int(640 * scale))
            vf = (f"[0:v]split[a][b];[a]crop=2400:1440:0:0,scale={tw}:{th}:flags=area[t];"
                  f"[b]crop=1920:1440:2400:0,scale={bw}:{th}:flags=area[d];[t][d]hstack")
            self.size = (tw + bw, th)
        elif tall:  # the vertical cut: the top screen whole, the width of the frame
            tw, th = int(1080 * scale), int(648 * scale)
            vf = f"{top}scale={tw}:{th}:flags=area"
            self.size = (tw, th)
        elif shot.kb:
            vf = top.rstrip(",") or "null"
            self.size = (2400, 1440)
        else:
            ch = 1350  # 16:9 of the top screen's width
            y0 = int(round((1440 - ch) * shot.y))
            vf = f"{top}crop=2400:{ch}:0:{y0},scale={W}:{H}:flags=lanczos"
            self.size = (W, H)
        # (half a frame early: the decode starts at the first frame at or after the seek)
        args = ["ffmpeg", "-loglevel", "error", "-ss", f"{max(0.0, (shot.src - 0.5) / FPS):.5f}", "-i", path]
        args += ["-filter_complex", vf] if shot.layout == "ds" else ["-vf", vf]
        args += ["-f", "rawvideo", "-pix_fmt", "rgb24", "-"]
        self.proc = subprocess.Popen(args, stdout=subprocess.PIPE, bufsize=self.size[0] * self.size[1] * 3 * 2)

    def frame(self, k):
        """Source frame k (absolute in the reel); past the reel's end, its last frame."""
        while self.next <= k:
            buf = self.proc.stdout.read(self.size[0] * self.size[1] * 3)
            if len(buf) < self.size[0] * self.size[1] * 3:
                break
            self.last = np.frombuffer(buf, np.uint8).reshape(self.size[1], self.size[0], 3)
            self.next += 1
        return self.last

    def close(self):
        self.proc.kill()


def rounded_mask(w, h, r):
    m = Image.new("L", (w * 4, h * 4), 0)
    ImageDraw.Draw(m).rounded_rectangle([0, 0, w * 4 - 1, h * 4 - 1], r * 4, fill=255)
    return np.asarray(m.resize((w, h), Image.LANCZOS), np.float32) / 255.0


class Look:
    """What's drawn over and around the shots: the DS frame, the vignette, the titles, captions, end card."""

    def __init__(self, W, H, scale):
        self.W, self.H, self.s = W, H, scale
        yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
        d = np.sqrt(((xx - W / 2) / (W / 2)) ** 2 + ((yy - H / 2) / (H / 2)) ** 2) / math.sqrt(2)
        self.vignette = (1.0 - 0.22 * np.clip((d - 0.45) / 0.55, 0, 1) ** 1.6)[..., None]
        # the DS layout: the top screen over the bottom, centred, rounded, with a soft shadow
        self.tall = CANVAS == "tall"
        if self.tall:  # the vertical cut: the screens the frame's width, below a band for the wordmark
            tw, th, bw, gap, y0 = int(1080 * scale), int(648 * scale), int(864 * scale), int(24 * scale), int(300 * scale)
            self.full = (0, int(600 * scale), tw, th)  # a one-screen shot's place
        else:
            tw, th, bw = int(800 * scale), int(480 * scale), int(640 * scale)
            gap = int(30 * scale)
            y0 = (H - th * 2 - gap) // 2
        self.ds = {"top": ((W - tw) // 2, y0, tw, th), "bottom": ((W - bw) // 2, y0 + th + gap, bw, th)}
        self.ds_mask = {k: rounded_mask(v[2], v[3], int(12 * scale)) for k, v in self.ds.items()}
        shadow = Image.new("L", (W, H), 0)
        dr = ImageDraw.Draw(shadow)
        for x, y, w, h in self.ds.values():
            dr.rounded_rectangle([x - 2, y + int(8 * scale), x + w + 2, y + h + int(12 * scale)], int(16 * scale), fill=200)
        self.ds_shadow = (np.asarray(shadow.filter(ImageFilter.GaussianBlur(18 * scale)), np.float32) / 255.0 * 0.75)[..., None]
        self.fonts = {}
        self.cache = {}
        self.embers = np.random.default_rng(7).random((70, 5))  # x, speed, size, phase, hue

    def font(self, which, size):
        key = (which, int(size * self.s))
        if key not in self.fonts:
            self.fonts[key] = ImageFont.truetype(FONT_TITLE if which == "title" else FONT_BODY, int(size * self.s))
        return self.fonts[key]

    # -- text, drawn once into a layer (RGBA float, premultiplied) with its glow and shadow
    def text_layer(self, key, draw_fn, pad=60):
        if key in self.cache:
            return self.cache[key]
        img = Image.new("RGBA", (self.W, self.H), (0, 0, 0, 0))
        draw_fn(img)
        a = np.asarray(img, np.float32) / 255.0
        alpha = Image.fromarray((a[..., 3] * 255).astype(np.uint8))
        sh = np.asarray(alpha.filter(ImageFilter.GaussianBlur(7 * self.s)), np.float32)[..., None] / 255.0
        sh = np.roll(sh, int(3 * self.s), axis=0) * 0.7
        rgb = a[..., :3] * a[..., 3:4]
        out_rgb = rgb + np.array(TRACK, np.float32) / 255.0 * sh * (1 - a[..., 3:4])
        out_a = a[..., 3:4] + sh * (1 - a[..., 3:4])
        ys, xs = np.nonzero(out_a[..., 0] > 0.002)
        box = (max(0, ys.min() - 2), min(self.H, ys.max() + 3), max(0, xs.min() - 2), min(self.W, xs.max() + 3)) if len(ys) else (0, 1, 0, 1)
        y0, y1, x0, x1 = box
        layer = (out_rgb[y0:y1, x0:x1].copy(), out_a[y0:y1, x0:x1].copy(), (y0, x0))
        self.cache[key] = layer
        return layer

    @staticmethod
    def blend(frame, layer, opacity, dy=0):
        if opacity <= 0.001:
            return
        rgb, a, (y0, x0) = layer
        y0 += int(dy)
        h, w = a.shape[:2]
        if y0 < 0 or y0 + h > frame.shape[0]:
            return
        region = frame[y0:y0 + h, x0:x0 + w]
        region *= (1 - a * opacity)
        region += rgb * opacity

    def centered(self, draw, text, font, cy, fill, spacing=0):
        if spacing:
            widths = [font.getlength(ch) for ch in text]
            total = sum(widths) + spacing * (len(text) - 1)
            x = self.W / 2 - total / 2
            for ch, wch in zip(text, widths):
                draw.text((x, cy), ch, font=font, fill=fill, anchor="lm")
                x += wch + spacing
        else:
            draw.text((self.W / 2, cy), text, font=font, fill=fill, anchor="mm")

    def wordmark(self, cy, size=128, sub_gap=96, sub_size=40, glow=True):
        """EMBERCLUTCH in gold over a rule-flanked SKYREACH VALLEY (the title screen's look)."""
        key = ("wordmark", cy, size)
        s = self.s

        def draw(img):
            if glow:
                g = Image.new("RGBA", img.size, (0, 0, 0, 0))
                ImageDraw.Draw(g).text((self.W / 2, cy), "EMBERCLUTCH", font=self.font("title", size), fill=EMBER + (150,), anchor="mm")
                img.alpha_composite(g.filter(ImageFilter.GaussianBlur(22 * s)))
            d = ImageDraw.Draw(img)
            d.text((self.W / 2, cy), "EMBERCLUTCH", font=self.font("title", size), fill=GOLD + (255,), anchor="mm")
            f2 = self.font("title", sub_size)
            label = "Skyreach Valley"
            wlab = f2.getlength(label)
            y2 = cy + sub_gap * s
            d.text((self.W / 2, y2), label, font=f2, fill=SHELL + (255,), anchor="mm")
            gap, rule = 26 * s, 150 * s
            for side in (-1, 1):
                xa = self.W / 2 + side * (wlab / 2 + gap)
                xb = xa + side * rule
                d.line([(xa, y2 + 2 * s), (xb, y2 + 2 * s)], fill=GOLD + (255,), width=max(1, int(3 * s)))
                r = 5 * s
                d.ellipse([xb - r, y2 + 2 * s - r, xb + r, y2 + 2 * s + r], fill=GOLD + (255,))
        return self.text_layer(key, draw)

    def caption(self, text, side=False):
        """Low and centred over a full shot; beside the screens (two lines, in the open space) over a DS one."""
        def draw(img):
            d = ImageDraw.Draw(img)
            if side:
                words = text.split()
                half = (len(words) + 1) // 2
                f = self.font("body", 52)
                x = self.ds["top"][0] / 2
                for i, part in enumerate([" ".join(words[:half]), " ".join(words[half:])]):
                    d.text((x, self.H / 2 + (i - 0.5) * 66 * self.s), part, font=f, fill=SHELL + (255,), anchor="mm")
            else:
                d.text((self.W / 2, self.H - 118 * self.s), text, font=self.font("body", 54), fill=SHELL + (255,), anchor="mm")
        return self.text_layer(("caption", text, side), draw)

    def caption_tall(self, text, cy):
        """The vertical cut's captions: its lines, as said, wrapped under the screens."""
        def draw(img):
            d = ImageDraw.Draw(img)
            f = self.font("body", 60)
            lines, cur = [], ""
            for w in text.split():
                trial = (cur + " " + w).strip()
                if cur and f.getlength(trial) > 940 * self.s:
                    lines.append(cur)
                    cur = w
                else:
                    cur = trial
            lines.append(cur)
            for i, ln in enumerate(lines):
                d.text((self.W / 2, (cy + (i - (len(lines) - 1) / 2) * 76) * self.s), ln, font=f, fill=SHELL + (255,), anchor="mm")
        return self.text_layer(("ctall", text, cy), draw)

    def line(self, text, cy, size, fill, font="body"):
        def draw(img):
            ImageDraw.Draw(img).text((self.W / 2, cy * self.s), text, font=self.font(font, size), fill=fill + (255,), anchor="mm")
        return self.text_layer(("line", text, cy, size), draw)

    # -- the end card's own picture: the title screen's plum, its egg aglow, embers rising
    def end_background(self, t):
        key = "endbg"
        if key not in self.cache:
            W, H, s = self.W, self.H, self.s
            yy = np.linspace(0, 1, H, dtype=np.float32)[:, None, None]
            bg = (np.array(TRACK, np.float32) * (1 - yy) * 0.35 + np.array(PLUM, np.float32) * (1 - yy) * 0.65
                  + np.array(DUSK, np.float32) * yy) / 255.0
            bg = np.broadcast_to(bg, (H, W, 3)).copy()
            # the egg's glow, over the whole frame (a soft elliptical fall-off: nothing to clip at any edge)
            ex, ey, eh = W / 2, H * 0.52, 250 * s
            gy, gx = np.mgrid[0:H, 0:W].astype(np.float32)
            r = np.sqrt(((gx - ex) / 0.82) ** 2 + (gy - ey) ** 2) / eh
            glow = (np.exp(-(r / 0.95) ** 2) * 0.62 + np.exp(-(r / 2.2) ** 2) * 0.18)[..., None]
            bg += glow * (np.array(EMBER, np.float32) / 255.0) * 0.9
            # the egg, drawn 4x in a box about it and brought down, for clean edges
            k = 4
            bw_, bh_ = int(eh * 1.2), int(eh * 1.4)
            bx0, by0 = int(ex - bw_ / 2), int(ey - bh_ / 2)
            big = Image.new("RGBA", (bw_ * k, bh_ * k), (0, 0, 0, 0))
            cx, cy = (ex - bx0) * k, (ey - by0) * k
            # the egg: cream warming to peach toward its foot (as on the title screen), a soft sheen, its spots
            rw, rh = eh * 0.37 * k, eh * 0.5 * k
            mask = Image.new("L", big.size, 0)
            ImageDraw.Draw(mask).ellipse([cx - rw, cy - rh, cx + rw, cy + rh], fill=255)
            yy = np.clip((np.arange(big.size[1], dtype=np.float32) - (cy - rh)) / (2 * rh), 0, 1)
            g = np.clip((yy - 0.42) / 0.45, 0, 1)
            g = g * g * (3 - 2 * g)
            col = (np.array((255, 238, 205), np.float32)[None, :] * (1 - g[:, None])
                   + np.array((246, 178, 112), np.float32)[None, :] * g[:, None])
            body = np.broadcast_to(col[:, None, :], (big.size[1], big.size[0], 3))
            egg = np.dstack([body, np.asarray(mask, np.float32)]).astype(np.uint8)
            big = Image.fromarray(egg, "RGBA")
            sheen = Image.new("L", big.size, 0)
            ImageDraw.Draw(sheen).ellipse([cx - rw * 0.62, cy - rh * 0.78, cx - rw * 0.05, cy - rh * 0.18], fill=110)
            sheen = sheen.filter(ImageFilter.GaussianBlur(rw * 0.18))
            sheen = Image.fromarray((np.asarray(sheen, np.float32) * (np.asarray(mask, np.float32) / 255.0)).astype(np.uint8))
            big.alpha_composite(Image.merge("RGBA", (Image.new("L", big.size, 255),) * 3 + (sheen,)))
            d = ImageDraw.Draw(big)
            for (sx, sy, sr) in [(-0.35, -0.45, 0.07), (0.25, -0.25, 0.05), (-0.1, 0.15, 0.09), (0.3, 0.42, 0.06), (-0.45, 0.35, 0.05)]:
                d.ellipse([cx + sx * rw - sr * rw, cy + sy * rh - sr * rw, cx + sx * rw + sr * rw, cy + sy * rh + sr * rw],
                          fill=(196, 120, 80, 210))
            small = big.resize((bw_, bh_), Image.LANCZOS)
            eg = np.asarray(small, np.float32) / 255.0
            region = bg[by0:by0 + bh_, bx0:bx0 + bw_]
            region[:] = region * (1 - eg[..., 3:4]) + eg[..., :3] * eg[..., 3:4]
            dot = np.zeros((25, 25), np.float32)
            cv2.circle(dot, (12, 12), 5, 1.0, -1)
            dot = cv2.GaussianBlur(dot, (0, 0), 3.0)
            self.cache[key] = (np.clip(bg, 0, 1), dot / dot.max())
        bg, dot = self.cache[key]
        frame = bg.copy()
        W, H, s = self.W, self.H, self.s
        for x, sp, size, ph, hue in self.embers:
            y = (H * 1.1 - ((t * (30 + 60 * sp) * s + ph * H * 1.2) % (H * 1.2)))
            xx = int(x * W + math.sin(t * 0.8 + ph * 6) * 18 * s)
            yy = int(y)
            sz = max(5, int((10 + 16 * size) * s))
            spr = cv2.resize(dot, (sz, sz))[..., None]
            if 0 <= yy < H - sz and 0 <= xx < W - sz:
                col = (np.array(GOLD if hue > 0.4 else EMBER, np.float32) / 255.0)
                flick = 0.35 + 0.35 * math.sin(t * (2 + 3 * sp) + ph * 9) ** 2
                frame[yy:yy + sz, xx:xx + sz] += spr * col * flick
        return np.clip(frame, 0, 1)


def kb_at(shot, tau, dur):
    """The push's zoom and centre (in the top screen, 0..1) at tau: (zoom from, zoom to) about the crop's centre
    over the shot, or keyframes [(tau, zoom, cx, cy), ...] eased between (two close together: a snap)."""
    kb = shot.kb
    if len(kb) == 2 and not isinstance(kb[0], tuple):
        k = min(1.0, max(0.0, tau / max(dur, 1e-6)))
        k = k * k * (3 - 2 * k)
        return kb[0] + (kb[1] - kb[0]) * k, 0.5, (90 * shot.y + 675) / 1440
    if tau <= kb[0][0]:
        return kb[0][1:]
    for (t0, z0, x0, y0), (t1, z1, x1, y1) in zip(kb, kb[1:]):
        if tau <= t1:
            k = (tau - t0) / max(t1 - t0, 1e-6)
            k = k * k * (3 - 2 * k)
            return z0 + (z1 - z0) * k, x0 + (x1 - x0) * k, y0 + (y1 - y0) * k
    return kb[-1][1:]


def kb_crop(shot, src_frame, tau, dur, W, H):
    """A push (Ken Burns) on the top screen: a 16:9 crop of it, resampled; kept inside the picture."""
    z, cx, cy = kb_at(shot, tau, dur)
    cw, ch = 2400 / z, 1350 / z
    x0 = min(max(cx * 2400 - cw / 2, 0.0), 2400 - cw)
    y0 = min(max(cy * 1440 - ch / 2, 0.0), 1440 - ch)
    m = np.array([[W / cw, 0, -x0 * W / cw], [0, H / ch, -y0 * H / ch]], np.float32)
    return cv2.warpAffine(src_frame, m, (W, H), flags=cv2.INTER_CUBIC)


def render_video(path, t0, t1, scale):
    W, H = canvas(scale)
    look = Look(W, H, scale)
    enc = subprocess.Popen(["ffmpeg", "-loglevel", "error", "-y", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{W}x{H}",
                            "-r", str(FPS), "-i", "-", "-vf", "scale=out_color_matrix=bt709:out_range=tv",
                            "-c:v", "libx264", "-preset", "slow" if scale >= 1 else "veryfast", "-crf", "15",
                            "-pix_fmt", "yuv420p", "-colorspace", "bt709", "-color_primaries", "bt709",
                            "-color_trc", "bt709", "-color_range", "tv", path], stdin=subprocess.PIPE)
    decs = {}
    n0, n1 = int(round(t0 * FPS)), int(round(t1 * FPS))
    for n in range(n0, n1):
        t = n / FPS
        frame = shot_frame(t, decs, look, W, H, scale)
        # the end card over everything, fading up
        if t >= END_AT:
            k = min(1.0, (t - END_AT) / 1.2)
            card = look.end_background(t - END_AT)
            frame = frame * (1 - k) + card * k
        frame *= look.vignette
        overlays(frame, t, look)
        if t > LENGTH - 1.6:  # the last breath: down to black
            frame *= max(0.0, (LENGTH - t) / 1.2) if t > LENGTH - 1.2 else 1.0
        enc.stdin.write((np.clip(frame, 0, 1) * 255 + 0.5).astype(np.uint8).tobytes())
        for name in [k for k in decs if not any(s.reel == k and s.t - 1 <= t < s.end + 2 for s in SHOTS)]:
            decs.pop(name).close()
        if n % 300 == 0:
            print(f"[edit] video {t:6.1f} s", flush=True)
    enc.stdin.close()
    enc.wait()
    for d in decs.values():
        d.close()


def canvas(scale):
    w, h = (1080, 1920) if CANVAS == "tall" else (1920, 1080)
    return int(w * scale) // 2 * 2, int(h * scale) // 2 * 2


def still(t, scale=1.0):
    """One finished frame at t (fresh decoders), as 8-bit RGB: for looking at moments without a render."""
    W, H = canvas(scale)
    look = Look(W, H, scale)
    decs = {}
    frame = shot_frame(t, decs, look, W, H, scale)
    if t >= END_AT:
        k = min(1.0, (t - END_AT) / 1.2)
        frame = frame * (1 - k) + look.end_background(t - END_AT) * k
    frame *= look.vignette
    overlays(frame, t, look)
    for d in decs.values():
        d.close()
    return (np.clip(frame, 0, 1) * 255 + 0.5).astype(np.uint8)


def shot_frame(t, decs, look, W, H, scale):
    """The picture at t: the shot playing (and the one it's crossfading from), as floats 0..1."""
    active = [s for s in SHOTS if s.t - (0 if s.fade == 0 else 0) <= t < s.end + next_fade(s)]
    if not active:
        return np.zeros((H, W, 3), np.float32)
    frames = []
    for s in active:
        if s.reel not in decs or decs[s.reel].shot is not s:
            if s.reel in decs:
                decs[s.reel].close()
            decs[s.reel] = Decoder(s, W, H, scale)
        d = decs[s.reel]
        tau = t - s.t
        k = s.src + int(round(tau * FPS))
        img = d.frame(k)
        if img is None:
            frames.append((s, np.zeros((H, W, 3), np.float32)))
            continue
        frames.append((s, compose(s, img, tau, look, W, H)))
    out = frames[0][1]
    for s, f in frames[1:]:  # the later shot fading in over the earlier
        k = min(1.0, max(0.0, (t - s.t) / s.fade)) if s.fade > 0 else 1.0
        out = out * (1 - k) + f * k
    cur = frames[-1][0]
    if cur.dip:  # a dip: up to the colour before the cut, down from it after
        col = np.array((1.0, 0.98, 0.94) if cur.dip == "white" else (0, 0, 0), np.float32)
        u = (t - cur.t)
        a = max(0.0, 1 - u / 0.55) if u >= 0 else 0.0
        out = out * (1 - a) + col * a
    nxt = next((s for s in SHOTS if s.t > t and s.dip), None)
    if nxt is not None and nxt.t - t < 0.3:
        col = np.array((1.0, 0.98, 0.94) if nxt.dip == "white" else (0, 0, 0), np.float32)
        a = 1 - (nxt.t - t) / 0.3
        out = out * (1 - a) + col * a
    if t < OPEN + 0.9:  # out of black
        out = out * max(0.0, (t - OPEN) / 0.9)
    return out


def next_fade(s):
    i = SHOTS.index(s)
    return SHOTS[i + 1].fade if i + 1 < len(SHOTS) else 1.5  # (the last runs on under the end card)


def backdrop(top, look, W, H):
    """Around the screens: the top screen itself, blurred far out and dimmed toward the den's plum."""
    small = cv2.resize(top, (36, 64) if look.tall else (64, 36), interpolation=cv2.INTER_AREA)
    bg = cv2.resize(cv2.GaussianBlur(small, (0, 0), 2.2), (W, H), interpolation=cv2.INTER_CUBIC).astype(np.float32) / 255.0
    return bg * 0.42 + np.array(PLUM, np.float32) / 255.0 * 0.32


def compose(s, img, tau, look, W, H):
    if look.tall and s.layout != "ds":  # the vertical cut's one-screen shot: the whole top screen across the middle
        bg = backdrop(img, look, W, H)
        x, y, w, h = look.full
        band = int(36 * look.s)
        bg[y - band:y] *= np.linspace(1.0, 0.55, band, dtype=np.float32)[:, None, None]
        bg[y + h:y + h + band] *= np.linspace(0.55, 1.0, band, dtype=np.float32)[:, None, None]
        bg[y:y + h, x:x + w] = img.astype(np.float32) / 255.0
        return bg
    if s.layout == "ds":
        tw = look.ds["top"][2]
        top, bottom = img[:, :tw], img[:, tw:]
        bg = backdrop(top, look, W, H)
        bg *= (1 - look.ds_shadow)
        for part, pix in (("top", top), ("bottom", bottom)):
            x, y, w, h = look.ds[part]
            m = look.ds_mask[part][..., None]
            bg[y:y + h, x:x + w] = bg[y:y + h, x:x + w] * (1 - m) + pix.astype(np.float32) / 255.0 * m
        return bg
    out = (kb_crop(s, img, tau, s.end - s.t, W, H) if s.kb else img).astype(np.float32) / 255.0
    if s.lift:
        out = out ** (1.0 / (1.0 + s.lift))
    return out


def overlays(frame, t, look):
    s = look.s
    t0, t1 = TITLE
    if t0 - 0.1 <= t <= t1 + 0.5:
        a = min(1.0, (t - t0) / 0.7) if t < t0 + 0.7 else max(0.0, 1 - (t - t1) / 0.45)
        mark = look.wordmark(330 * s, size=92, sub_gap=70, sub_size=32) if look.tall else look.wordmark(look.H * 0.30)
        look.blend(frame, mark, max(0, a), dy=(1 - min(1, max(0, (t - t0) / 0.7))) * 10 * s)
    if look.tall and t1 <= t < END_AT + 0.6:  # the vertical cut keeps a small wordmark in its top band
        a = min(1.0, (t - t1) / 0.5) * min(1.0, max(0.0, (END_AT + 0.6 - t) / 0.6))
        look.blend(frame, look.wordmark(150 * s, size=60, sub_gap=50, sub_size=22, glow=False), 0.92 * a)
    for text, c0, c1 in CAPTIONS:
        if c0 - 0.05 <= t <= c1 + 0.4:
            a = min(1.0, (t - c0) / 0.35) if t < c0 + 0.35 else max(0.0, 1 - (t - c1) / 0.35)
            on = next((x for x in SHOTS if x.t <= (c0 + c1) / 2 < x.end), None)
            ds = bool(on and on.layout == "ds")
            layer = look.caption_tall(text, 1775 if ds else 1420) if look.tall else look.caption(text, side=ds)
            look.blend(frame, layer, max(0, a), dy=(1 - min(1, max(0, (t - c0) / 0.35))) * 8 * s)
    if look.tall:
        mark_at, lines_at = 0.3, 3.7
        mark = look.wordmark(560 * s, size=100, sub_gap=76, sub_size=34)
        lines = [("Free homebrew for the Nintendo 3DS family", 1330, 44, SHELL), ("Runs on the original 2011 3DS", 1398, 38, GOLD),
                 ("github.com/RedLynx101/emberclutch", 1478, 36, SHELL),
                 ("A fan-made game. Not affiliated with Nintendo.", 1850, 24, (190, 170, 190))]
    else:
        mark_at, lines_at = 0.8, 4.3
        mark = None
        lines = [("Free homebrew for the Nintendo 3DS family", 790, 46, SHELL), ("Runs on the original 2011 3DS", 852, 38, GOLD),
                 ("github.com/RedLynx101/emberclutch", 930, 36, SHELL),
                 ("A fan-made game. Not affiliated with Nintendo.", 1032, 22, (190, 170, 190))]
    if t >= END_AT + mark_at:  # the end card's words, in turn
        u = t - END_AT
        mark = mark or look.wordmark(look.H * 0.2, size=118, sub_gap=88, glow=True)
        look.blend(frame, mark, min(1, (u - mark_at) / 0.8))
        for i, (text, cy, size, fill) in enumerate(lines):
            at = lines_at + 0.7 * min(i, 2)
            if u >= at:
                look.blend(frame, look.line(text, cy, size, fill), min(1, (u - at) / 0.6))


# ---------------------------------------------------------------------------------------------- the sound
def decode(path, start=0.0, dur=None, mono=False):
    args = ["ffmpeg", "-loglevel", "error"]
    if start:
        args += ["-ss", f"{start:.4f}"]
    args += ["-i", path]
    if dur:
        args += ["-t", f"{dur:.4f}"]
    args += ["-f", "f32le", "-ac", "1" if mono else "2", "-ar", str(RATE), "-"]
    a = np.frombuffer(subprocess.run(args, capture_output=True, check=True).stdout, np.float32)
    return a.copy() if mono else a.reshape(-1, 2).copy()


def place(bus, a, t, gain=1.0):
    i = int(round(t * RATE))
    if a.ndim == 1:
        a = np.stack([a, a], axis=1)
    if i < 0:
        a, i = a[-i:], 0
    j = min(len(bus), i + len(a))
    if j > i:
        bus[i:j] += a[:j - i] * gain


def fade(a, fin, fout):
    n = len(a)
    if fin > 0:
        k = min(n, int(fin * RATE))
        a[:k] *= np.linspace(0, 1, k)[:, None] if a.ndim == 2 else np.linspace(0, 1, k)
    if fout > 0:
        k = min(n, int(fout * RATE))
        a[n - k:] *= np.linspace(1, 0, k)[:, None] if a.ndim == 2 else np.linspace(1, 0, k)
    return a


def db(x):
    return 10 ** (x / 20)


def lufs(a):
    """Integrated loudness (ITU-R BS.1770-4, stereo at 48 kHz)."""
    b1, a1 = [1.53512485958697, -2.69169618940638, 1.19839281085285], [1.0, -1.69065929318241, 0.73248077421585]
    b2, a2 = [1.0, -2.0, 1.0], [1.0, -1.99004745483398, 0.99007225036621]
    k = signal.lfilter(b2, a2, signal.lfilter(b1, a1, a, axis=0), axis=0)
    blk, hop = int(0.4 * RATE), int(0.1 * RATE)
    if len(k) < blk:
        return -70.0
    ms = np.array([np.mean(k[i:i + blk] ** 2, axis=0).sum() for i in range(0, len(k) - blk, hop)])
    lk = -0.691 + 10 * np.log10(ms + 1e-12)
    g = ms[lk > -70]
    if not len(g):
        return -70.0
    rel = -0.691 + 10 * np.log10(g.mean()) - 10
    g = ms[(lk > -70) & (lk > rel)]
    return float(-0.691 + 10 * np.log10(g.mean()))


def limit(a, ceiling_db=-1.0):
    """A look-ahead peak limiter (true peak estimated at 4x)."""
    ceil = db(ceiling_db)
    up = signal.resample_poly(a, 4, 1, axis=0)
    peak = np.abs(up).max(axis=1).reshape(-1, 4).max(axis=1)[:len(a)]
    need = np.minimum(1.0, ceil / np.maximum(peak, 1e-9))
    look = int(0.004 * RATE)
    from scipy.ndimage import minimum_filter1d
    g = minimum_filter1d(need, size=2 * look + 1, origin=0)
    rel = math.exp(-1 / (0.08 * RATE))
    sm = signal.lfilter([1 - rel], [1, -rel], g)
    sm = np.minimum(sm, g)
    return a * sm[:, None]


def reverb_tail(a, seconds=2.2):
    """A synthetic room (exponentially decaying noise) for a struck chord to ring on in."""
    rng = np.random.default_rng(3)
    n = int(seconds * RATE)
    env = np.exp(-np.linspace(0, 6.5, n))
    ir = rng.standard_normal((n, 2)) * env[:, None]
    ir[:, 1] = np.roll(ir[:, 1], 37)
    b, a_ = signal.butter(2, 6000 / (RATE / 2))
    ir = signal.lfilter(b, a_, ir, axis=0)
    out = np.stack([signal.fftconvolve(a[:, c], ir[:, c]) for c in range(2)], axis=1)
    return out / (np.abs(out).max() + 1e-9) * np.abs(a).max() * 0.8


SOUND_CACHE = {}


def sound(file, pitch=1.0, lowpass=0.0):
    key = (file, round(pitch, 3), round(lowpass))
    if key not in SOUND_CACHE:
        if file.startswith("v") and "/" in file:  # a villager's letter: its first 2,600 frames at its own rate
            voice, ch = file.split("/")
            path = os.path.join(VOICE, voice, ch + ".wav")
            rate = int(subprocess.run(["ffprobe", "-v", "error", "-show_entries", "stream=sample_rate", "-of", "csv=p=0", path],
                                      capture_output=True, text=True).stdout.strip() or 16000)
            a = decode(path, 0, 2600 / rate, mono=True)
        else:
            a = decode(os.path.join(SFX, file + ".wav"), mono=True)
        if abs(pitch - 1.0) > 0.005:
            fr = Fraction(1 / pitch).limit_denominator(100)
            a = signal.resample_poly(a, fr.numerator, fr.denominator)
        if lowpass > 0:
            b, a_ = signal.butter(2, min(0.99, lowpass / (RATE / 2)))
            a = signal.lfilter(b, a_, a)
        SOUND_CACHE[key] = a.astype(np.float32)
    return SOUND_CACHE[key]


def narration_pieces():
    takes = json.load(open(os.path.join(NARR, "takes.json")))
    pick = {}
    for pid, p in takes.items():
        for piece in p["takes"][p["pick"] - 1]["pieces"]:
            pick[piece] = p["pick"]
    return pick


def render_audio(path, t0, t1):
    n = int(LENGTH * RATE) + RATE
    music, voice, fx, beds = (np.zeros((n, 2), np.float32) for _ in range(4))
    # the music's three movements
    for slug, src, at, end, fin, fout in MUSIC_PLAN:
        dur = (end - at) if end else None
        a = decode(os.path.join(MUSIC, slug + ".wav"), src, dur)
        a = fade(a, fin, fout)
        place(music, a, at)
        if slug == "skyreach":  # its last downbeat rings on into the hush
            hit = decode(os.path.join(MUSIC, slug + ".wav"), src + (end - 0.12 - at), 0.35)
            hit = fade(hit, 0.0, 0.3)
            place(music, reverb_tail(hit, 2.6) * 0.55, end - 0.12)
    # the narration, each line brought to one loudness, low end trimmed
    pick = narration_pieces()
    hp_b, hp_a = signal.butter(2, 85 / (RATE / 2), "high")
    spans = []
    for piece, at in NARRATION:
        a = decode(os.path.join(NARR, f"{piece}_t{pick[piece]}.wav"), mono=True)
        a = signal.lfilter(hp_b, hp_a, a).astype(np.float32)
        loud = np.sqrt(np.mean(a[np.abs(a) > 0.02] ** 2) + 1e-12)
        a *= db(-17.5) / loud
        place(voice, a, at)
        spans.append((at, at + len(a) / RATE))
    # the game's own sounds and beds, from each shot's cue sheet (sounds that start in the shot; beds while it plays)
    for s in SHOTS:
        if s.sound is None:
            continue
        _, _, _, _, cues = reel(s.reel)
        f0, f1 = s.src, s.src + int((s.end - s.t) * FPS)
        for (f, kind, file, pitch, gain, lp) in cues:
            if kind not in ("sfx", "letter") or not (f0 <= f < f1):
                continue
            if (not s.ui and file.startswith(SKIP_SOUNDS)) or (s.mute and file.startswith(s.mute)):
                continue
            at = s.t + (f - s.src) / FPS
            place(fx, sound(file, pitch, lp), at, gain * db(s.sound - 8.0))
        levels = {}
        for (f, kind, file, pitch, gain, lp) in cues:
            if kind == "bed":
                levels.setdefault(file, []).append((f, gain))
        for file, pts in levels.items():
            pts.sort()
            frames = np.arange(f0, f1)
            lv = np.interp(frames, [p[0] for p in pts], [p[1] for p in pts], left=pts[0][1])
            if lv.max() < 0.01:
                continue
            loop = sound(file)
            dur = (f1 - f0) / FPS + 0.6
            reps = int(np.ceil(dur * RATE / len(loop))) + 1
            start = (f0 * 1234567) % len(loop)  # (somewhere in the loop: not always its start)
            a = np.tile(loop, reps)[start:start + int(dur * RATE)]
            env = np.interp(np.arange(len(a)) / RATE, (frames - f0) / FPS, lv)
            a = a * env
            a = fade(a, 0.25, 0.5)
            place(beds, a, s.t, db(s.beds))
    for file, at, gdb in EXTRA_SOUNDS:
        place(fx, sound(file), at, db(gdb))
    for file, a0, a1, gdb, fin, fout in AMBIENCE:
        loop = sound(file)
        n_ = int((a1 - a0) * RATE)
        a = np.tile(loop, n_ // len(loop) + 2)[:n_].copy()
        place(beds, fade(a, fin, fout), a0, db(gdb))
    # the music and beds lowered under the voice (attack 90 ms, release 180 ms: quick enough for a downbeat after a line)
    gate = np.zeros(n, np.float32)
    for a0, a1 in spans:
        gate[int((a0 - 0.08) * RATE):int((a1 + 0.02) * RATE)] = 1.0
    att, rel = math.exp(-1 / (0.09 * RATE)), math.exp(-1 / (0.18 * RATE))
    env = np.zeros(n, np.float32)
    up = signal.lfilter([1 - att], [1, -att], gate)
    down = signal.lfilter([1 - rel], [1, -rel], gate)
    env = np.maximum(up, down)
    duck = 1 - (1 - db(-8.0)) * env
    mix = music * db(MUSIC_DB) * duck[:, None] + voice * db(VOICE_DB) + fx * db(-2.0) + beds * (1 - (1 - db(-4)) * env)[:, None]
    mix = fade(mix[:int(LENGTH * RATE)], 0.0, 1.5)
    # the whole thing to -14 LUFS (YouTube's level), peaks held under -1 dBTP (twice: the limiter takes a little)
    lu = lufs(mix)
    raw = mix
    for _ in range(2):
        mix = limit(raw * db(-14.0 - lu), -1.0)
        lu += lufs(mix) + 14.0
    lu = lufs(raw)
    print(f"[edit] sound: {lu:.1f} LUFS before, {lufs(mix):.1f} after; peak {20 * np.log10(np.abs(mix).max()):.1f} dBFS")
    seg = mix[int(t0 * RATE):int(t1 * RATE)]
    subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-f", "f32le", "-ar", str(RATE), "-ac", "2", "-i", "-",
                    "-c:a", "pcm_s24le", path], input=seg.astype(np.float32).tobytes(), check=True)
    stems = {"music": music * db(MUSIC_DB) * duck[:, None], "voice": voice * db(VOICE_DB), "fx": fx * db(-2.0), "beds": beds}
    gain = db(-14.0 - lu)
    for k, v in stems.items():
        print(f"[edit]   {k:<6} {lufs(v[:int(LENGTH * RATE)] * gain):6.1f} LUFS")
    plot_sound(path.replace(".wav", "_sound.png"), {k: v[:int(LENGTH * RATE)] * gain for k, v in stems.items()}, mix)
    return stems


def plot_sound(path, stems, mix):
    """The mix seen, not heard: each stem's short-term loudness, with the cuts, lines and bar lines."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    def short_term(a):
        hop, win = RATE // 10, RATE * 3 // 10
        k = np.array([np.sqrt(np.mean(a[i:i + win] ** 2) + 1e-12) for i in range(0, len(a) - win, hop)])
        return np.arange(len(k)) * 0.1, 20 * np.log10(k)
    fig, ax = plt.subplots(figsize=(26, 6))
    for name, v in list(stems.items()) + [("mix", mix)]:
        t, y = short_term(v)
        ax.plot(t, y, lw=1.4 if name == "mix" else 0.9, label=name, color="k" if name == "mix" else None)
    for s in SHOTS:
        ax.axvline(s.t, color="r", lw=0.6, alpha=0.6)
        ax.text(s.t + 0.05, -6, s.reel.split("_", 1)[1][:8], fontsize=7, color="r", rotation=90, va="top")
    for piece, at in NARRATION:
        ax.text(at, -58, piece, fontsize=7, color="g", rotation=90)
    ax.axvline(END_AT, color="m", lw=1.2)
    ax.set_ylim(-60, 0)
    ax.set_xlim(0, LENGTH)
    ax.set_xticks(np.arange(0, LENGTH, 2))
    ax.grid(alpha=0.25)
    ax.legend(loc="lower right", fontsize=8)
    plt.tight_layout()
    plt.savefig(path, dpi=60)


# ---------------------------------------------------------------------------------------------- the rest
def srt(path):
    takes = narration_pieces()
    texts = {}
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import narration
    for _, pieces in narration.PASSAGES:
        for piece, words in pieces:
            texts[piece] = words

    def ts(x):
        h, r = divmod(x, 3600)
        m, s = divmod(r, 60)
        return f"{int(h):02d}:{int(m):02d}:{int(s):02d},{int((s % 1) * 1000):03d}"
    lines = []
    for i, (piece, at) in enumerate(NARRATION, 1):
        a = decode(os.path.join(NARR, f"{piece}_t{takes[piece]}.wav"), mono=True)
        lines.append(f"{i}\n{ts(at)} --> {ts(at + len(a) / RATE)}\n{texts[piece]}\n")
    open(path, "w", encoding="utf-8").write("\n".join(lines))


def sheet(video, path, every=1.0):
    w, h, n = probe(video)
    tw, th = (384, 216) if w >= h else (162, 288)
    picks = list(range(0, n, int(every * FPS)))
    proc = subprocess.Popen(["ffmpeg", "-loglevel", "error", "-i", video, "-vf", f"scale={tw}:{th}", "-f", "rawvideo",
                             "-pix_fmt", "rgb24", "-"], stdout=subprocess.PIPE)
    tiles, k = [], 0
    while True:
        buf = proc.stdout.read(tw * th * 3)
        if len(buf) < tw * th * 3:
            break
        if k in picks:
            im = Image.fromarray(np.frombuffer(buf, np.uint8).reshape(th, tw, 3))
            ImageDraw.Draw(im).text((4, 4), f"{k / FPS:.0f}s", fill=(255, 220, 120))
            tiles.append(im)
        k += 1
    cols = 8 if w >= h else 12
    rows = (len(tiles) + cols - 1) // cols
    out = Image.new("RGB", (cols * tw, rows * th))
    for i, t in enumerate(tiles):
        out.paste(t, ((i % cols) * tw, (i // cols) * th))
    out.save(path)


def previews():
    """The review page's copies (its files are 15 MB at most): 720p at 30 fps, two passes to a size, AAC 96k."""
    for name, size, kbps in (("trailer", "1280:720", 1150), ("shorts", "720:1280", 1500)):
        src = os.path.join(OUT, name + ".mp4")
        if not os.path.exists(src):
            continue
        dst = os.path.join(OUT, name + "_720.mp4")
        log = os.path.join(OUT, name + "_2pass")
        vf = f"scale={size}:flags=lanczos,fps=30"
        common = ["-c:v", "libx264", "-preset", "slow", "-b:v", f"{kbps}k", "-maxrate", f"{kbps * 2}k", "-bufsize", f"{kbps * 4}k",
                  "-pix_fmt", "yuv420p", "-passlogfile", log]
        subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", src, "-vf", vf] + common + ["-pass", "1", "-an", "-f", "null", "NUL"],
                       check=True)
        subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", src, "-vf", vf] + common +
                       ["-pass", "2", "-c:a", "aac", "-b:a", "96k", "-movflags", "+faststart", dst], check=True)
        for f in os.listdir(OUT):
            if f.startswith(name + "_2pass"):
                os.remove(os.path.join(OUT, f))
        print(f"[edit] {dst}: {os.path.getsize(dst) / 1e6:.1f} MB")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preview", action="store_true")
    ap.add_argument("--from", dest="t0", type=float, default=0.0)
    ap.add_argument("--to", dest="t1", type=float, default=None)
    ap.add_argument("--video-only", action="store_true")
    ap.add_argument("--audio-only", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--sheet", action="store_true")
    ap.add_argument("--out", default=None)
    ap.add_argument("--cut", default="wide", choices=["wide", "tall"], help="the trailer, or the vertical cut for Shorts")
    ap.add_argument("--stills", default="", help="comma-separated times: one PNG each in build/film/edit/stills")
    ap.add_argument("--previews", action="store_true", help="only the review page's 720p copies of the finished cuts")
    a = ap.parse_args()
    if a.previews:
        previews()
        return
    a.out = a.out or ("shorts" if a.cut == "tall" else "trailer")
    os.makedirs(OUT, exist_ok=True)
    use_cut(a.cut)
    resolve(SHOTS)
    t1 = a.t1 if a.t1 is not None else LENGTH
    if a.stills:
        os.makedirs(os.path.join(OUT, "stills"), exist_ok=True)
        for x in a.stills.split(","):
            p = os.path.join(OUT, "stills", f"{a.cut[0]}{float(x):06.2f}.png")
            Image.fromarray(still(float(x), 0.5 if a.preview else 1.0)).save(p)
            print(p)
        return
    if a.check:
        for s in SHOTS:
            _, w, h, n, cues = reel(s.reel)
            used = s.src + int((s.end - s.t) * FPS)
            print(f"{s.t:7.3f}-{s.end:7.3f}  {s.reel:<13} frames {s.src:4d}-{used:4d} of {n:4d}{'  SHORT' if used > n else ''}"
                  f"  {s.layout}{'  anchor ' + str(s.anchor) if s.anchor else ''}  cues {len(cues)}")
        print(f"end card {END_AT:.3f}, length {LENGTH:.3f}")
        return
    base = os.path.join(OUT, a.out + ("_preview" if a.preview else ""))
    if not a.audio_only:
        render_video(base + "_video.mp4", a.t0, t1, 0.5 if a.preview else 1.0)
    if not a.video_only:
        render_audio(base + "_audio.wav", a.t0, t1)
    if not a.audio_only and not a.video_only:
        subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", base + "_video.mp4", "-i", base + "_audio.wav",
                        "-c:v", "copy", "-c:a", "aac", "-b:a", "256k", "-movflags", "+faststart", "-shortest", base + ".mp4"],
                       check=True)
        print(f"[edit] {base}.mp4")
        srt(base + ".srt")
    if a.sheet:
        sheet(base + ".mp4" if os.path.exists(base + ".mp4") else base + "_video.mp4", base + "_sheet.png")


if __name__ == "__main__":
    main()
