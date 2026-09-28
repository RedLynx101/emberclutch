"""Checks the files we put on the 3DS before they go (Noah, 2026-09-24: "create some checks to
validate any new files we make before putting them on the 3ds so we know they will be seen as
valid"). Every CIA, .3dsx, banner (.bnr), SMDH, CGFX, glTF, WAV and Ogg we build can be checked;
tools/deploy_ftp.ps1 and tools/banner_lab.ps1 refuse to upload anything that fails.

  py -3.12 tools/check_3ds.py <file> [<file> ...] [--quiet] [--sound-out <dir>]

What it checks, and why (each rule is one we met on the 3DS, or one the formats require):
- CIA: the header and its sections; the TMD's hashes (content info, chunk records, each
  content's SHA-256); the ticket's and the TMD's title IDs agree with the NCCH's.
- NCCH: unencrypted as we build it; the exheader's, ExeFS's and RomFS's hashes (the RomFS
  through every IVFC level), each ExeFS file's hash. A wrong hash is the "SD card removed"
  error (ErrDisp) we saw at the start.
- The boot logo: it has to be one the HOME Menu accepts. The logo is an LZ11-compressed darc
  ending in an HMAC-SHA256 whose key is Nintendo's (3dbrew "Logo"; yellows8/ctr-logobuilder
  needs a key taken from the HOME Menu). Ours, redrawn, couldn't carry a valid one, and 0.1.8
  stopped with "SD card removed" at start (run 9). So only logos known to work pass: makerom's
  homebrew logo (KNOWN_LOGOS).
- SMDH: the magic, the titles, region-free, the flags (visible, allow3d; extendedbanner for a
  3D banner), both icons.
- Banner (CBMD): the CGFX decompresses, fits 0x80000 bytes (3dbrew CBMD), is a CGFX whose model
  is COMMON; the sound (CWAV) is one the HOME Menu plays: 16-bit PCM, **stereo**, 32000 or
  44100 Hz, at most 3 s. Mono 22050 Hz sound (0.1.1 to lab 5) played as the same short high
  chirp whatever it held (runs 8, 9; gbatemp threads 398072 and 399472 describe the same
  beeping and its fix). --sound-out writes what the banner will play back as a WAV to listen to.
- CGFX: under 512 KB, the COMMON model; its dictionaries listed.
- glTF (a banner's source): no skins or bone weights (a skinned banner freezes the HOME Menu on
  hardware, gbatemp thread 683412).
- WAV: what bannertool is given (the same sound rules). A WAV inside romfs is one of the game's
  own sounds instead (romfs/sfx, romfs/voice): what audio.cpp's loadWav reads (16-bit PCM, the
  fmt chunk before the data), 22,050 Hz mono like the set, headroom under full scale, a clean
  end, and for a loop (amb-*, egg-hum, wing-flutter) a seam that doesn't jump.
- Ogg (romfs/music): an Ogg Vorbis stream, its channels, rate, length and loop tags.
- 3DSX: the header, its segment sizes add up, the SMDH and RomFS it carries.
"""
from __future__ import annotations

import hashlib
import math
import os
import struct
import sys
import wave

MEDIA = 0x200
CGFX_LIMIT = 0x80000  # a CXI banner's CGFX, decompressed (3dbrew CBMD)
SOUND_RATES = (32000, 44100)
SOUND_SECONDS = 3.0

# Decompressed logo darcs (with their HMAC) that the HOME Menu is known to accept: SHA-256.
KNOWN_LOGOS = {
    "e90f90f5682e72c61179771ac7e9e3c64a77d8fec9ecce3841dc0246dd5cba06":
        "makerom's homebrew logo (Logo: Homebrew; 0.1.7 ran with it on the 3DS)",
}

SMDH_FLAGS = {0x1: "visible", 0x2: "autoboot", 0x4: "allow3d", 0x8: "eula", 0x10: "autosave",
              0x20: "extendedbanner", 0x40: "rating", 0x80: "savedata", 0x100: "recordusage",
              0x400: "nosavebackups", 0x1000: "new3ds"}


class Report:
    def __init__(self, name: str, quiet: bool):
        self.name, self.quiet = name, quiet
        self.fails, self.warns = 0, 0
        print(f"{name}")

    def ok(self, msg: str) -> None:
        if not self.quiet:
            print(f"  ok    {msg}")

    def warn(self, msg: str) -> None:
        self.warns += 1
        print(f"  WARN  {msg}")

    def fail(self, msg: str) -> None:
        self.fails += 1
        print(f"  FAIL  {msg}")

    def check(self, cond: bool, msg: str, why: str = "") -> bool:
        if cond:
            self.ok(msg)
        else:
            self.fail(msg + (f": {why}" if why else ""))
        return cond


def sha(b: bytes) -> bytes:
    return hashlib.sha256(b).digest()


def align(x: int, a: int) -> int:
    return (x + a - 1) // a * a


# ------------------------------------------------------------------------------ LZ11
def lz11_decompress(b: bytes) -> bytes:
    if not b or b[0] != 0x11:
        raise ValueError(f"not LZ11 (first byte {b[:1].hex()})")
    size = b[1] | b[2] << 8 | b[3] << 16
    i, o = 4, bytearray()
    while len(o) < size:
        flags = b[i]
        i += 1
        for bit in range(8):
            if len(o) >= size:
                break
            if flags & (0x80 >> bit):
                t = b[i] >> 4
                if t == 0:
                    n = ((b[i] & 0xF) << 4 | b[i + 1] >> 4) + 0x11
                    disp = ((b[i + 1] & 0xF) << 8 | b[i + 2]) + 1
                    i += 3
                elif t == 1:
                    n = ((b[i] & 0xF) << 12 | b[i + 1] << 4 | b[i + 2] >> 4) + 0x111
                    disp = ((b[i + 2] & 0xF) << 8 | b[i + 3]) + 1
                    i += 4
                else:
                    n = t + 1
                    disp = ((b[i] & 0xF) << 8 | b[i + 1]) + 1
                    i += 2
                if disp > len(o):
                    raise ValueError("LZ11 reaches before the start")
                for _ in range(n):
                    o.append(o[-disp])
            else:
                o.append(b[i])
                i += 1
    return bytes(o)


# ------------------------------------------------------------------------------ SMDH
def check_smdh(r: Report, b: bytes, want_extended: bool | None = None) -> None:
    if not r.check(len(b) >= 0x36C0 and b[:4] == b"SMDH", "SMDH: magic and size (0x36C0)",
                   f"{b[:4]!r}, {len(b):#x} bytes"):
        return
    short = b[0x08 + 0x200:0x08 + 0x200 + 0x80].decode("utf-16le").split("\0")[0]  # English
    long_ = b[0x88 + 0x200:0x88 + 0x200 + 0x100].decode("utf-16le").split("\0")[0]
    pub = b[0x188 + 0x200:0x188 + 0x200 + 0x80].decode("utf-16le").split("\0")[0]
    r.check(bool(short) and bool(pub), f'SMDH: titles "{short}" / "{long_}" / "{pub}"', "an empty title")
    region, = struct.unpack_from("<I", b, 0x2018)
    r.check(region & 0x7F == 0x7F, f"SMDH: region-free ({region:#x})", "the HOME Menu would lock it by region")
    flags, = struct.unpack_from("<I", b, 0x2028)
    names = [n for bit, n in SMDH_FLAGS.items() if flags & bit]
    r.check(bool(flags & 0x1), f"SMDH: flags {', '.join(names) or 'none'}", "not visible")
    if want_extended is not None:
        has = bool(flags & 0x20)
        r.check(has == want_extended,
                f"SMDH: extendedbanner {'set' if has else 'clear'} for a {'3D' if want_extended else '2D'} banner",
                "a 3D banner needs the flag, a 2D one mustn't have it")
    small, large = b[0x2040:0x24C0], b[0x24C0:0x36C0]
    r.check(any(small) and any(large), "SMDH: 24 x 24 and 48 x 48 icons", "an icon is blank")


# ------------------------------------------------------------------------------ sound
def check_sound(r: Report, what: str, fmt: str, channels: int, rate: int, seconds: float) -> None:
    r.check(fmt == "PCM16", f"{what}: {fmt}", "the HOME Menu wants 16-bit PCM")
    r.check(channels == 2, f"{what}: {channels} channel{'s' if channels != 1 else ''}",
            "banner sound must be stereo: mono plays as the same short high chirp (runs 8, 9)")
    r.check(rate in SOUND_RATES, f"{what}: {rate} Hz", f"use {' or '.join(map(str, SOUND_RATES))} Hz")
    r.check(0.2 <= seconds <= SOUND_SECONDS, f"{what}: {seconds:.2f} s", f"at most {SOUND_SECONDS:.0f} s")


def check_cwav(r: Report, b: bytes, at: int, sound_out: str | None, label: str) -> None:
    if not r.check(b[at:at + 4] == b"CWAV", "banner sound: CWAV magic", repr(b[at:at + 4])):
        return
    bom, hsize, ver, fsize, nblocks = struct.unpack_from("<HHIIH", b, at + 4)
    r.check(bom == 0xFEFF and at + fsize <= len(b), f"banner sound: header (version {ver:#x}, {fsize} bytes)",
            "bad byte order or runs past the banner")
    refs = {}
    for k in range(nblocks):
        t, _, off, size = struct.unpack_from("<HHII", b, at + 0x14 + k * 12)
        refs[t] = (at + off, size)
    if not r.check(0x7000 in refs and 0x7001 in refs, "banner sound: INFO and DATA blocks", str(refs)):
        return
    info, data = refs[0x7000][0], refs[0x7001][0]
    r.check(b[info:info + 4] == b"INFO" and b[data:data + 4] == b"DATA", "banner sound: block magics")
    enc, loop, rate, _, frames = struct.unpack_from("<BBxxIII", b, info + 8)
    nch, = struct.unpack_from("<I", b, info + 0x1C)
    fmt = {0: "PCM8", 1: "PCM16", 2: "DSP-ADPCM", 3: "IMA-ADPCM"}.get(enc, f"encoding {enc}")
    check_sound(r, "banner sound", fmt, nch, rate, frames / rate if rate else 0)
    if enc != 1 or not rate:
        return
    chans = []
    for c in range(nch):
        _, _, off = struct.unpack_from("<HHi", b, info + 0x20 + c * 8)
        ci = info + 0x1C + off
        _, _, soff = struct.unpack_from("<HHi", b, ci)
        start = data + 8 + soff
        chans.append(struct.unpack_from(f"<{frames}h", b, start) if start + frames * 2 <= len(b) else None)
    if not r.check(all(c is not None for c in chans), "banner sound: every channel's samples inside the file"):
        return
    peak = max(max(abs(s) for s in c) for c in chans)
    r.check(peak > 1000, f"banner sound: peak {peak} of 32767", "silent")
    if sound_out:
        os.makedirs(sound_out, exist_ok=True)
        path = os.path.join(sound_out, f"{label}.wav")
        with wave.open(path, "wb") as w:
            w.setnchannels(nch)
            w.setsampwidth(2)
            w.setframerate(rate)
            inter = bytearray()
            for i in range(frames):
                for c in chans:
                    inter += struct.pack("<h", c[i])
            w.writeframes(bytes(inter))
        print(f"  ..    the banner's sound, as the 3DS reads it: {path}")


def check_wav(r: Report, path: str) -> None:
    try:
        with wave.open(path) as w:
            fmt = "PCM16" if w.getsampwidth() == 2 else f"PCM{8 * w.getsampwidth()}"
            check_sound(r, "WAV", fmt, w.getnchannels(), w.getframerate(), w.getnframes() / w.getframerate())
    except wave.Error as e:
        r.fail(f"WAV: not a PCM WAV ({e}); bannertool reads plain PCM only (a float WAV screeches)")


# The game's own sounds (romfs/sfx, romfs/voice) follow other rules than the banner's: what
# src/app/audio.cpp's loadWav reads (RIFF, "fmt " before "data", 16-bit PCM), in the set's format
# (tools/audio/process_sfx.py and make_synth_sfx.py write 22,050 Hz mono), levelled with room
# under full scale, ending cleanly (a sound cut off mid-wave clicks), and loops
# whose end runs smoothly into their start. One-shots are preloaded into linear memory at
# boot, so a long one is worth a second look.
GAME_RATE = 22050
GAME_ONESHOT_SECONDS = 3.0
GAME_LOOPS = ("egg-hum", "wing-flutter")  # the beds not named amb-*


def check_game_wav(r: Report, path: str) -> None:
    b = open(path, "rb").read()
    if not r.check(len(b) > 12 and b[:4] == b"RIFF" and b[8:12] == b"WAVE", "WAV: RIFF/WAVE", repr(b[:12])):
        return
    fmt, data, p = None, None, 12
    while p + 8 <= len(b):
        cid, size = b[p:p + 4], struct.unpack_from("<I", b, p + 4)[0]
        if cid == b"fmt " and fmt is None:
            fmt = struct.unpack_from("<HHIIHH", b, p + 8)
        elif cid == b"data":
            data = (p + 8, size)
            break  # loadWav stops at the data too
        p += 8 + size + (size & 1)
    if not r.check(fmt is not None and data is not None, "WAV: a fmt chunk before the data",
                   "loadWav would read the samples with the default format"):
        return
    tag, channels, rate, _, _, bits = fmt
    at, size = data
    r.check(tag == 1 and bits == 16, f"WAV: format {tag}, {bits}-bit", "the game reads 16-bit PCM only")
    r.check(channels == 1, f"WAV: {channels} channel{'s' if channels != 1 else ''}",
            "the set is mono (stereo plays, but costs twice the memory)")
    if rate != GAME_RATE:
        r.warn(f"WAV: {rate} Hz (the set is {GAME_RATE} Hz; the game plays any rate)")
    if not r.check(at + size <= len(b) and size % (2 * channels) == 0 and size > 0,
                   f"WAV: {size} bytes of samples inside the file", "the data runs past the end or is ragged"):
        return
    if tag != 1 or bits != 16:
        return
    x = struct.unpack_from(f"<{size // 2}h", b, at)[::channels]
    seconds = len(x) / rate
    peak = max(abs(v) for v in x)
    peak_db = 20 * math.log10(max(peak, 1) / 32768)
    r.check(-40.0 < peak_db <= -0.5, f"WAV: peak {peak_db:.1f} dBFS",
            "silent" if peak_db <= -40.0 else "no headroom (the set peaks at -1 dBFS)")
    name = os.path.splitext(os.path.basename(path))[0]
    loop = name.startswith("amb-") or name in GAME_LOOPS
    steps = sorted(abs(x[i] - x[i - 1]) for i in range(1, len(x)))
    typical = steps[int(0.999 * (len(steps) - 1))] if steps else 0
    if loop:
        seam = abs(x[0] - x[-1])
        r.ok(f"WAV: a loop of {seconds:.2f} s")
        if seam > max(typical, 64):
            r.warn(f"WAV: the loop's seam jumps {seam} (99.9% of its steps are under {typical}): a click each time round")
        else:
            r.ok(f"WAV: seamless (the wrap steps {seam}; 99.9% of its steps are under {typical})")
    else:
        if seconds > GAME_ONESHOT_SECONDS:
            r.warn(f"WAV: {seconds:.2f} s (one-shots are preloaded at boot; most are under a second)")
        else:
            r.ok(f"WAV: {seconds:.2f} s")
        if abs(x[-1]) > 64:  # (a start is masked by the sound's own attack; an end isn't)
            r.warn(f"WAV: ends mid-sound (last sample {x[-1]}): it may click as it stops")


def check_ogg(r: Report, path: str) -> None:
    """Music and stingers (romfs/music): Ogg Vorbis as Tremor reads it; the channels, the rate,
    the length, and a loop's LOOPSTART tag (tools/audio/make_loop.py)."""
    b = open(path, "rb").read()
    if not r.check(b[:4] == b"OggS", "Ogg: OggS page", repr(b[:4])):
        return
    segs = b[26]
    body = 27 + segs
    ident = b[body:body + 30]
    if not r.check(ident[:7] == b"\x01vorbis", "Ogg: a Vorbis stream", repr(ident[:7])):
        return
    version, channels, rate = struct.unpack_from("<IBI", ident, 7)
    r.check(version == 0 and channels in (1, 2), f"Ogg: Vorbis {version}, {channels} channel(s)")
    if rate != 32000:
        r.warn(f"Ogg: {rate} Hz (the music is 32000 Hz)")
    else:
        r.ok(f"Ogg: {rate} Hz")
    last = b.rfind(b"OggS")
    granule = struct.unpack_from("<q", b, last + 6)[0]
    seconds = granule / rate if rate else 0
    r.check(0.3 < seconds < 600, f"Ogg: {seconds:.2f} s")
    tags = b"LOOPSTART=" in b[:4096]
    r.ok(f"Ogg: {'a loop (LOOPSTART)' if tags else 'no loop tags (a stinger, or plays once)'}")


# ------------------------------------------------------------------------------ CGFX
DATA_DICTS = ["models", "textures", "luts", "materials", "shaders", "cameras", "lights", "fogs",
              "environments", "skeletal anims", "material anims", "visibility anims", "camera anims",
              "light anims", "emitters"]  # the DATA block's 15 dictionaries (3dbrew CGFX)


def dict_names(b: bytes, at: int) -> list[str]:
    if b[at:at + 4] != b"DICT":
        raise ValueError(f"no DICT at {at:#x}")
    n, = struct.unpack_from("<I", b, at + 8)
    names = []
    for k in range(n):
        e = at + 0x1C + k * 0x10
        rel, = struct.unpack_from("<i", b, e + 8)
        s = e + 8 + rel
        names.append(b[s:b.index(b"\0", s)].decode("ascii", "replace"))
    return names


def check_cgfx(r: Report, b: bytes, limit: int = CGFX_LIMIT) -> None:
    if not r.check(b[:4] == b"CGFX", "CGFX: magic", repr(b[:4])):
        return
    r.check(len(b) <= limit, f"CGFX: {len(b) // 1024} KB of {limit // 1024}", "too big for a banner")
    if not r.check(b[0x14:0x18] == b"DATA", "CGFX: DATA block"):
        return
    found = {}
    for k, kind in enumerate(DATA_DICTS):
        count, rel = struct.unpack_from("<Ii", b, 0x1C + k * 8)
        if count:
            try:
                found[kind] = dict_names(b, 0x1C + k * 8 + 4 + rel)
            except (ValueError, IndexError) as e:
                r.fail(f"CGFX: the {kind} dictionary is unreadable ({e})")
    r.check(found.get("models") == ["COMMON"], f"CGFX: model {found.get('models')}",
            "a banner's one model must be named COMMON")
    anims = found.get("skeletal anims")
    if anims is not None:
        r.check(anims == ["COMMON"], f"CGFX: skeletal animation {anims}", "must be named COMMON")
    r.ok("CGFX: " + "; ".join(f"{k} {len(v)}" for k, v in found.items()))


def check_gltf(r: Report, path: str) -> None:
    import json
    g = json.load(open(path, encoding="utf-8"))
    r.check(not g.get("skins"), f"glTF: skins {len(g.get('skins', []))}", "a skinned banner freezes the HOME Menu")
    bad = [m.get("name") for m in g.get("meshes", []) for p in m["primitives"]
           if any(a.startswith(("JOINTS_", "WEIGHTS_")) for a in p["attributes"])]
    r.check(not bad, "glTF: no bone indices or weights", f"in {', '.join(map(str, bad))}")
    for a in g.get("animations", []):
        for ch in a["channels"]:
            path = ch["target"]["path"]
            if path == "pointer":  # a material animation: only the colour ones have run on the 3DS
                ptr = ch["target"]["extensions"]["KHR_animation_pointer"]["pointer"]
                r.check(ptr.startswith("/materials/") and ptr.endswith(("baseColorFactor", "emissiveFactor")),
                        f"glTF: {a.get('name')} animates {ptr}", "only material colours have run on the 3DS (labs J-R)")
            elif path not in ("translation", "rotation", "scale"):
                r.fail(f"glTF: animation {a.get('name')} drives {path} (only TRS is safe)")


# ------------------------------------------------------------------------------ banner
def check_banner(r: Report, b: bytes, sound_out: str | None, label: str) -> bool:
    """Returns whether the banner is 3D (a CGFX not made from a flat PNG)."""
    if not r.check(b[:4] == b"CBMD", "banner: CBMD magic", repr(b[:4])):
        return False
    cgfx_at, = struct.unpack_from("<I", b, 0x08)
    cwav_at, = struct.unpack_from("<I", b, 0x84)
    try:
        cgfx = lz11_decompress(b[cgfx_at:])
    except (ValueError, IndexError) as e:
        r.fail(f"banner: the CGFX doesn't decompress ({e})")
        return False
    r.ok(f"banner: CGFX {len(b[cgfx_at:cwav_at or len(b)]) // 1024} KB compressed")
    check_cgfx(r, cgfx)
    if cwav_at:
        check_cwav(r, b, cwav_at, sound_out, label)
    else:
        r.warn("banner: no sound")
    # bannertool's flat banner is a CGFX holding one 256 x 128 texture and a plane.
    return len(cgfx) > 0x30000


# ------------------------------------------------------------------------------ NCCH
def check_ivfc(r: Report, rom: bytes) -> None:
    if not r.check(rom[:4] == b"IVFC", "RomFS: IVFC magic", repr(rom[:4])):
        return
    mhs, = struct.unpack_from("<I", rom, 0x08)
    lv = []
    for k in range(3):
        off, size, bs = struct.unpack_from("<QQI", rom, 0x0C + k * 0x18)
        lv.append((off, size, 1 << bs))
    master = rom[0x60:0x60 + mhs]
    # Physical order: level 3 (the files), then levels 1 and 2, each aligned to its block.
    l3 = align(0x60 + mhs, lv[2][2])
    l1 = align(l3 + lv[2][1], lv[2][2])
    l2 = align(l1 + lv[0][1], lv[0][2])
    places = [(l1, lv[0]), (l2, lv[1]), (l3, lv[2])]
    hashes = master
    for k, (at, (_, size, bs)) in enumerate(places):
        nblocks = (size + bs - 1) // bs
        bad = 0
        for i in range(nblocks):
            block = rom[at + i * bs:at + min((i + 1) * bs, size)]
            block += b"\0" * (bs - len(block))
            if sha(block) != hashes[i * 32:(i + 1) * 32]:
                bad += 1
        if not r.check(bad == 0, f"RomFS: IVFC level {k + 1}, {nblocks} blocks", f"{bad} blocks don't match"):
            return
        hashes = rom[at:at + size]


def check_logo(r: Report, region: bytes, from_header: int) -> None:
    r.check(len(region) == 0x2000, f"logo: region {len(region):#x} bytes (header says {from_header:#x})",
            "GBATEK: the logo is always padded to 0x2000")
    try:
        arc = lz11_decompress(region)
    except (ValueError, IndexError) as e:
        r.fail(f"logo: doesn't decompress ({e})")
        return
    if not r.check(arc[:4] == b"darc", "logo: a darc", repr(arc[:4])):
        return
    body, = struct.unpack_from("<I", arc, 0x0C)
    r.check(len(arc) == body + 0x20, f"logo: darc {body} bytes + a 0x20-byte HMAC", f"{len(arc)} bytes decompressed")
    h = sha(arc).hex()
    known = KNOWN_LOGOS.get(h)
    r.check(known is not None, f"logo: {known or 'unknown (' + h[:16] + '...)'}",
            "only logos known to work pass: a redrawn logo's HMAC can't be made valid (0.1.8, run 9)")


def check_ncch(r: Report, n: bytes, title_id: int | None, sound_out: str | None, label: str) -> None:
    if not r.check(n[0x100:0x104] == b"NCCH", "NCCH: magic", repr(n[0x100:0x104])):
        return
    size, = struct.unpack_from("<I", n, 0x104)
    r.check(size * MEDIA == len(n), f"NCCH: {len(n) / 2**20:.2f} MB, as its header says")
    pid, = struct.unpack_from("<Q", n, 0x118)
    product = n[0x150:0x160].split(b"\0")[0].decode("ascii", "replace")
    r.ok(f"NCCH: program {pid:016X}, product code {product}")
    if title_id is not None:
        r.check(pid == title_id, "NCCH: program ID matches the TMD's title", f"{pid:016X} vs {title_id:016X}")
    r.check((pid >> 32) == 0x00040000 and 0x300 <= (pid >> 8 & 0xFFFFF) < 0xF7FFF,
            "NCCH: an application ID in the homebrew-safe range", f"{pid:016X}")
    flags = n[0x188:0x190]
    if not r.check(flags[7] & 0x4, "NCCH: unencrypted (NoCrypto), so it can be checked",
                   "encrypted content can't be checked here"):
        return
    exh_size, = struct.unpack_from("<I", n, 0x180)
    r.check(sha(n[0x200:0x200 + exh_size]) == n[0x160:0x180], "NCCH: exheader hash")
    title = n[0x200:0x208].split(b"\0")[0].decode("ascii", "replace")
    r.ok(f'exheader: title "{title}"')
    exh_pid, = struct.unpack_from("<Q", n, 0x200 + 0x200)  # the access control info's program ID
    r.check(exh_pid == pid, "exheader: its program ID matches", f"{exh_pid:016X}")

    lo, ls = struct.unpack_from("<II", n, 0x198)
    if ls:
        region = n[lo * MEDIA:(lo + ls) * MEDIA]
        r.check(sha(region) == n[0x130:0x150], "NCCH: logo region hash")
        check_logo(r, region, ls * MEDIA)
    else:
        r.fail("NCCH: no logo region")

    eo, es, ehs = struct.unpack_from("<III", n, 0x1A0)
    exefs = n[eo * MEDIA:(eo + es) * MEDIA]
    r.check(sha(exefs[:ehs * MEDIA]) == n[0x1C0:0x1E0], "NCCH: ExeFS header hash")
    files = {}
    for k in range(10):
        name = exefs[k * 16:k * 16 + 8].split(b"\0")[0].decode("ascii", "replace")
        off, fsize = struct.unpack_from("<II", exefs, k * 16 + 8)
        if not name:
            continue
        data = exefs[0x200 + off:0x200 + off + fsize]
        want = exefs[0x1E0 - k * 32:0x200 - k * 32]
        r.check(sha(data) == want, f"ExeFS: {name} ({fsize} bytes) hash")
        files[name] = data
    for need in (".code", "icon", "banner"):
        r.check(need in files, f"ExeFS: has {need}")
    three_d = False
    if "banner" in files:
        three_d = check_banner(r, files["banner"], sound_out, label)
    if "icon" in files:
        check_smdh(r, files["icon"], want_extended=three_d if "banner" in files else None)

    ro, rs, rhs = struct.unpack_from("<III", n, 0x1B0)
    if rs:
        rom = n[ro * MEDIA:(ro + rs) * MEDIA]
        r.check(sha(rom[:rhs * MEDIA]) == n[0x1E0:0x200], "NCCH: RomFS superblock hash")
        check_ivfc(r, rom)
    else:
        r.ok("NCCH: no RomFS (a banner-lab title)")


# ------------------------------------------------------------------------------ CIA
def check_cia(r: Report, b: bytes, sound_out: str | None, label: str) -> None:
    hsize, ctype, ver, certs, tik, tmd_size, meta = struct.unpack_from("<IHHIIII", b, 0)
    csize, = struct.unpack_from("<Q", b, 0x18)
    if not r.check(hsize == 0x2020, f"CIA: header ({hsize:#x})"):
        return
    at_cert = align(hsize, 64)
    at_tik = align(at_cert + certs, 64)
    at_tmd = align(at_tik + tik, 64)
    at_content = align(at_tmd + tmd_size, 64)
    at_meta = align(at_content + csize, 64)
    end = at_meta + meta if meta else at_content + csize
    r.check(end <= len(b) <= align(end, 64), f"CIA: sections fit the file ({len(b)} bytes)", f"expected {end}")

    tik_b = b[at_tik:at_tik + tik]
    tmd = b[at_tmd:at_tmd + tmd_size]
    sig, = struct.unpack_from(">I", tmd, 0)
    if not r.check(sig == 0x10004, "TMD: RSA-2048 signature type"):
        return
    body = 0x140
    tid, = struct.unpack_from(">Q", tmd, body + 0x4C)
    tver, count = struct.unpack_from(">HH", tmd, body + 0x9C)
    r.ok(f"TMD: title {tid:016X}, version {tver >> 10}.{tver >> 4 & 0x3F}.{tver & 0xF}, {count} content")
    tik_tid, = struct.unpack_from(">Q", tik_b, 0x140 + 0x9C)
    r.check(tik_tid == tid, "ticket: title ID matches the TMD", f"{tik_tid:016X}")
    info = tmd[body + 0xC4:body + 0xC4 + 64 * 0x24]
    r.check(sha(info) == tmd[body + 0xA4:body + 0xC4], "TMD: content info records hash")
    chunks = tmd[body + 0xC4 + 64 * 0x24:body + 0xC4 + 64 * 0x24 + count * 0x30]
    first_n, = struct.unpack_from(">H", info, 2)
    r.check(sha(chunks[:first_n * 0x30]) == info[4:0x24], "TMD: content chunk records hash")
    at = at_content
    for k in range(count):
        cid, idx, ctype_, size = struct.unpack_from(">IHHQ", chunks, k * 0x30)
        h = chunks[k * 0x30 + 0x10:k * 0x30 + 0x30]
        content = b[at:at + size]
        if ctype_ & 1:
            r.warn(f"content {idx}: encrypted with the title key, not checked")
        else:
            r.check(sha(content) == h, f"content {idx}: {size / 2**20:.2f} MB, hash")
            if idx == 0:
                check_ncch(r, content, tid, sound_out, label)
        at = align(at + size, 64)


# ------------------------------------------------------------------------------ 3DSX
def check_3dsx(r: Report, b: bytes) -> None:
    if not r.check(b[:4] == b"3DSX", "3DSX: magic", repr(b[:4])):
        return
    hsize, rsize, _, _, code, rodata, data, bss = struct.unpack_from("<HHIIIIII", b, 4)
    if not r.check(hsize >= 0x2C, "3DSX: extended header (SMDH and RomFS)", f"header {hsize:#x}"):
        return
    smdh_at, smdh_size, romfs_at = struct.unpack_from("<III", b, 0x20)
    relocs = [struct.unpack_from("<II", b, hsize + k * rsize) for k in range(3)]
    body_end = hsize + 3 * rsize + code + rodata + (data - bss) + 4 * sum(a + c for a, c in relocs)
    r.check(body_end == smdh_at, f"3DSX: code {code}, rodata {rodata}, data {data} (bss {bss}) and relocations add up",
            f"end {body_end:#x}, SMDH at {smdh_at:#x}")
    check_smdh(r, b[smdh_at:smdh_at + smdh_size])
    if romfs_at:
        hl, = struct.unpack_from("<I", b, romfs_at)
        r.check(hl == 0x28 and romfs_at < len(b), f"3DSX: RomFS at {romfs_at:#x}", f"header length {hl:#x}")
    else:
        r.warn("3DSX: no RomFS")


# ------------------------------------------------------------------------------ main
def check(path: str, quiet: bool, sound_out: str | None) -> Report:
    r = Report(path, quiet)
    label = os.path.splitext(os.path.basename(path))[0]
    ext = os.path.splitext(path)[1].lower()
    if ext == ".wav":
        # the game's own sounds live in romfs; any other WAV is taken for the banner's
        if "romfs" in os.path.normpath(os.path.abspath(path)).split(os.sep):
            check_game_wav(r, path)
        else:
            check_wav(r, path)
        return r
    if ext == ".ogg":
        check_ogg(r, path)
        return r
    if ext == ".gltf":
        check_gltf(r, path)
        return r
    b = open(path, "rb").read()
    try:
        if ext == ".cia":
            check_cia(r, b, sound_out, label)
        elif ext == ".3dsx":
            check_3dsx(r, b)
        elif b[:4] == b"CBMD":
            check_banner(r, b, sound_out, label)
        elif b[:4] == b"SMDH":
            check_smdh(r, b)
        elif b[:4] == b"CGFX":
            check_cgfx(r, b)
        else:
            r.fail(f"not a file this knows ({b[:4]!r})")
    except (struct.error, IndexError, ValueError, UnicodeDecodeError) as e:
        r.fail(f"unreadable: {type(e).__name__}: {e}")
    return r


def main() -> None:
    args = sys.argv[1:]
    quiet = "--quiet" in args
    sound_out = None
    if "--sound-out" in args:
        i = args.index("--sound-out")
        sound_out = args[i + 1]
        del args[i:i + 2]
    paths = [a for a in args if not a.startswith("--")]
    if not paths:
        sys.exit(__doc__)
    fails = warns = 0
    for p in paths:
        rep = check(p, quiet, sound_out)
        fails += rep.fails
        warns += rep.warns
        print(f"  => {'FAIL' if rep.fails else 'ok'} ({rep.fails} failed, {rep.warns} warnings)")
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
