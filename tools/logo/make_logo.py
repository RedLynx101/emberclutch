"""Our own boot logo (Alpha 2 WP11c, D58, D66): makerom's "homebrew" logo, its layout and
animation kept, with the word "homebrew" swapped for EMBERCLUTCH. The HOME Menu plays it as
the game starts; the emulator never shows it.

  py -3.12 tools/logo/make_logo.py [--makerom <path>] [--review]

1. makerom builds a small CXI with `Logo: Homebrew`; its logo region is read back (makerom's
   own asset: taken from its output each time, never kept in this repo);
2. the LZ11-compressed darc is unpacked, and timg/logo.bclim (ETC1, 225 x 40 in 256 x 64) is
   drawn again: EMBERCLUTCH in Cinzel Decorative (the game's title face), white on black like
   the original, so the layout's material colours it the same way;
3. ETC1-encoded here (per 4 x 4 block: both flips, the 16 base levels, all 8 tables), put
   back in place (same size and format, so the darc keeps its layout), LZ11-compressed again
   and checked to fit makerom's 0x2000-byte logo region;
4. written to build/logo/emberclutch.bcma.lz, which tools/package_cia.ps1 passes with `-logo`.
   With --review, the old and new textures as PNGs in build/review/logo/.

A round trip checks the result: the written logo is decompressed, its texture decoded again
and compared with the drawing (PSNR).
"""
import math
import os
import struct
import subprocess
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT = os.path.join(ROOT, "build", "logo")
REVIEW = os.path.join(ROOT, "build", "review", "logo")
FONT = os.path.join(ROOT, "assets", "fonts", "cinzel-decorative", "CinzelDecorative-Bold.ttf")
WORD = "EMBERCLUTCH"
LOGO_LIMIT = 0x2000
MOD = [[2, 8, -2, -8], [5, 17, -5, -17], [9, 29, -9, -29], [13, 42, -13, -42], [18, 60, -18, -60],
       [24, 80, -24, -80], [33, 106, -33, -106], [47, 183, -47, -183]]


def arg(name, default=None):
    return sys.argv[sys.argv.index(name) + 1] if name in sys.argv else default


# ------------------------------------------------------------------------------ LZ11
def lz11_decompress(b):
    assert b[0] == 0x11, hex(b[0])
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
                for _ in range(n):
                    o.append(o[-disp])
            else:
                o.append(b[i])
                i += 1
    return bytes(o)


def lz11_compress(data):
    """Greedy LZ11 with hash chains (window 4096, matches 3..65808)."""
    out = bytearray([0x11, len(data) & 0xFF, len(data) >> 8 & 0xFF, len(data) >> 16 & 0xFF])
    heads, chain = {}, [-1] * len(data)
    i = 0
    n = len(data)

    def insert(p):
        if p + 3 <= n:
            k = data[p:p + 3]
            chain[p] = heads.get(k, -1)
            heads[k] = p

    while i < n:
        flag_at = len(out)
        out.append(0)
        flags = 0
        for bit in range(8):
            if i >= n:
                break
            best_len, best_disp = 0, 0
            if i + 3 <= n:
                cand = heads.get(data[i:i + 3], -1)
                tries = 0
                while cand >= 0 and i - cand <= 4096 and tries < 64:
                    length = 0
                    limit = min(0x10110, n - i)
                    while length < limit and data[cand + length] == data[i + length]:
                        length += 1
                    if length > best_len:
                        best_len, best_disp = length, i - cand
                        if length == limit:
                            break
                    cand = chain[cand]
                    tries += 1
            if best_len >= 3:
                flags |= 0x80 >> bit
                d = best_disp - 1
                if best_len <= 0x10:
                    out += bytes([(best_len - 1) << 4 | d >> 8, d & 0xFF])
                elif best_len <= 0x110:
                    m = best_len - 0x11
                    out += bytes([m >> 4, (m & 0xF) << 4 | d >> 8, d & 0xFF])
                else:
                    m = best_len - 0x111
                    out += bytes([0x10 | m >> 12, m >> 4 & 0xFF, (m & 0xF) << 4 | d >> 8, d & 0xFF])
                for p in range(i, i + best_len):
                    insert(p)
                i += best_len
            else:
                out.append(data[i])
                insert(i)
                i += 1
        out[flag_at] = flags
    while len(out) % 4:
        out.append(0)
    return bytes(out)


# ------------------------------------------------------------------------------ darc
def darc_files(arc):
    assert arc[:4] == b"darc", arc[:4]
    ftab, = struct.unpack_from("<I", arc, 0x10)
    n = struct.unpack_from("<III", arc, ftab)[2]
    names = ftab + n * 12

    def name_at(off):
        s = arc[names + off:]
        end = 0
        while s[end:end + 2] != b"\0\0":
            end += 2
        return s[:end].decode("utf-16le")

    files, ends = {}, []
    for k in range(n):
        no, a, b = struct.unpack_from("<III", arc, ftab + k * 12)
        nm = name_at(no & 0xFFFFFF)
        while ends and k >= ends[-1][0]:
            ends.pop()
        prefix = ends[-1][1] if ends else ""
        if no & 0x01000000:
            ends.append((b, prefix + nm + "/"))
        else:
            files[prefix + nm] = (a, b)
    return files


# ------------------------------------------------------------------------------ ETC1
def swizzle_blocks(w, h):
    """(x, y) of each 4 x 4 block in the order the 3DS stores them (8 x 8 tiles, Z order)."""
    for ty in range(h // 8):
        for tx in range(w // 8):
            for bx, by in ((0, 0), (1, 0), (0, 1), (1, 1)):
                yield tx * 8 + bx * 4, ty * 8 + by * 4


def etc1_decode_block(v):
    diff, flip = v >> 33 & 1, v >> 32 & 1
    t1, t2 = v >> 37 & 7, v >> 34 & 7
    if diff:
        def s3(x):
            return x - 8 if x & 4 else x
        base = [(v >> 59) & 31, (v >> 51) & 31, (v >> 43) & 31]
        dl = [s3(v >> 56 & 7), s3(v >> 48 & 7), s3(v >> 40 & 7)]
        e5 = lambda c: (c << 3) | (c >> 2)
        c1 = [e5(c) for c in base]
        c2 = [e5((b + d) & 31) for b, d in zip(base, dl)]
    else:
        c1 = [(v >> s & 15) * 17 for s in (60, 52, 44)]
        c2 = [(v >> s & 15) * 17 for s in (56, 48, 40)]
    out = {}
    for x in range(4):
        for y in range(4):
            i = x * 4 + y
            k = (v >> (16 + i) & 1) << 1 | (v >> i & 1)
            sub = (y >= 2) if flip else (x >= 2)
            c, t = (c2, t2) if sub else (c1, t1)
            out[(x, y)] = tuple(max(0, min(255, ch + MOD[t][k])) for ch in c)
    return out


def etc1_encode_block(px):
    """px[(x, y)] = grey level 0..255. Individual mode (4-bit bases), the best flip, base and
    table for each half; the block as the 64-bit value etc1_decode_block reads."""
    best = None
    for flip in (0, 1):
        halves = []
        for sub in (0, 1):
            pts = [(x, y) for x in range(4) for y in range(4) if ((y >= 2) if flip else (x >= 2)) == bool(sub)]
            vals = [px[p] for p in pts]
            mean = sum(vals) / len(vals)
            h_best = None
            for base4 in range(max(0, int(mean / 17) - 2), min(16, int(mean / 17) + 3)):
                base = base4 * 17
                for t in range(8):
                    err, idx = 0, {}
                    for p, v in zip(pts, vals):
                        e, k = min(((max(0, min(255, base + MOD[t][k])) - v) ** 2, k) for k in range(4))
                        err += e
                        idx[p] = k
                    if h_best is None or err < h_best[0]:
                        h_best = (err, base4, t, idx)
            halves.append(h_best)
        total = halves[0][0] + halves[1][0]
        if best is None or total < best[0]:
            best = (total, flip, halves)
    _, flip, (h1, h2) = best
    v = 0
    for s, b in ((60, h1[1]), (52, h1[1]), (44, h1[1]), (56, h2[1]), (48, h2[1]), (40, h2[1])):
        v |= b << s
    v |= h1[2] << 37 | h2[2] << 34 | flip << 32  # diff bit 33 stays 0: individual mode
    for (x, y), k in list(h1[3].items()) + list(h2[3].items()):
        i = x * 4 + y
        v |= (k >> 1) << (16 + i) | (k & 1) << i
    return v


def etc1_encode(img):
    w, h = img.size
    g = img.convert("L").load()
    data = bytearray()
    for bx, by in swizzle_blocks(w, h):
        px = {(x, y): g[bx + x, by + y] for x in range(4) for y in range(4)}
        data += struct.pack("<Q", etc1_encode_block(px))
    return bytes(data)


def etc1_decode(data, w, h):
    img = Image.new("L", (w, h))
    p = img.load()
    for n, (bx, by) in enumerate(swizzle_blocks(w, h)):
        v, = struct.unpack_from("<Q", data, n * 8)
        for (x, y), c in etc1_decode_block(v).items():
            p[bx + x, by + y] = c[0]
    return img


# ------------------------------------------------------------------------------ the logo
def homebrew_logo(makerom):
    """makerom's own logo region, read back from a CXI it builds with the game's code."""
    os.makedirs(OUT, exist_ok=True)
    rsf = os.path.join(OUT, "logo_probe.rsf")  # the game's settings without its romfs
    text = open(os.path.join(ROOT, "tools", "cia.rsf"), encoding="utf-8").read()
    start = text.index("RomFs:")
    end = text.index("\n\n", start) + 2
    with open(rsf, "w", encoding="utf-8") as f:
        f.write(text[:start] + text[end:])
    elf = os.path.join(ROOT, "emberclutch.elf")
    cxi = os.path.join(OUT, "logo_probe.cxi")
    subprocess.run([makerom, "-f", "cxi", "-o", cxi, "-elf", elf, "-rsf", rsf, "-target", "t",
                    "-DAPP_ENCRYPTED=false"], check=True, capture_output=True)
    d = open(cxi, "rb").read()
    assert d[0x100:0x104] == b"NCCH"
    lo, ls = struct.unpack_from("<II", d, 0x198)
    region = d[lo * 0x200:(lo + ls) * 0x200]
    os.remove(cxi)
    return region


def wordmark(w, h, box_w, box_h):
    """EMBERCLUTCH, white on black, as big as fits box_w x box_h at the texture's top left
    (where "homebrew" sat)."""
    img = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(img)
    size = box_h
    while size > 8:
        font = ImageFont.truetype(FONT, size)
        l, t, r, b = d.textbbox((0, 0), WORD, font=font, stroke_width=1)
        if r - l <= box_w - 4 and b - t <= box_h - 4:
            break
        size -= 1
    x = (box_w - (r - l)) // 2 - l
    y = (box_h - (b - t)) // 2 - t
    d.text((x, y), WORD, fill=255, font=font, stroke_width=1, stroke_fill=255)  # a little bolder: it's small on the 3DS
    return img


def main():
    makerom = arg("--makerom", os.path.join(ROOT, "..", "3ds-ai", "tools", "win64", "makerom", "makerom.exe"))
    region = homebrew_logo(makerom)
    arc = bytearray(lz11_decompress(region))
    files = darc_files(arc)
    key = next(k for k in files if k.endswith("timg/logo.bclim"))
    off, size = files[key]
    clim = arc[off:off + size]
    w, h, fmt = struct.unpack_from("<HHI", clim, size - 0x14 + 8)
    assert fmt == 10, f"logo.bclim is format {fmt}, not ETC1"
    pw, ph = 1 << (w - 1).bit_length(), 1 << (h - 1).bit_length()
    data_len = pw * ph // 2
    old = etc1_decode(bytes(clim[:data_len]), pw, ph)
    drawn = wordmark(pw, ph, w, h)
    enc = etc1_encode(drawn)
    assert len(enc) == data_len
    arc[off:off + data_len] = enc
    packed = lz11_compress(bytes(arc))
    print(f"[logo] darc {len(arc)} bytes -> {len(packed)} compressed (limit {LOGO_LIMIT})")
    if len(packed) > LOGO_LIMIT:
        sys.exit("[logo] too big for the logo region")
    # The round trip: what the 3DS will read back.
    back = bytearray(lz11_decompress(packed))
    assert back == arc, "LZ11 round trip differs"
    again = etc1_decode(bytes(back[off:off + data_len]), pw, ph)
    mse = sum((a - b) ** 2 for a, b in zip(again.getdata(), drawn.getdata())) / (pw * ph)
    psnr = 99.0 if mse == 0 else 10 * math.log10(255 * 255 / mse)
    print(f"[logo] texture {w} x {h} ({pw} x {ph}), ETC1 round trip PSNR {psnr:.1f} dB")
    path = os.path.join(OUT, "emberclutch.bcma.lz")
    with open(path, "wb") as f:
        f.write(packed)
    print(f"[logo] wrote {path}")
    if "--review" in sys.argv:
        os.makedirs(REVIEW, exist_ok=True)
        for name, im in (("homebrew", old), ("emberclutch", again)):
            im.resize((pw * 3, ph * 3), Image.NEAREST).save(os.path.join(REVIEW, f"logo_{name}.png"))


if __name__ == "__main__":
    main()
