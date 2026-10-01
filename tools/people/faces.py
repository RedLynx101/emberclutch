"""The feelings kit's faces (D138, the Living Valley pass): every person's eyes, mouth and brows
as part groups of their own, one variant drawn at a time, so a line's feeling shows on the face
of whoever says it (app/emotes picks them; core/people faceFor maps the twenty feelings).

  eyes   (group 0, on the eyes bone: the blink squashes them)  EYES below
  mouth  (group 11, on the head)                               MOUTHS
  brows  (group 12, on the head; empty for someone without)    BROWS

face(p, ...) builds them all from the person's own measurements: where the eyes sit
(p.eyes_at), their size, the mouth's height and width, the brows' shape. Sizes scale with the
head (p.head_k: the Storybook head is 0.8 of the old one).
"""
import math

from body import _decal_at
from geom import decal, rigid, strip

EYES = ["calm", "smile", "shut", "wide", "sad", "angry", "sleepy", "hearts", "sparkle", "spiral"]
MOUTHS = ["smile", "open", "wide", "o", "frown", "flat", "pout", "smirk", "wobble", "yawn"]
BROWS = ["calm", "raised", "angry", "worried"]

HEAD = rigid("head")


def _arc(n, width, f):
    """n points across `width` (u), v = f(t) for t in -1..1."""
    return [(width / 2 * t, f(t)) for t in (-1 + 2 * i / (n - 1) for i in range(n))]


def _eyes(p, size, depth, pupil, lashes):
    h = p.head
    rx, ry = size
    w = rigid("eyes")
    lift = 0.0015
    line_lift = depth + 0.0035  # strokes over an iris clear its dome
    line = 0.0075 * p.head_k

    def iris(m, az, el, sx=1.0, sy=1.0, dv=0.0, pxs=pupil[0], pys=pupil[1], glints=((0.34, 0.36, 0.3, 5), (-0.3, -0.34, 0.14, 4))):
        """An iris (scaled sx, sy and dropped dv), its pupil and glints: the calm eye's pieces."""
        erx, ery = rx * sx, ry * sy

        def ih(u, v):
            r = math.sqrt((u / erx) ** 2 + ((v - dv) / ery) ** 2)
            return depth * max(0.0, 1 - r)

        _decal_at(m, h, az, el, 0.0, dv, erx, ery, depth, 9, "iris", w, base=lambda u, v: 0.0, lift=lift)
        prx, pry = erx * pxs, ery * pys
        pdv = dv - (0.1 * ery if pxs > 0.5 else 0.0)  # (calm pupils sit a touch low)
        pd = 0.0025
        _decal_at(m, h, az, el, 0.0, pdv, prx, pry, pd, 7, "pupil", w, base=lambda u, v: ih(u, v) + 0.0006, lift=lift)

        def gb(u, v):
            r = math.sqrt((u / prx) ** 2 + ((v - pdv) / pry) ** 2)
            return ih(u, v) + (pd * math.sqrt(max(0.0, 1 - r * r)) if r < 1 else 0.0) + 0.0012

        for gu, gv, gr, gs in glints:
            gw = erx * gr
            _decal_at(m, h, az, el, gu * erx, dv + gv * ery, gw, gw * 1.05, 0.0015, gs, "glint", w, base=gb, lift=lift)

    def stroke(m, az, el, pts, thick=None, mat="pupil", lift_=None):
        strip(m, h, pts, az, el, [thick or line] * len(pts), mat, w, lift=lift_ if lift_ is not None else 0.0025,
              depth=0.0015)

    for s in (-1, 1):
        az, el = s * p.eyes_at[0], p.eyes_at[1]
        inner = -s  # toward the nose, in the tangent plane's u
        for name, m in zip(EYES, p.eyes):
            if name == "calm":
                iris(m, az, el)
            elif name == "wide":
                iris(m, az, el, 1.08, 1.08, pxs=0.36, pys=0.38)
            elif name == "sparkle":
                iris(m, az, el, glints=((0.32, 0.34, 0.36, 5), (-0.3, -0.32, 0.18, 4), (0.36, -0.3, 0.12, 3)))
            elif name == "smile":  # happy arcs, the storybook squint
                stroke(m, az, el, _arc(6, rx * 1.8, lambda t: ry * (0.32 * (1 - t * t) - 0.12)), line * 1.1)
            elif name == "shut":  # squeezed shut: > <, each pointing in to the nose
                stroke(m, az, el, [(-inner * rx * 0.75, ry * 0.42), (inner * rx * 0.55, 0.0), (-inner * rx * 0.75, -ry * 0.42)],
                       line * 1.05)
            elif name in ("sad", "angry"):  # a smaller eye under a slanted lid
                iris(m, az, el, 0.92, 0.72, dv=-ry * 0.2)
                hi, lo = ry * 0.6, ry * 0.18
                a, b = (hi, lo) if name == "sad" else (lo, hi)  # sad: high by the nose; angry: low by it
                stroke(m, az, el, [(inner * rx * 1.0, a), (-inner * rx * 1.0, b)], line * 1.1, lift_=line_lift)
            elif name == "sleepy":  # half shut
                iris(m, az, el, 0.95, 0.5, dv=-ry * 0.32)
                stroke(m, az, el, _arc(3, rx * 2.0, lambda t: ry * (0.02 - 0.05 * t * t)), line, lift_=line_lift)
            elif name == "hearts":
                lobe = rx * 0.5
                for du in (-0.42, 0.42):
                    _decal_at(m, h, az, el, du * rx, ry * 0.22, lobe, lobe, 0.002, 6, "heart", w,
                              base=lambda u, v: 0.0, lift=lift)
                strip(m, h, [(0.0, ry * 0.22), (0.0, -ry * 0.25), (0.0, -ry * 0.68)], az, el,
                      [rx * 1.78, rx * 0.95, rx * 0.06], "heart", w, lift=lift + 0.0006, depth=0.002)
            elif name == "spiral":  # dizzy: a dark spiral on white
                decal(m, h, az, el, rx * 0.95, ry * 0.95, depth * 0.6, 9, "white", w, lift=lift)
                pts = []
                for i in range(11):
                    a = i / 10 * 2.1 * 2 * math.pi
                    r = 0.08 + 0.78 * i / 10
                    pts.append((s * rx * r * math.cos(a), ry * r * math.sin(a)))
                stroke(m, az, el, pts, line * 0.8, lift_=depth * 0.6 + 0.003)
        if lashes:  # on the open eyes (calm, wide, sparkle, sad, angry)
            for name, m in zip(EYES, p.eyes):
                if name not in ("calm", "wide"):
                    continue
                sx = 1.08 if name == "wide" else 1.0
                pts = [(rx * sx * (-0.95 + 1.9 * i / 4), ry * sx * 0.9 * math.sqrt(max(0.0, 1 - (-0.95 + 1.9 * i / 4) ** 2)))
                       for i in range(5)]
                strip(m, h, pts, az, el, [0.006 * p.head_k] * 5, "pupil", w, lift=0.003)


def _mouths(p, el, width, curve, thick):
    h, k = p.head, p.head_k
    k2 = max(k, 0.95)  # (open mouths read a little bigger than the head's scale on the small screen)
    lip = 0.0015

    def line(m, pts, th, e=el):
        strip(m, h, pts, 0, e, th, "mouth", HEAD, lift=lip, depth=0.0015)

    def taper(n, t0=1.0):
        return [thick * (1 - 0.55 * (-1 + 2 * i / (n - 1)) ** 2) * t0 for i in range(n)]

    def hole(m, rx, ry, drop, flat_top, tongue=True):
        """An open mouth: an ellipse (its top flattened `flat_top`), a tongue at the bottom."""
        shape = (lambda a: 1.0 - flat_top * max(0.0, math.sin(a))) if flat_top else None
        decal(m, h, 0, el - drop, rx, ry, 0.001, 8, "mouth", HEAD, lift=lip, shape=shape)
        if tongue:
            _decal_at(m, h, 0, el - drop, 0.0, -ry * 0.45, rx * 0.55, ry * 0.32, 0.0008, 6, "tongue", HEAD,
                      base=lambda u, v: 0.0, lift=lip + 0.0012)

    for name, m in zip(MOUTHS, p.mouths):
        if name == "smile":
            line(m, _arc(4, width, lambda t: curve * t * t), taper(4))
        elif name == "open":
            hole(m, 0.025 * k2, 0.017 * k2, 1.4, 0.55)
        elif name == "wide":
            hole(m, 0.03 * k2, 0.025 * k2, 2.2, 0.4)
        elif name == "o":
            hole(m, 0.013 * k2, 0.016 * k2, 1.2, 0.0, tongue=False)
        elif name == "frown":
            line(m, _arc(4, width * 0.8, lambda t: -curve * 0.85 * t * t), taper(4), el - 0.8)
        elif name == "flat":
            line(m, _arc(3, width * 0.6, lambda t: 0.0), [thick * 0.8] * 3)
        elif name == "pout":
            line(m, _arc(3, width * 0.36, lambda t: -0.004 * k * t * t), [thick * 1.05] * 3)
        elif name == "smirk":
            line(m, _arc(4, width * 0.8, lambda t: curve * 1.1 * ((t + 1) / 2) ** 2 - curve * 0.15), taper(4))
        elif name == "wobble":
            line(m, [(width * 0.42 * (-1 + 2 * i / 6), 0.0032 * k * (1 if i % 2 else -1)) for i in range(7)],
                 [thick * 0.75] * 7, el - 0.6)
        elif name == "yawn":
            hole(m, 0.018 * k2, 0.027 * k2, 2.0, 0.0)


def _brows(p, mat, az=21.0, el=10.5, width=0.05, arch=0.008, thick=0.011, tilt=0.0, n=4):
    h, k = p.head, p.head_k

    def brow(m, el_, arch_, tilt_):
        for s in (-1, 1):
            pts, th = [], []
            for i in range(n):
                f = -1 + 2 * i / (n - 1)
                pts.append((f * width / 2, arch_ * (1 - f * f) + tilt_ * f * s))
                th.append(thick * (1 - 0.35 * f * f))
            strip(m, h, pts, s * az, el_, th, mat, HEAD, lift=0.002, depth=0.002)

    for name, m in zip(BROWS, p.brows):
        if name == "calm":
            brow(m, el, arch, tilt)
        elif name == "raised":
            brow(m, el + 3.5, arch * 1.5, tilt)
        elif name == "angry":  # their outer ends up, the inner down to the nose
            brow(m, el - 1.0, arch * 0.5, tilt + 0.011 * k)
        elif name == "worried":
            brow(m, el + 1.5, arch * 0.6, tilt - 0.01 * k)


def face(p, eye_size=(0.04, 0.055), depth=0.009, pupil=(0.72, 0.74), lashes=False, brows=None, brow_mat="hair",
         mouth=None):
    """Every variant of the eyes, mouth and brows (brows=None: none). mouth = dict(el, width, curve,
    thick) overriding the head's smile (el -29.5, 0.052 wide, curving 0.011, 0.0085 thick, scaled)."""
    k = p.head_k
    _eyes(p, eye_size, depth, pupil, lashes)
    mo = dict(el=-29.5, width=0.056 * max(k, 0.9), curve=0.012 * max(k, 0.9), thick=0.0095 * max(k, 0.9))
    mo.update(mouth or {})
    _mouths(p, **mo)
    if brows:
        _brows(p, brow_mat, **brows)


# Each feeling's face (core/story Feel order; gen_looks.py writes it for core/people faceFor), and the
# portraits' five (app/emotes portraitFrame: calm, happy, sad, angry, surprised).
FEELS = [  # (feeling, eyes, mouth, brows)
    ("calm", "calm", "smile", "calm"),
    ("happy", "smile", "open", "raised"),
    ("laugh", "shut", "wide", "raised"),
    ("excited", "sparkle", "open", "raised"),
    ("surprised", "wide", "o", "raised"),
    ("shock", "wide", "wide", "raised"),
    ("sad", "sad", "frown", "worried"),
    ("crying", "shut", "wobble", "worried"),
    ("angry", "angry", "frown", "angry"),
    ("huff", "angry", "pout", "angry"),
    ("worried", "calm", "wobble", "worried"),
    ("scared", "wide", "wobble", "worried"),
    ("sleepy", "sleepy", "yawn", "calm"),
    ("love", "hearts", "open", "raised"),
    ("proud", "smile", "smirk", "raised"),
    ("cool", "sleepy", "smirk", "calm"),
    ("shy", "smile", "smile", "worried"),
    ("thinking", "calm", "flat", "worried"),
    ("wistful", "sad", "smile", "worried"),
    ("dizzy", "spiral", "wobble", "worried"),
]
PORTRAIT_FEELS = ["calm", "happy", "sad", "angry", "surprised"]


def face_of(feel):
    """(eyes, mouth, brows) variant indices for a feeling's name."""
    for f, e, m, b in FEELS:
        if f == feel:
            return EYES.index(e), MOUTHS.index(m), BROWS.index(b)
    raise KeyError(feel)
