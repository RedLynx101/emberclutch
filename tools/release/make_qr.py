#!/usr/bin/env python3
"""A QR code for FBI's remote install (Remote Install -> Scan QR Code): the release's CIA link as
a PNG, pure Python (no libraries to install).

  python tools/release/make_qr.py <url> <out.png> [--scale 8]

Byte mode, versions 1-10, error correction M (L if M doesn't fit), the best of the eight masks
by the standard's penalty rules. It checks itself before writing: the format bits against the
standard's table, every block's Reed-Solomon syndromes, and the data read back out of the
finished symbol.
"""
import struct
import sys
import zlib

# ---- GF(256) with the QR polynomial x^8 + x^4 + x^3 + x^2 + 1
EXP = [0] * 512
LOG = [0] * 256
_x = 1
for _i in range(255):
    EXP[_i] = _x
    LOG[_x] = _i
    _x <<= 1
    if _x & 0x100:
        _x ^= 0x11D
for _i in range(255, 512):
    EXP[_i] = EXP[_i - 255]


def gmul(a, b):
    return 0 if a == 0 or b == 0 else EXP[LOG[a] + LOG[b]]


def rs_generator(n):
    g = [1]
    for i in range(n):
        g = [a ^ gmul(b, EXP[i]) for a, b in zip(g + [0], [0] + g)]
    return g  # highest power first


def rs_ec(data, n):
    g = rs_generator(n)
    rem = list(data) + [0] * n
    for i in range(len(data)):
        c = rem[i]
        if c:
            for j in range(1, len(g)):
                rem[i + j] ^= gmul(g[j], c)
    return rem[len(data):]


def rs_syndromes_zero(codeword, n):
    for i in range(n):
        s = 0
        for c in codeword:
            s = gmul(s, EXP[i]) ^ c
        if s:
            return False
    return True


# ---- the standard's tables (versions 1-10): total codewords, and per level the EC codewords per
# block and the block groups (count, data codewords)
BLOCKS = {
    1: {"L": (7, [(1, 19)]), "M": (10, [(1, 16)])},
    2: {"L": (10, [(1, 34)]), "M": (16, [(1, 28)])},
    3: {"L": (15, [(1, 55)]), "M": (26, [(1, 44)])},
    4: {"L": (20, [(1, 80)]), "M": (18, [(2, 32)])},
    5: {"L": (26, [(1, 108)]), "M": (24, [(2, 43)])},
    6: {"L": (18, [(2, 68)]), "M": (16, [(4, 27)])},
    7: {"L": (20, [(2, 78)]), "M": (18, [(4, 31)])},
    8: {"L": (24, [(2, 97)]), "M": (22, [(2, 38), (2, 39)])},
    9: {"L": (30, [(2, 116)]), "M": (22, [(3, 36), (2, 37)])},
    10: {"L": (18, [(2, 68), (2, 69)]), "M": (26, [(4, 43), (1, 44)])},
}
ALIGN = {1: [], 2: [6, 18], 3: [6, 22], 4: [6, 26], 5: [6, 30], 6: [6, 34], 7: [6, 22, 38], 8: [6, 24, 42],
         9: [6, 26, 46], 10: [6, 28, 50]}
LEVEL_BITS = {"L": 1, "M": 0}
# The standard's format strings for level M, masks 0..7 (to check our BCH against)
FORMAT_M = ["101010000010010", "101000100100101", "101111001111100", "101101101001011", "100010111111001",
            "100000011001110", "100111110010111", "100101010100000"]


def format_bits(level, mask):
    data = (LEVEL_BITS[level] << 3) | mask
    v = data << 10
    for i in range(14, 9, -1):
        if v & (1 << i):
            v ^= 0x537 << (i - 10)
    return ((data << 10) | v) ^ 0x5412


def version_bits(version):
    v = version << 12
    for i in range(17, 11, -1):
        if v & (1 << i):
            v ^= 0x1F25 << (i - 12)
    return (version << 12) | v


def choose(data_len):
    for version in range(1, 11):
        for level in ("M", "L"):
            ec, groups = BLOCKS[version][level]
            capacity = sum(c * d for c, d in groups)
            count_bits = 8 if version < 10 else 16
            if 4 + count_bits + 8 * data_len <= 8 * capacity:
                return version, level
    raise SystemExit("error: too long for versions 1-10")


def encode_data(payload, version, level):
    ec, groups = BLOCKS[version][level]
    capacity = sum(c * d for c, d in groups)
    bits = []

    def put(v, n):
        bits.extend((v >> (n - 1 - i)) & 1 for i in range(n))
    put(0b0100, 4)
    put(len(payload), 8 if version < 10 else 16)
    for b in payload:
        put(b, 8)
    put(0, min(4, 8 * capacity - len(bits)))
    while len(bits) % 8:
        bits.append(0)
    data = [int("".join(map(str, bits[i:i + 8])), 2) for i in range(0, len(bits), 8)]
    pad = [0xEC, 0x11]
    while len(data) < capacity:
        data.append(pad[(len(data) - len(bits) // 8) % 2])
    blocks, k = [], 0
    for count, size in groups:
        for _ in range(count):
            blocks.append(data[k:k + size])
            k += size
    ecs = [rs_ec(b, ec) for b in blocks]
    for b, e in zip(blocks, ecs):
        assert rs_syndromes_zero(b + e, ec), "Reed-Solomon check failed"
    out = []
    for i in range(max(len(b) for b in blocks)):
        out += [b[i] for b in blocks if i < len(b)]
    for i in range(ec):
        out += [e[i] for e in ecs]
    return out, data


def build(payload):
    version, level = choose(len(payload))
    n = 17 + 4 * version
    M = [[None] * n for _ in range(n)]  # None: data; 0/1: function module
    fixed = [[False] * n for _ in range(n)]

    def setf(r, c, v):
        M[r][c] = v
        fixed[r][c] = True

    def finder(r0, c0):
        for r in range(-1, 8):
            for c in range(-1, 8):
                rr, cc = r0 + r, c0 + c
                if 0 <= rr < n and 0 <= cc < n:
                    on = 0 <= r <= 6 and 0 <= c <= 6 and (r in (0, 6) or c in (0, 6) or (2 <= r <= 4 and 2 <= c <= 4))
                    setf(rr, cc, 1 if on else 0)
    finder(0, 0)
    finder(0, n - 7)
    finder(n - 7, 0)
    for i in range(8, n - 8):
        setf(6, i, 1 - i % 2)
        setf(i, 6, 1 - i % 2)
    pos = ALIGN[version]
    for i, r in enumerate(pos):
        for j, c in enumerate(pos):
            if (i == 0 and j == 0) or (i == 0 and j == len(pos) - 1) or (i == len(pos) - 1 and j == 0):
                continue  # (under the finders)
            for dr in range(-2, 3):
                for dc in range(-2, 3):
                    setf(r + dr, c + dc, 1 if max(abs(dr), abs(dc)) != 1 else 0)
    setf(4 * version + 9, 8, 1)  # the dark module
    for i in range(9):  # format areas, reserved
        if not fixed[8][i]:
            setf(8, i, 0)
        if not fixed[i][8]:
            setf(i, 8, 0)
    for i in range(8):
        setf(8, n - 1 - i, 0)
    for i in range(7):  # (not the dark module above them)
        setf(n - 1 - i, 8, 0)
    if version >= 7:
        vb = version_bits(version)
        for i in range(18):
            b = (vb >> i) & 1
            setf(i // 3, n - 11 + i % 3, b)
            setf(n - 11 + i % 3, i // 3, b)
    codewords, data = encode_data(payload, version, level)
    bits = [(cw >> (7 - i)) & 1 for cw in codewords for i in range(8)]
    order = []  # the zigzag, from the bottom right, two columns at a time, skipping column 6
    c, up = n - 1, True
    while c > 0:
        if c == 6:
            c -= 1
        rows = range(n - 1, -1, -1) if up else range(n)
        for r in rows:
            for cc in (c, c - 1):
                if not fixed[r][cc]:
                    order.append((r, cc))
        up = not up
        c -= 2
    for k, (r, cc) in enumerate(order):
        M[r][cc] = bits[k] if k < len(bits) else 0
    masks = [lambda r, c: (r + c) % 2 == 0, lambda r, c: r % 2 == 0, lambda r, c: c % 3 == 0,
             lambda r, c: (r + c) % 3 == 0, lambda r, c: (r // 2 + c // 3) % 2 == 0,
             lambda r, c: (r * c) % 2 + (r * c) % 3 == 0, lambda r, c: ((r * c) % 2 + (r * c) % 3) % 2 == 0,
             lambda r, c: ((r + c) % 2 + (r * c) % 3) % 2 == 0]

    def with_mask(m):
        Q = [row[:] for row in M]
        for r in range(n):
            for cc in range(n):
                if not fixed[r][cc] and masks[m](r, cc):
                    Q[r][cc] ^= 1
        fb = format_bits(level, m)
        fbits = [(fb >> (14 - i)) & 1 for i in range(15)]
        # around the top left: row 8 left to right (skipping the timing column), then column 8 up
        spots = [(8, 0), (8, 1), (8, 2), (8, 3), (8, 4), (8, 5), (8, 7), (8, 8), (7, 8), (5, 8), (4, 8), (3, 8), (2, 8),
                 (1, 8), (0, 8)]
        for (r, cc), b in zip(spots, fbits):
            Q[r][cc] = b
        # and split between the other two corners
        spots2 = [(n - 1 - i, 8) for i in range(7)] + [(8, n - 8 + i) for i in range(8)]
        for (r, cc), b in zip(spots2, fbits):
            Q[r][cc] = b
        return Q

    def penalty(Q):
        p = 0
        for grid in (Q, [list(col) for col in zip(*Q)]):
            for row in grid:
                run, last = 0, None
                for v in row:
                    if v == last:
                        run += 1
                    else:
                        if run >= 5:
                            p += run - 2
                        run, last = 1, v
                if run >= 5:
                    p += run - 2
                s = "".join(map(str, row))
                p += 40 * (s.count("10111010000") + s.count("00001011101"))
        for r in range(n - 1):
            for cc in range(n - 1):
                if Q[r][cc] == Q[r + 1][cc] == Q[r][cc + 1] == Q[r + 1][cc + 1]:
                    p += 3
        dark = sum(map(sum, Q)) * 100 / (n * n)
        p += int(abs(dark - 50) // 5) * 10
        return p
    best = min(range(8), key=lambda m: penalty(with_mask(m)))
    Q = with_mask(best)
    # Read it back: format from the top-left copy, unmask, zigzag, deinterleave.
    got = 0
    spots = [(8, 0), (8, 1), (8, 2), (8, 3), (8, 4), (8, 5), (8, 7), (8, 8), (7, 8), (5, 8), (4, 8), (3, 8), (2, 8),
             (1, 8), (0, 8)]
    for r, cc in spots:
        got = (got << 1) | Q[r][cc]
    assert got == format_bits(level, best)
    read = [Q[r][cc] ^ (1 if masks[best](r, cc) else 0) for r, cc in order]
    cws = [int("".join(map(str, read[i:i + 8])), 2) for i in range(0, len(codewords) * 8, 8)]
    assert cws == codewords, "read-back mismatch"
    return Q, version, level, best, data


def write_png(Q, path, scale=8, quiet=4):
    n = len(Q)
    size = (n + 2 * quiet) * scale
    rows = []
    for y in range(size):
        r = y // scale - quiet
        line = bytearray([0])
        for x in range(size):
            c = x // scale - quiet
            dark = 0 <= r < n and 0 <= c < n and Q[r][c]
            line.append(0 if dark else 255)
        rows.append(bytes(line))
    raw = b"".join(rows)

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 0, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    open(path, "wb").write(png)


def main():
    args = sys.argv[1:]
    if len(args) < 2:
        raise SystemExit(__doc__)
    for m, want in enumerate(FORMAT_M):  # our BCH against the standard's table
        assert format(format_bits("M", m), "015b") == want, (m, format(format_bits("M", m), "015b"), want)
    scale = int(args[args.index("--scale") + 1]) if "--scale" in args else 8
    Q, version, level, mask, _ = build(args[0].encode("utf-8"))
    write_png(Q, args[1], scale)
    print(f"[qr] {args[1]}: version {version}-{level}, mask {mask}, {len(Q)} modules, {args[0]}")


if __name__ == "__main__":
    main()
