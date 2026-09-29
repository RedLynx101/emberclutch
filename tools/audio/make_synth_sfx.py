#!/usr/bin/env python3
"""Cute synthesised sound effects and ambient beds for 1.0: romfs/sfx/<slug>[-N].wav.

  python tools/audio/make_synth_sfx.py                     (every sound and bed)
  python tools/audio/make_synth_sfx.py hit plop amb-cove   (just these)
  python tools/audio/make_synth_sfx.py --list              (the slugs, and how each should sound)
  python tools/audio/make_synth_sfx.py --jingles           (the stingers too, to build/synth/<slug>.ogg; needs ffmpeg)

The 1.0 sounds (src/app/audio.hpp's 1.0 block, D89-D91) needn't be specific, so they're made
here from scratch instead of generated: soft marimba and kalimba plucks, bell-like chimes,
airy whooshes of filtered noise, water from pitched sine bubbles, all short and warm, to sit
with the storybook sounds of the generated set. Pure Python (wave, math, random, struct): no
numpy, nothing to install, and the same files every run (each sound's noise is seeded by its
name).

The effects are finished the way tools/audio/process_sfx.py finishes the generated ones, so
they sit in the same mix: 22,050 Hz 16-bit mono, a high-pass by kind, clean fades (no clicks),
levelled by kind (the loudest 100 ms reaches the kind's target, peaks under -1 dBFS).

The beds are seamless by construction rather than by a crossfade: every layer repeats in the
loop's length. Noise is filtered twice round the loop and the second pass kept (the filters
have settled into the loop by then), slow swells are whole cycles per loop, and a wave's or a
chime's tail that runs past the end wraps round to the start. They are levelled by overall
RMS like the other loops (-26 dBFS).

The jingles (a battle won, a level up, a ribbon) are longer phrases for audio::playStinger,
rendered as 32 kHz stereo WAVs and encoded by tools/audio/make_loop.py's stinger mode (levelled
like the other stingers), into build/synth/ to be heard first: they are sparse music-box phrases
beside the full, generated stingers, so one ships only if it sounds right by ear (copy it into
romfs/music and call audio::playStinger with its name). The effects victory, level-up and
ribbon already cover those moments.
"""
from __future__ import annotations

import argparse
import cmath
import json
import math
import random
import struct
import subprocess
import sys
import wave
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from process_sfx import KINDS, PEAK_CEILING, RATE, db, loudest_rms, rms, soft_limit  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
OUT_DIR = ROOT / "romfs" / "sfx"
BUILD_DIR = ROOT / "build" / "synth"
TAU = 2 * math.pi
NYQUIST_SAFE = 0.45 * RATE  # partials above this are left out (they would alias)


# ------------------------------------------------------------------------------------ basics
def n_of(sec: float) -> int:
    return max(1, int(round(sec * RATE)))


def note(name: str) -> float:
    """'C5', 'F#4', 'Bb6' -> Hz (A4 = 440)."""
    steps = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}[name[0]]
    rest = name[1:]
    if rest[0] in "#b":
        steps += 1 if rest[0] == "#" else -1
        rest = rest[1:]
    return 440.0 * 2 ** ((steps + 12 * (int(rest) + 1) - 69) / 12)


def add(dst: list, src: list, at: float = 0.0, gain: float = 1.0) -> list:
    """Mixes src into dst from `at` seconds, growing dst as needed."""
    i0 = int(round(at * RATE))
    if i0 + len(src) > len(dst):
        dst.extend([0.0] * (i0 + len(src) - len(dst)))
    for i, v in enumerate(src):
        dst[i0 + i] += v * gain
    return dst


def norm(x: list, peak: float = 1.0) -> list:
    m = max((abs(v) for v in x), default=0.0)
    return [v * peak / m for v in x] if m > 0 else x


def mul(x: list, env: list) -> list:
    return [a * b for a, b in zip(x, env)]


def fade_in(x: list, sec: float) -> list:
    n = min(len(x), n_of(sec))
    for i in range(n):
        x[i] *= 0.5 - 0.5 * math.cos(math.pi * i / n)
    return x


def fade_out(x: list, sec: float) -> list:
    n = min(len(x), n_of(sec))
    for i in range(n):
        x[len(x) - 1 - i] *= 0.5 - 0.5 * math.cos(math.pi * i / n)
    return x


def tail_off(x: list) -> list:
    """Fades the last quarter of a note (at most 0.25 s): a note stops at its length while it
    may still be ringing, and a cut there would click."""
    return fade_out(x, min(0.25 * len(x) / RATE, 0.25))


def hump(n: int, peak: float = 0.5, rise: float = 1.0, fall: float = 1.0) -> list:
    """0 -> 1 -> 0 over n samples, the top at `peak` (0..1): sine-shaped sides raised to a power
    (a larger `rise` swells in later, a larger `fall` dies away sooner)."""
    out = []
    peak = min(max(peak, 1e-3), 1 - 1e-3)
    for i in range(n):
        u = i / max(1, n - 1)
        if u < peak:
            out.append(max(0.0, math.sin(0.5 * math.pi * u / peak)) ** (2 * rise))
        else:  # (clamped: at the very end the cosine can come out a hair below zero)
            out.append(max(0.0, math.cos(0.5 * math.pi * (u - peak) / (1 - peak))) ** (2 * fall))
    return out


def decay_env(n: int, tau: float, attack: float = 0.002) -> list:
    k = math.exp(-1.0 / (tau * RATE))
    a = max(1, n_of(attack))
    out, e = [], 1.0
    for i in range(n):
        out.append(e * (0.5 - 0.5 * math.cos(math.pi * i / a) if i < a else e))
        e *= k
    return out


def noise(n: int, rng: random.Random) -> list:
    r = rng.random
    return [2.0 * r() - 1.0 for _ in range(n)]


# ----------------------------------------------------------------------------------- filters
def biquad(x: list, kind: str, fc: float, q: float = 0.7071, gain_db: float = 0.0) -> list:
    """RBJ cookbook biquad (lp, hp, bp with 0 dB peak, and a low shelf of gain_db),
    transposed direct form II."""
    fc = min(fc, NYQUIST_SAFE)
    w = TAU * fc / RATE
    cw, alpha = math.cos(w), math.sin(w) / (2 * q)
    a0, a1, a2 = 1 + alpha, -2 * cw, 1 - alpha
    if kind == "lp":
        b0, b1, b2 = (1 - cw) / 2, 1 - cw, (1 - cw) / 2
    elif kind == "hp":
        b0, b1, b2 = (1 + cw) / 2, -(1 + cw), (1 + cw) / 2
    elif kind == "shelf":
        A = 10 ** (gain_db / 40)
        sa = 2 * math.sqrt(A) * math.sin(w) / math.sqrt(2)  # shelf slope 1
        b0 = A * ((A + 1) - (A - 1) * cw + sa)
        b1 = 2 * A * ((A - 1) - (A + 1) * cw)
        b2 = A * ((A + 1) - (A - 1) * cw - sa)
        a0 = (A + 1) + (A - 1) * cw + sa
        a1 = -2 * ((A - 1) + (A + 1) * cw)
        a2 = (A + 1) + (A - 1) * cw - sa
    else:
        b0, b1, b2 = alpha, 0.0, -alpha
    b0, b1, b2, a1, a2 = b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0
    out, z1, z2 = [], 0.0, 0.0
    for v in x:
        y = b0 * v + z1
        z1 = b1 * v - a1 * y + z2
        z2 = b2 * v - a2 * y
        out.append(y)
    return out


def svf(x: list, fc, q: float = 0.7071, mode: str = "bp") -> list:
    """A state-variable filter (the topology-preserving form, stable however fast its cutoff
    moves) whose cutoff `fc` may be a number or a list, one per sample: the sweeps. The band
    output is scaled to 0 dB at its peak."""
    k = 1.0 / q
    varying = isinstance(fc, list)
    out, ic1, ic2 = [], 0.0, 0.0
    a1 = a2 = a3 = 0.0
    for i, v0 in enumerate(x):
        if i == 0 or (varying and i % 8 == 0):
            f = min(fc[i] if varying else fc, NYQUIST_SAFE)
            g = math.tan(math.pi * f / RATE)
            a1 = 1.0 / (1.0 + g * (g + k))
            a2 = g * a1
            a3 = g * a2
        v3 = v0 - ic2
        v1 = a1 * ic1 + a2 * v3
        v2 = ic2 + a2 * ic1 + a3 * v3
        ic1 = 2 * v1 - ic1
        ic2 = 2 * v2 - ic2
        out.append(k * v1 if mode == "bp" else v2 if mode == "lp" else v0 - k * v1 - v2)
    return out


def sweep(n: int, f0: float, f1: float, curve: float = 1.0) -> list:
    """f0 -> f1 over n samples, even in pitch (exponential); curve > 1 moves late, < 1 early."""
    return [f0 * (f1 / f0) ** ((i / max(1, n - 1)) ** curve) for i in range(n)]


# ------------------------------------------------------------------------------- instruments
# Each returns its sound at a peak of 1: the mixes below set how loud each part is.
def partials(f: float, dur: float, spec, attack: float = 0.002) -> list:
    """A struck sound as decaying sines: spec is [(ratio to f, amplitude, decay tau s)]."""
    n = n_of(dur)
    out = [0.0] * n
    for ratio, amp, tau in spec:
        fr = f * ratio
        if fr >= NYQUIST_SAFE or amp <= 0:
            continue
        z = cmath.rect(math.exp(-1.0 / (tau * RATE)), TAU * fr / RATE)
        p = complex(amp, 0.0)
        for i in range(min(n, int(tau * RATE * math.log(amp / 1e-5)) + 1)):
            out[i] += p.imag
            p *= z
    return norm(tail_off(fade_in(out, attack)))


def mallet(f: float, dur: float, decay: float = 0.3, bright: float = 1.0, rng: random.Random | None = None) -> list:
    """A soft marimba bar: a round fundamental, its tuned overtones (about 4x and 9.2x) dying
    fast, and a little felt knock at the start."""
    x = partials(f, dur, [(1.0, 1.0, decay), (3.93, 0.3 * bright, decay * 0.2), (9.3, 0.09 * bright, decay * 0.07)],
                 attack=0.0015)
    if rng:
        knock = mul(biquad(noise(n_of(0.02), rng), "lp", min(3 * f, 5000)), decay_env(n_of(0.02), 0.003, 0.0005))
        add(x, norm(knock), 0.0, 0.12 * bright)
    return norm(x)


def kalimba(f: float, dur: float, decay: float = 0.4) -> list:
    """A thumb piano's tine: a clear fundamental, a faint octave and a quick, bright upper tine
    partial (about 5.9x) that gives the pluck."""
    return partials(f, dur, [(1.0, 1.0, decay), (2.0, 0.06, decay * 0.4), (5.93, 0.22, decay * 0.1),
                             (12.8, 0.04, decay * 0.05)], attack=0.0012)


def chime(f: float, dur: float, decay: float = 0.6, bright: float = 1.0) -> list:
    """A small bell or chime bar: the free bar's partials (1, 2.76, 5.40, 8.93) with a
    slightly detuned twin of the fundamental, so it shimmers as it rings."""
    return partials(f, dur, [(1.0, 1.0, decay), (1.0035, 0.35, decay), (2.76, 0.28 * bright, decay * 0.45),
                             (5.40, 0.12 * bright, decay * 0.22), (8.93, 0.05 * bright, decay * 0.12)],
                    attack=0.0015)


def glide(f0: float, f1: float, dur: float, t_glide: float, tau: float | None = None, env: list | None = None,
          harmonics=((1, 1.0),), vibrato=(0.0, 0.0), attack: float = 0.003) -> list:
    """A sung or whistled tone sliding f0 -> f1 over t_glide (even in pitch), then holding:
    decaying (tau), shaped by `env`, or both. Bubbles, blips, coos and swoons."""
    n = n_of(dur)
    out, ph = [], 0.0
    k = math.exp(-1.0 / (tau * RATE)) if tau else 1.0
    e = 1.0
    vr, vd = vibrato
    for i in range(n):
        t = i / RATE
        u = min(1.0, t / t_glide) if t_glide > 0 else 1.0
        f = f0 * (f1 / f0) ** u
        if vd:
            f *= 1.0 + vd * math.sin(TAU * vr * t)
        ph += TAU * f / RATE
        s = 0.0
        for h, a in harmonics:
            if f * h < NYQUIST_SAFE:
                s += a * math.sin(h * ph)
        out.append(s * e * (env[i] if env else 1.0))
        e *= k
    return norm(tail_off(fade_in(out, attack)))


def swish(dur: float, f0: float, f1: float, rng: random.Random, q: float = 1.0, peak: float = 0.5,
          rise: float = 1.0, fall: float = 1.0, curve: float = 1.0) -> list:
    """An airy whoosh: noise through a band sweeping f0 -> f1, swelling and fading."""
    n = n_of(dur)
    return norm(mul(svf(noise(n, rng), sweep(n, f0, f1, curve), q), hump(n, peak, rise, fall)))


def pof(rng: random.Random, f0: float, f1: float, tau: float, noise_lp: float = 1400, noise_amt: float = 0.5) -> list:
    """A soft body landing or bonk: a round thump dropping in pitch and a puff of muffled
    noise. Its 2nd and 3rd harmonics are strong enough for the 3DS's little speakers, which
    hardly play the fundamental of a low thump, to carry it."""
    dur = 6 * tau
    x = glide(f0, f1, dur, tau * 0.8, tau=tau, harmonics=((1, 1.0), (2, 0.45), (3, 0.18)), attack=0.0015)
    n = n_of(dur)
    puff = mul(biquad(biquad(noise(n, rng), "lp", noise_lp), "lp", noise_lp), decay_env(n, tau * 0.45, 0.001))
    return norm(add(x, norm(puff), 0.0, noise_amt))


def tick(rng: random.Random, f: float, tau: float, noise_fc: float, noise_amt: float = 0.5) -> list:
    """A small woody or plastic click: two quick partials and a bright crumb of noise."""
    dur = 7 * tau
    x = partials(f, dur, [(1.0, 1.0, tau), (2.31, 0.5, tau * 0.6)], attack=0.0006)
    n = n_of(dur)
    crumb = mul(biquad(noise(n, rng), "bp", noise_fc, 1.2), decay_env(n, tau * 0.4, 0.0004))
    return norm(add(x, norm(crumb), 0.0, noise_amt))


def clink(rng: random.Random, f: float) -> list:
    """Two small shells knocking: short inharmonic partials, like thin porcelain."""
    return norm(add(partials(f, 0.1, [(1.0, 1.0, 0.018), (1.47, 0.6, 0.014), (2.09, 0.4, 0.011), (2.56, 0.25, 0.009)],
                             attack=0.0006),
                    norm(mul(biquad(noise(n_of(0.02), rng), "hp", 3000), decay_env(n_of(0.02), 0.002, 0.0003))),
                    0.0, 0.2))


def crackle(rng: random.Random, fc: float, tau: float, q: float = 1.5) -> list:
    """One crackle or crunch grain: a speck of noise rung through a narrow band."""
    n = n_of(8 * tau)
    return norm(svf(mul(noise(n, rng), decay_env(n, tau, 0.0003)), fc, q))


def droplet(f: float, tau: float = 0.012, rise: float = 1.7) -> list:
    """A water drop's bloop: a sine bubble sliding up as it closes."""
    return glide(f, f * rise, 6 * tau, 2 * tau, tau=tau, attack=0.001)


def splash_noise(rng: random.Random, dur: float, fc: float, tau: float) -> list:
    n = n_of(dur)
    return norm(tail_off(mul(biquad(biquad(noise(n, rng), "bp", fc, 0.6), "lp", 6000), decay_env(n, tau, 0.002))))


# ---------------------------------------------------------------------------- the effects
# Each makes one take (take 0, 1, 2 ...) with its own random generator. Notes are in C major
# and its pentatonic, like the game's chimes.
def hop_on(rng, take):
    """Hopping up onto your dragon: a round little 'hup' sliding up, a swish of cloth, and a
    soft 'pof' as you settle into the saddle."""
    x = add([], glide(300, 640, 0.15, 0.09, tau=0.06, harmonics=((1, 1.0), (2, 0.25), (3, 0.08))), 0.0, 0.7)
    add(x, swish(0.16, 900, 2400, rng, q=0.9, peak=0.4), 0.0, 0.3)
    add(x, pof(rng, 220, 125, 0.05, noise_lp=1500, noise_amt=0.6), 0.13, 0.9)
    return x


def hop_off(rng, take):
    """Hopping down: the slide going down, a swish, and a slightly heavier 'pof' on the ground."""
    x = add([], glide(620, 290, 0.16, 0.11, tau=0.07, harmonics=((1, 1.0), (2, 0.25), (3, 0.08))), 0.0, 0.65)
    add(x, swish(0.16, 2200, 800, rng, q=0.9, peak=0.45), 0.0, 0.3)
    add(x, pof(rng, 190, 100, 0.06, noise_lp=1200, noise_amt=0.7), 0.17, 1.0)
    return x


def nuzzle(rng, take):
    """Your dragon rubbing its head against you: soft rustles of scales, and a warm contented
    little coo gliding up under them."""
    x = []
    rubs = [(0.0, 0.3, 1100, 1900), (0.22, 0.34, 1700, 1000)] if take == 0 else [(0.0, 0.46, 1000, 1800)]
    for at, dur, f0, f1 in rubs:
        add(x, swish(dur, f0, f1, rng, q=0.6, peak=0.45, rise=1.3, fall=1.2), at, 0.55)
    f0, f1 = (370, 440) if take == 0 else (330, 415)
    n = n_of(0.42)
    coo = glide(f0, f1, 0.42, 0.3, env=hump(n, 0.35, 1.2, 1.0), harmonics=((1, 1.0), (2, 0.35), (3, 0.1)),
                vibrato=(5.5, 0.012))
    add(x, biquad(coo, "lp", 1600), 0.07, 0.4)
    return x


def shutter(rng, take):
    """A photo: a small bright click as the shutter opens, a breath of air, a lower click as it
    shuts, and a tiny glint of a chime."""
    x = add([], tick(rng, 2600, 0.010, 4000), 0.0, 1.0)
    add(x, swish(0.05, 2500, 4200, rng, q=1.2), 0.008, 0.25)
    add(x, tick(rng, 1900, 0.014, 3000), 0.065, 0.85)
    add(x, chime(note("E6"), 0.3, decay=0.1, bright=0.5), 0.10, 0.22)
    return x


def equip(rng, take):
    """Putting on an accessory: a quick 'fwip' of fabric, then two bright little bell notes."""
    x = add([], swish(0.09, 1200, 3200, rng, q=0.8, peak=0.6), 0.0, 0.5)
    add(x, chime(note("A5"), 0.4, decay=0.14), 0.06, 0.6)
    add(x, chime(note("E6"), 0.42, decay=0.18), 0.12, 0.5)
    return x


def level_up(rng, take):
    """A level up: four kalimba notes running up (C E G C), a soft chord of chimes ringing out
    and a few tiny fairy-dust twinkles above it."""
    x = []
    for i, nm in enumerate(("C5", "E5", "G5", "C6")):
        add(x, kalimba(note(nm), 0.45, decay=0.2), 0.075 * i, 0.8)
    for nm, g in (("C6", 0.5), ("E6", 0.4), ("G6", 0.35)):
        add(x, chime(note(nm), 0.75, decay=0.3, bright=0.6), 0.30, g)
    for k in range(6):
        nm = rng.choice(("C7", "D7", "E7", "G7", "A7"))
        add(x, chime(note(nm), 0.18, decay=0.045, bright=0.3), 0.33 + 0.06 * k + rng.uniform(0, 0.02), 0.12)
    return x


def unlock(rng, take):
    """Something new unlocked: a key's click and clunk, then three chimes climbing (G B D)."""
    x = add([], tick(rng, 1500, 0.012, 2500), 0.0, 0.9)
    add(x, tick(rng, 950, 0.018, 1600), 0.055, 1.0)
    for i, nm in enumerate(("G5", "B5", "D6")):
        add(x, chime(note(nm), 0.45, decay=0.2, bright=0.7), 0.13 + 0.065 * i, 0.55)
    return x


def notice(rng, take):
    """Something catches the eye: a round 'bwip!' sliding up, and a small ping above it."""
    x = add([], glide(680, 1360, 0.12, 0.05, tau=0.045, harmonics=((1, 1.0), (2, 0.15))), 0.0, 0.7)
    add(x, chime(note("A6"), 0.28, decay=0.1, bright=0.5), 0.06, 0.4)
    return x


def battle_start(rng, take):
    """A battle begins: a rush of air swelling up, then a warm marimba chord (C G C E) struck
    with a soft drum and a bright bell. Exciting without being loud."""
    x = add([], swish(0.38, 350, 3200, rng, q=1.4, peak=0.92, rise=2.0, fall=1.0), 0.0, 0.7)
    t = 0.34
    for nm, g in (("C5", 0.7), ("G5", 0.55), ("C6", 0.5), ("E6", 0.3)):
        add(x, mallet(note(nm), 0.6, decay=0.28, rng=rng), t, g)
    add(x, pof(rng, 170, 95, 0.1, noise_lp=900, noise_amt=0.3), t, 0.9)
    add(x, chime(note("G6"), 0.5, decay=0.22, bright=0.6), t + 0.01, 0.3)
    return x


def swipe(rng, take):
    """A claw swipe: a quick whoosh of air falling from bright to dull."""
    sc, dur = ((1.0, 0.17), (0.88, 0.2), (1.12, 0.15))[take]
    x = add([], swish(dur, 3800 * sc, 850 * sc, rng, q=1.1, peak=0.35, rise=1.5, fall=1.6), 0.0, 1.0)
    if take == 1:  # the other claw close behind
        add(x, swish(0.12, 3000, 900, rng, q=1.1, peak=0.35, rise=1.5, fall=1.6), 0.07, 0.45)
    return x


def hit(rng, take):
    """A hit: a soft padded 'bmf', like a pillow bonk, with a small 'pok' on top so the
    speakers carry it."""
    sc = (1.0, 0.9, 1.1)[take]
    x = add([], pof(rng, 300 * sc, 130 * sc, 0.055, noise_lp=2000, noise_amt=0.6), 0.0, 1.0)
    add(x, mallet(620 * sc, 0.12, decay=0.03, bright=0.4), 0.0, 0.5)
    return x


def hit_big(rng, take):
    """A big hit (super effective, or a critical): a deeper, fuller bonk, a cartoon wobble
    ringing after it and three tiny star twinkles."""
    sc = (1.0, 0.92)[take]
    x = add([], pof(rng, 260 * sc, 95 * sc, 0.11, noise_lp=1600, noise_amt=0.7), 0.0, 1.0)
    add(x, mallet(480 * sc, 0.2, decay=0.05, bright=0.5), 0.0, 0.5)
    add(x, glide(520 * sc, 500 * sc, 0.3, 0.3, tau=0.09, vibrato=(14, 0.04), harmonics=((1, 1.0), (2, 0.2))), 0.02, 0.22)
    stars = (("E7", "B6", "G7"), ("G7", "D7", "A7"))[take]
    for k, nm in enumerate(stars):
        add(x, chime(note(nm), 0.15, decay=0.045, bright=0.3), 0.10 + 0.05 * k, 0.13)
    return x


def whiff(rng, take):
    """A miss: a thin, quick breath of air sailing past, and a faint falling 'fwee'."""
    sc = (1.0, 1.15)[take]
    x = add([], swish(0.2, 2600 * sc, 1300 * sc, rng, q=1.6, peak=0.5, rise=2.0, fall=2.0), 0.0, 1.0)
    n = n_of(0.2)
    add(x, glide(1300 * sc, 800 * sc, 0.2, 0.18, env=hump(n, 0.3, 1.0, 1.5)), 0.02, 0.18)
    return x


def faint(rng, take):
    """A dragon worn out in a battle (never hurt): a soft dizzy swoon, 'wooo' sliding down
    with a wobble, then a gentle flop."""
    n = n_of(0.8)
    x = add([], glide(640, 230, 0.8, 0.7, env=hump(n, 0.08, 1.0, 0.8), harmonics=((1, 1.0), (2, 0.3), (3, 0.1)),
                      vibrato=(6.5, 0.025)), 0.0, 0.55)
    add(x, pof(rng, 190, 100, 0.07, noise_lp=1000, noise_amt=0.5), 0.74, 0.9)
    add(x, swish(0.15, 1500, 600, rng, q=0.7), 0.7, 0.2)
    return x


def stat_up(rng, take):
    """A stat goes up: five quick bright marimba notes climbing, a rising shimmer, a twinkle."""
    x = []
    for i, nm in enumerate(("C6", "D6", "E6", "G6", "A6")):
        add(x, mallet(note(nm), 0.2, decay=0.07, bright=0.8, rng=rng), 0.045 * i, 0.55)
    add(x, swish(0.28, 1500, 5000, rng, q=0.7, peak=0.8), 0.0, 0.18)
    add(x, chime(note("C7"), 0.3, decay=0.12, bright=0.4), 0.23, 0.3)
    return x


def stat_down(rng, take):
    """A stat goes down: four softer notes stepping down, each drooping a little in pitch."""
    x = []
    for i, nm in enumerate(("A5", "F5", "D5", "A4")):
        f = note(nm)
        add(x, glide(f, f * 0.96, 0.22, 0.2, tau=0.07, harmonics=((1, 1.0), (2, 0.12))), 0.075 * i, 0.55)
    add(x, swish(0.3, 3000, 900, rng, q=0.7, peak=0.3), 0.0, 0.12)
    return x


def victory(rng, take):
    """A battle won: three marimba notes up (G C E) into a bright ringing chord with bells."""
    x = []
    for i, nm in enumerate(("G5", "C6", "E6")):
        add(x, mallet(note(nm), 0.45, decay=0.16, bright=0.9, rng=rng), 0.09 * i, 0.65)
    t = 0.3
    for nm, g in (("C6", 0.55), ("E6", 0.45), ("G6", 0.45), ("C7", 0.25)):
        add(x, mallet(note(nm), 1.0, decay=0.36, bright=0.8, rng=rng), t, g)
    for nm, g in (("G6", 0.25), ("C7", 0.2)):
        add(x, chime(note(nm), 1.0, decay=0.4, bright=0.5), t + 0.01, g)
    return x


def defeat(rng, take):
    """A battle lost, gently: three soft notes falling (A E C), settling on a warm soft
    chord (F A C), an 'oh well, next time' rather than a sad one."""
    x = []
    for i, nm in enumerate(("A5", "E5", "C5")):
        add(x, mallet(note(nm), 0.6, decay=0.26, bright=0.5), 0.2 * i, 0.6)
    for nm, g in (("F4", 0.45), ("A4", 0.45), ("C5", 0.4)):
        add(x, mallet(note(nm), 0.9, decay=0.42, bright=0.4), 0.6, g)
    return x


def pose(rng, take):
    """A pose on the pageant's stage: a quick swish up and a 'ta-da' of chimes (C E G) with a
    sprinkle of tiny twinkles."""
    x = add([], swish(0.12, 800, 2600, rng, q=0.8, peak=0.8), 0.0, 0.35)
    t = 0.1
    for nm, g in (("C6", 0.5), ("E6", 0.45), ("G6", 0.4)):
        add(x, chime(note(nm), 0.55, decay=0.26, bright=0.7), t, g)
    for k, nm in enumerate(("C7", "E7", "G7", "D7", "A7")):
        add(x, chime(note(nm), 0.16, decay=0.04, bright=0.3), t + 0.03 + 0.045 * k, 0.15)
    return x


def twirl(rng, take):
    """A twirl: a swirl of air rising and falling twice as it spins, and a run of kalimba
    notes spinning up to a chime."""
    n = n_of(0.62)
    fc = [700 * 3.2 ** (0.5 - 0.5 * math.cos(TAU * 2 * i / n)) for i in range(n)]
    x = add([], norm(mul(svf(noise(n, rng), fc, 1.8), hump(n, 0.5, 1.0, 1.0))), 0.0, 0.55)
    for i, nm in enumerate(("C5", "D5", "E5", "G5", "A5", "C6")):
        add(x, kalimba(note(nm), 0.28, decay=0.11), 0.07 * i, 0.4)
    add(x, chime(note("E6"), 0.35, decay=0.18), 0.45, 0.3)
    return x


def ribbon(rng, take):
    """A ribbon won: a warm chord of bells (C E G C) strummed quickly, ringing, with two
    higher chimes floating after it."""
    x = []
    for i, (nm, g) in enumerate((("C5", 0.5), ("E5", 0.45), ("G5", 0.45), ("C6", 0.5))):
        add(x, chime(note(nm), 1.05, decay=0.42, bright=0.6), 0.018 * i, g)
    add(x, chime(note("G6"), 0.7, decay=0.26, bright=0.4), 0.2, 0.2)
    add(x, chime(note("C7"), 0.55, decay=0.2, bright=0.3), 0.3, 0.15)
    return x


def cast(rng, take):
    """Casting a line: the rod's swish, then the reel's ratchet spinning out and slowing."""
    x = add([], swish(0.16, 700, 3200, rng, q=1.0, peak=0.6), 0.0, 0.9)
    t, rate = 0.12, 42.0
    while t < 0.5:
        add(x, tick(rng, 2400 * rng.uniform(0.97, 1.03), 0.004, 3000, 0.4), t, 0.32 * (1 - (t - 0.12) / 0.45))
        rate *= 0.93
        t += 1 / rate
    return x


def plop(rng, take):
    """The bobber landing: a small 'bup' as it hits, the water's 'bloop' closing over it and
    a few tiny droplets."""
    sc = (1.0, 0.85)[take]
    x = add([], glide(520 * sc, 190 * sc, 0.1, 0.03, tau=0.025), 0.0, 0.55)
    add(x, glide(360 * sc, 1050 * sc, 0.16, 0.06, tau=0.045), 0.018, 0.8)
    add(x, splash_noise(rng, 0.18, 1800, 0.04), 0.0, 0.3)
    for k in range(3):
        add(x, droplet(rng.uniform(900, 1500) * sc), 0.09 + 0.05 * k + rng.uniform(0, 0.02), 0.18)
    return x


def bite(rng, take):
    """A fish takes the bait: the bobber pulled under twice, 'blub-blub', with little splashes."""
    x = []
    for k, at in enumerate((0.0, 0.14)):
        add(x, glide(240, 620, 0.12, 0.05, tau=0.04), at, 0.8 - 0.15 * k)
        add(x, splash_noise(rng, 0.1, 1400, 0.025), at, 0.25)
    return x


def reel(rng, take):
    """Reeling in: a short run of soft ratchet clicks over the faint whirr of the spool
    (played again and again while you reel, so it's quiet)."""
    rate, f = ((26.0, 1900.0), (32.0, 2130.0))[take]
    x, clicks = [], 9
    for k in range(clicks):
        add(x, tick(rng, f * rng.uniform(0.97, 1.03), 0.005, f * 1.3, 0.4), k / rate,
            0.6 + 0.4 * math.sin(math.pi * k / (clicks - 1)))
    n = n_of(clicks / rate)
    add(x, norm(mul(biquad(noise(n, rng), "bp", 700, 1.2), hump(n, 0.5))), 0.0, 0.12)
    return x


def shell_pick(rng, take):
    """Picking up a shell: two small porcelain clinks and a sweet kalimba note."""
    x = add([], clink(rng, 2300), 0.0, 0.6)
    add(x, clink(rng, 2900), 0.045, 0.45)
    add(x, kalimba(note("E6"), 0.35, decay=0.14), 0.08, 0.5)
    return x


def ice_crack(rng, take):
    """Ice cracking: a small 'tunk' as it gives, a spray of crisp little crackles and a faint
    glassy ring."""
    x = add([], pof(rng, 330, 200, 0.03, noise_lp=2500, noise_amt=0.3), 0.0, 0.4)
    t = 0.0
    for k in range(9 if take == 0 else 7):
        t += rng.uniform(0.008, 0.05)
        add(x, crackle(rng, rng.uniform(2500, 5000), rng.uniform(0.002, 0.006)), t, 0.9 * 0.82 ** k)
    base = (1850, 2070)[take]
    add(x, partials(base, 0.4, [(1.0, 1.0, 0.12), (1.69, 0.6, 0.08), (2.53, 0.35, 0.05)]), 0.01, 0.3)
    return biquad(x, "lp", 7000)


def step_sand(rng, take):
    """A footstep on sand: a soft grainy 'shff' and the faintest thud."""
    sc = (1.0, 0.9, 1.12)[take]
    n = n_of(0.17)
    env = [e * f for e, f in zip(hump(n, 0.1, 1.0, 1.0), decay_env(n, 0.06, 0.0))]
    grains = [0.0] * n
    for _ in range(90):  # sand shifting: many tiny grains, denser where the step presses
        i = rng.randrange(n)
        if rng.random() < env[i]:
            g = mul(noise(n_of(0.003), rng), hump(n_of(0.003)))
            for j, v in enumerate(g):
                if i + j < n:
                    grains[i + j] += v * rng.uniform(0.5, 1.0)
    body = mul(noise(n, rng), env)
    x = [a + 0.6 * b for a, b in zip(grains, body)]
    x = biquad(biquad(x, "bp", 1700 * sc, 0.7), "lp", 4500)
    add(x, glide(150, 110, 0.08, 0.05, tau=0.025), 0.0, 0.3 * max(abs(v) for v in x))
    return x


def step_snow(rng, take):
    """A footstep in snow: a soft packed crunch of little squeaky grains over a muffled body."""
    n = n_of(0.2)
    x = [0.0] * n
    count = (22, 18, 26)[take]
    env = hump(n, 0.25, 0.8, 1.3)
    for _ in range(count):
        t = rng.uniform(0.01, 0.13)
        add(x, crackle(rng, rng.uniform(900, 2400), rng.uniform(0.003, 0.006), q=3.0), t,
            env[min(n - 1, int(t * RATE))] * rng.uniform(0.4, 1.0))
    body = norm(mul(biquad(noise(n, rng), "lp", 1100), [e * f for e, f in zip(hump(n, 0.08), decay_env(n, 0.05, 0.0))]))
    add(x, body, 0.0, 0.5 * max(abs(v) for v in x))
    add(x, glide(140, 100, 0.08, 0.05, tau=0.025), 0.0, 0.25 * max(abs(v) for v in x))
    return x[:n]


def burst(rng, take):
    """A burst of speed: a fast rush of air sweeping up, a 'vwoop' rising under it and a bright
    ping at the top."""
    x = add([], swish(0.42, 500, 4200, rng, q=1.2, peak=0.7, rise=1.8, fall=1.2), 0.0, 0.9)
    n = n_of(0.3)
    add(x, glide(300, 900, 0.3, 0.25, env=hump(n, 0.8, 1.0, 1.0), harmonics=((1, 1.0), (2, 0.2))), 0.0, 0.3)
    add(x, chime(note("G6"), 0.32, decay=0.14, bright=0.5), 0.26, 0.3)
    return x


def brake(rng, take):
    """A brake in the air: wings thrown wide catch it with a 'fwump', the rush falling away,
    a flutter of membrane, and a soft 'vwum' sliding down."""
    n = n_of(0.46)
    flutter = [1.0 - 0.35 * max(0.0, 1 - i / (0.22 * RATE)) * (0.5 + 0.5 * math.cos(TAU * 17 * i / RATE))
               for i in range(n)]
    air = mul(svf(noise(n, rng), sweep(n, 3500, 450), 0.9, "lp"), mul(hump(n, 0.06, 1.0, 1.4), flutter))
    x = add([], norm(air), 0.0, 1.0)
    add(x, glide(520, 220, 0.4, 0.35, env=hump(n_of(0.4), 0.1, 1.0, 1.0), harmonics=((1, 1.0), (2, 0.2))), 0.0, 0.25)
    return x


# ---------------------------------------------------------------- workstream D: trainers and people
def roamer_hello(rng, take):
    """A roaming trainer's hello as you pass: a cheery two-note whistle, up then down ('wheet-whoo'),
    a little breathy, with a soft kalimba pluck under the first note."""
    sc = (1.0, 1.06)[take]
    x = []
    for t0, f0, f1, dur in ((0.0, 1180 * sc, 1560 * sc, 0.16), (0.2, 1480 * sc, 1080 * sc, 0.24)):
        n = n_of(dur)
        tone = glide(f0, f1, dur, dur * 0.6, env=hump(n, 0.3, 1.0, 1.2), harmonics=((1, 1.0), (2, 0.08)), vibrato=(7.0, 0.012))
        add(x, tone, t0, 0.8)
        add(x, norm(mul(biquad(noise(n, rng), "bp", f1 * 1.0, 3.0), hump(n, 0.3))), t0, 0.12)
    add(x, kalimba(note("E6") * sc, 0.3, decay=0.12), 0.0, 0.25)
    return x


def hands_clap(rng, take):
    """A few people clapping for a show or a duel: a small patter of claps over a second or so,
    each a bright snap of noise with a hollow knock under it, the hands not quite together."""
    n = n_of(1.3)
    x = [0.0] * n
    count = (9, 11, 8)[take]
    for k in range(count):
        t = 0.02 + k * (1.1 / count) + rng.uniform(-0.03, 0.03)
        for hand in range(rng.choice((1, 2))):  # (two in the crowd at almost the same moment)
            m = n_of(0.05)
            snap = mul(biquad(biquad(noise(m, rng), "bp", rng.uniform(1300, 2300), 1.1), "hp", 700), decay_env(m, 0.008, 0.0004))
            knock = glide(rng.uniform(420, 560), 380, 0.05, 0.02, tau=0.01, attack=0.0005)
            add(x, norm(snap), t + hand * rng.uniform(0.008, 0.02), rng.uniform(0.5, 0.9))
            add(x, knock, t + hand * 0.01, 0.15)
    env = [min(1.0, i / n_of(0.05)) * min(1.0, (n - i) / n_of(0.35)) for i in range(n)]
    return mul(x, env)


def soft_snore(rng, take):
    """Someone dozing: a soft, slow breath in with a low purring rattle, then a gentle whistle out,
    sleepy and cute rather than loud."""
    sc = (1.0, 0.92)[take]
    n_in, n_out = n_of(0.9), n_of(0.7)
    rattle = [0.55 + 0.45 * math.sin(TAU * 28 * sc * i / RATE) for i in range(n_in)]
    breath_in = mul(biquad(biquad(noise(n_in, rng), "bp", 420 * sc, 0.9), "lp", 1400), [a * b for a, b in zip(hump(n_in, 0.6, 1.0, 1.4), rattle)])
    x = add([], norm(breath_in), 0.0, 0.8)
    add(x, glide(110 * sc, 96 * sc, 0.9, 0.8, env=[a * b for a, b in zip(hump(n_in, 0.6), rattle)],
                 harmonics=((1, 1.0), (2, 0.5), (3, 0.3))), 0.0, 0.35)
    breath_out = mul(biquad(noise(n_out, rng), "bp", 900 * sc, 1.4), hump(n_out, 0.25, 1.0, 1.6))
    add(x, norm(breath_out), 0.95, 0.4)
    add(x, glide(980 * sc, 720 * sc, 0.55, 0.5, env=hump(n_of(0.55), 0.3), harmonics=((1, 1.0),)), 1.0, 0.12)
    return x


# slug: (kind in process_sfx's KINDS, takes, maker, dB against the kind's target)
# ----------------------------------------------------------------- the valley's critters (L)
# Stand-ins until generated ones arrive (docs/audio/sfx-life-prompts.md): small, soft and cute.
def bird_chirp(rng, take):
    """A songbird's little twitter: two or three quick whistled notes sliding up high, bright and
    tiny, like a sparrow chatting in the grass."""
    patterns = (((2600, 4200, 0.07), (3000, 4600, 0.06)),
                ((3400, 2600, 0.05), (2800, 4400, 0.07), (3300, 4800, 0.05)),
                ((2400, 3900, 0.09), (3900, 3200, 0.06)))
    x, t = [], 0.0
    for f0, f1, d in patterns[take]:
        add(x, glide(f0, f1, d, d * 0.7, tau=d * 0.5, harmonics=((1, 1.0), (2, 0.12)), vibrato=(38, 0.02), attack=0.004), t, 0.8)
        t += d + rng.uniform(0.02, 0.05)
    return x


def bird_flutter(rng, take):
    """A little flock bursting up off the grass: a flurry of soft wingbeats (quick puffs of
    feathery noise, thick at first and thinning as they fly off) and a startled peep or two."""
    x, t = [], 0.0
    while t < 0.75:
        n = n_of(0.03)
        puff = mul(biquad(noise(n, rng), "bp", rng.uniform(900, 1800), 0.9), hump(n, 0.3, 1.0, 1.5))
        add(x, norm(puff), t, (1.0 - t / 0.9) * rng.uniform(0.5, 0.9))
        t += rng.uniform(0.018, 0.04) * (1 + 1.5 * t)
    add(x, glide(3200, 4600, 0.06, 0.04, tau=0.03), 0.02, 0.35)
    add(x, glide(3600, 5000, 0.05, 0.03, tau=0.025), 0.12, 0.25)
    return x


def rabbit_hop(rng, take):
    """A rabbit bounding off: two or three soft padded thumps on the grass, quick and light, with
    a whisper of grass under them."""
    x = []
    for k in range(2 + take):
        at = k * 0.11
        add(x, pof(rng, 190 - 10 * k, 110, 0.025, noise_lp=1200, noise_amt=0.5), at, 0.8 - 0.15 * k)
        add(x, norm(mul(biquad(noise(n_of(0.05), rng), "hp", 2500), decay_env(n_of(0.05), 0.012))), at, 0.12)
    return x


def frog_croak(rng, take):
    """A small round frog's 'ribbit': a buzzy, throaty double croak (a warm tone pulsed quickly,
    like a thumb run along a comb), the second part a little higher. Cute, not slimy."""
    sc = (1.0, 1.12)[take]
    x = []
    for at, f0, f1, d in ((0.0, 280, 330, 0.13), (0.17, 340, 300, 0.1)):
        n = n_of(d)
        tone = glide(f0 * sc, f1 * sc, d, d, harmonics=((1, 1.0), (2, 0.6), (3, 0.35), (4, 0.15)), attack=0.004)
        pulses = [0.35 + 0.65 * max(0.0, math.sin(TAU * 34 * i / RATE)) ** 2 for i in range(n)]
        add(x, mul(mul(tone, pulses), hump(n, 0.3, 0.8, 1.2)), at, 1.0 if at == 0 else 0.8)
    return x


def duck_quack(rng, take):
    """A friendly farm duck's soft 'quack': a short nasal honk sliding down, reedy (lots of
    overtones through a narrow band), rounded off so it's gentle; the second take quacks twice."""
    sc = (1.0, 0.9)[take]
    d = 0.2
    n = n_of(d)
    tone = glide(560 * sc, 420 * sc, d, 0.12, harmonics=tuple((h, 1.0 / h ** 0.7) for h in range(1, 12)), attack=0.006)
    x = mul(norm(svf(tone, 1400 * sc, q=2.2)), hump(n, 0.2, 0.7, 1.3))
    if take == 1:
        x = add(x, list(x), 0.24, 0.7)  # quack-quack
    return x


def fox_yip(rng, take):
    """A curious young fox's soft 'yip': a quick bright bark-chirp jumping up and falling back,
    with a breath of air on it, more puppy than wolf."""
    x = add([], glide(760, 1500, 0.07, 0.05, harmonics=((1, 1.0), (2, 0.45), (3, 0.2)), attack=0.003), 0.0, 0.9)
    add(x, glide(1500, 950, 0.12, 0.1, tau=0.05, harmonics=((1, 1.0), (2, 0.35))), 0.055, 0.8)
    add(x, swish(0.1, 1500, 3000, rng, q=1.2, peak=0.3), 0.0, 0.2)
    return x


def butterfly_land(rng, take):
    """A butterfly settling: a tiny sparkle of three soft, high chime notes floating down, and
    the faintest flutter of air, delicate as a whisper."""
    x = []
    for i, nm in enumerate(("E7", "C7", "G6")):
        add(x, chime(note(nm), 0.5, decay=0.18, bright=0.4), 0.07 * i, 0.5 - 0.1 * i)
    add(x, swish(0.25, 2500, 5000, rng, q=0.8, peak=0.4), 0.0, 0.15)
    return x


def whistle_call(rng, take):
    """You whistling softly to the birds: a clear two-note call, a rising 'fweee' then a bright
    little 'whit', breathy and friendly."""
    x = add([], glide(1300, 2100, 0.32, 0.22, env=hump(n_of(0.32), 0.6, 0.8, 1.0), vibrato=(6, 0.01), attack=0.02), 0.0, 0.85)
    add(x, glide(2300, 2700, 0.14, 0.06, env=hump(n_of(0.14), 0.3, 0.8, 1.2), attack=0.01), 0.38, 0.8)
    add(x, swish(0.5, 2000, 3500, rng, q=0.7, peak=0.5), 0.0, 0.06)
    return x


def critter_friend(rng, take):
    """A new little friend: three warm kalimba notes skipping up (G C E) and a soft bell ringing
    on top, short and sweet."""
    x = []
    for i, nm in enumerate(("G5", "C6", "E6")):
        add(x, kalimba(note(nm), 0.7, decay=0.3), 0.09 * i, 0.8)
    add(x, chime(note("G6"), 0.9, decay=0.35, bright=0.5), 0.27, 0.35)
    return x


def leaf_rustle(rng, take):
    """A critter diving into a bush: a quick, soft rustle of leaves and twigs (a scatter of dry,
    crackly grains swelling and settling), over in a moment."""
    x, t = [], 0.0
    while t < 0.4:
        add(x, crackle(rng, rng.uniform(2500, 5500), rng.uniform(0.002, 0.005)), t, rng.uniform(0.3, 0.8) * (1.0 - t / 0.45))
        t += rng.uniform(0.006, 0.02)
    n = n_of(0.4)
    add(x, norm(mul(biquad(noise(n, rng), "bp", 3000, 0.7), hump(n, 0.25, 1.0, 1.4))), 0.0, 0.35)
    return x


EFFECTS = {
    "hop-on": ("body", 1, hop_on, 0.0),
    "hop-off": ("body", 1, hop_off, 0.0),
    "nuzzle": ("care", 2, nuzzle, -1.0),
    "shutter": ("ui", 1, shutter, 0.0),
    "equip": ("ui", 1, equip, 0.0),
    "level-up": ("ui", 1, level_up, 2.0),
    "unlock": ("ui", 1, unlock, 0.0),
    "notice": ("ui", 1, notice, 0.0),
    "battle-start": ("body", 1, battle_start, 0.0),
    "swipe": ("body", 3, swipe, 0.0),
    "hit": ("body", 3, hit, 0.0),
    "hit-big": ("body", 2, hit_big, 1.0),
    "whiff": ("body", 2, whiff, -3.0),
    "faint": ("body", 1, faint, 0.0),
    "stat-up": ("ui", 1, stat_up, 0.0),
    "stat-down": ("ui", 1, stat_down, 0.0),
    "victory": ("ui", 1, victory, 2.0),
    "defeat": ("ui", 1, defeat, 0.0),
    "pose": ("ui", 1, pose, 2.0),
    "twirl": ("ui", 1, twirl, 2.0),
    "ribbon": ("ui", 1, ribbon, 2.0),
    "cast": ("body", 1, cast, 0.0),
    "plop": ("care", 2, plop, 0.0),
    "bite": ("care", 1, bite, 0.0),
    "reel": ("care", 2, reel, -3.0),
    "shell-pick": ("care", 1, shell_pick, 0.0),
    "ice-crack": ("body", 2, ice_crack, 0.0),
    "step-sand": ("body", 3, step_sand, 0.0),
    "step-snow": ("body", 3, step_snow, 0.0),
    "burst": ("body", 1, burst, 0.0),
    "brake": ("body", 1, brake, 0.0),
    # the valley's critters (workstream L)
    "bird-chirp": ("voice", 3, bird_chirp, -3.0),
    "bird-flutter": ("body", 1, bird_flutter, 0.0),
    "rabbit-hop": ("body", 2, rabbit_hop, -3.0),
    "frog-croak": ("voice", 2, frog_croak, -3.0),
    "duck-quack": ("voice", 2, duck_quack, -3.0),
    "fox-yip": ("voice", 1, fox_yip, -3.0),
    "butterfly-land": ("ui", 1, butterfly_land, 0.0),
    "whistle-call": ("voice", 1, whistle_call, -3.0),
    "critter-friend": ("ui", 1, critter_friend, 1.0),
    "leaf-rustle": ("body", 1, leaf_rustle, -3.0),
    "roamer-hello": ("ui", 2, roamer_hello, 1.0),  # (workstream D)
    "hands-clap": ("care", 3, hands_clap, -2.0),
    "soft-snore": ("care", 2, soft_snore, -4.0),
}


def finish(x: list, kind: str, gain_db: float) -> list:
    """As process_sfx.py finishes a take: the kind's high-pass and low shelf at 200 Hz,
    trailing silence trimmed, clean fades, then levelled so the loudest 100 ms meets the
    kind's target with peaks under -1 dBFS (the soft limiter for the kinds that allow it)."""
    k = KINDS[kind]
    x = biquad(x, "hp", k["hp"])
    if k["shelf"]:
        x = biquad(x, "shelf", 200, gain_db=k["shelf"])
    block = RATE // 400  # 2.5 ms, as process_sfx.trim
    env = [max(abs(v) for v in x[i:i + block]) for i in range(0, len(x), block)]
    top = max(env)
    last = max(i for i, e in enumerate(env) if e > top * 10 ** (-50 / 20))
    x = x[:min(len(x), (last + 1) * block + n_of(0.02))]
    fade_in(x, 0.0015)
    fade_out(x, min(0.03, len(x) / RATE / 4))
    level, peak = loudest_rms(x), max(abs(v) for v in x)
    g = min(k["target"] + gain_db - db(level), PEAK_CEILING + k["limit"] - db(peak))
    if k["limit"]:
        return list(soft_limit(x, 10 ** (g / 20)))
    return [v * 10 ** (g / 20) for v in x]


# ------------------------------------------------------------------------------------ the beds
class Loop:
    """A loop's buffer: whatever is added past its end wraps round to its start."""

    def __init__(self, seconds: float):
        self.n = n_of(seconds)
        self.buf = [0.0] * self.n

    def add(self, src: list, at: float, gain: float = 1.0) -> None:
        i0 = int(round(at * RATE))
        n, buf = self.n, self.buf
        for i, v in enumerate(src):
            buf[(i0 + i) % n] += v * gain

    def layer(self, src: list, gain: float = 1.0) -> None:
        for i, v in enumerate(src):
            self.buf[i] += v * gain


def round_loop(x: list, chain) -> list:
    """Runs a filter chain on the loop twice over and keeps the second pass: the filters have
    settled into the loop by then, so its end runs straight into its start."""
    y = chain(x + x)
    return y[len(x):]


def periodic_curve(n: int, rng: random.Random, lo: float, hi: float, cycles=(1, 2, 3)) -> list:
    """A slow random swell between lo and hi that repeats exactly in n samples (whole cycles)."""
    parts = [(c, rng.uniform(0.5, 1.0) / c, rng.uniform(0, TAU)) for c in cycles]
    # worked out every 16 samples (the last points past the end, where it has come round again)
    raw = [sum(a * math.cos(TAU * c * j * 16 / n + p) for c, a, p in parts) for j in range(n // 16 + 2)]
    mn, mx = min(raw), max(raw)
    coarse = [lo + (hi - lo) * (v - mn) / (mx - mn) for v in raw]
    return [coarse[i // 16] + (coarse[i // 16 + 1] - coarse[i // 16]) * (i % 16) / 16 for i in range(n)]


def bed_cove(rng):
    """Driftwood Cove: small waves lapping at the beach. The sea's soft hush far out, and three
    waves (unevenly spaced) each swelling up the sand and drawing back as a fizz of foam with
    tiny bubbles."""
    L = Loop(8.0)
    hush = round_loop(noise(L.n, rng), lambda s: biquad(biquad(biquad(s, "lp", 650), "lp", 900), "hp", 110))
    L.layer(norm(mul(hush, periodic_curve(L.n, rng, 0.75, 1.0))), 0.22)
    for at, size in ((0.15, 1.0), (2.85, 0.8), (5.35, 0.92)):
        at += rng.uniform(-0.05, 0.05)
        n = n_of(4.0)
        swell = mul(svf(noise(n, rng), sweep(n, 320, 900, 0.7), 0.7), hump(n, 0.26, 1.2, 1.6))
        L.add(norm(swell), at, 0.55 * size)
        m = n_of(3.0)
        fizz_env = [e * (0.75 + 0.25 * rng.random()) for e in hump(m, 0.12, 1.0, 2.2)]
        fizz = mul(biquad(biquad(noise(m, rng), "hp", 1700), "lp", 5200), fizz_env)
        L.add(norm(fizz), at + 0.75, 0.3 * size)
        for _ in range(10):
            L.add(droplet(rng.uniform(1300, 3200), tau=rng.uniform(0.004, 0.008), rise=1.4),
                  at + 0.9 + rng.uniform(0, 1.6), 0.05 * size * rng.uniform(0.4, 1.0))
    return L.buf


def bed_caldera(rng):
    """Emberpeak Caldera: the crater's low rumble swelling slowly (with a body around 300 Hz,
    so the little speakers carry some of it), a steady fine crackle of embers and two soft lava
    blorps."""
    L = Loop(8.0)
    rumble = round_loop(noise(L.n, rng), lambda s: biquad(biquad(biquad(biquad(s, "lp", 240), "lp", 240), "hp", 55),
                                                            "hp", 55))
    L.layer(norm(mul(rumble, periodic_curve(L.n, rng, 0.65, 1.0))), 0.6)
    body = round_loop(noise(L.n, rng), lambda s: biquad(biquad(s, "bp", 300, 0.9), "lp", 600))
    L.layer(norm(mul(body, periodic_curve(L.n, rng, 0.6, 1.0, (1, 2, 4)))), 0.4)
    t = 0.0
    while t < 8.0:  # embers: many small crackles, none standing out
        t += rng.expovariate(10.0)
        a = min(1.0, 0.35 + 0.45 * rng.random() ** 2)
        L.add(crackle(rng, rng.uniform(1500, 4500), rng.uniform(0.0006, 0.0025), 0.9), t, 0.13 * a)
        if rng.random() < 0.12:  # now and then a quick little cluster
            for k in range(rng.randint(2, 3)):
                L.add(crackle(rng, rng.uniform(1800, 4200), 0.001, 0.9), t + 0.012 * (k + 1), 0.08 * a)
    for at in (1.7 + rng.uniform(0, 0.4), 5.6 + rng.uniform(0, 0.4)):
        n = n_of(0.4)
        L.add(glide(150, 250, 0.4, 0.22, env=hump(n, 0.35, 1.0, 1.4), harmonics=((1, 1.0), (2, 0.4), (3, 0.15))),
              at, 0.12)
    return L.buf


def bed_glade(rng):
    """Moonpetal Glade at night: a faint glowing shimmer of a chord breathing slowly, a soft
    breeze, and wind chimes (G pentatonic, high and gentle) now and then, their long tails
    ringing over each other. No crickets: the valley's night bed brings those."""
    L = Loop(10.0)
    for nm, g in (("G4", 1.0), ("D5", 0.8), ("B5", 0.5), ("E5", 0.45)):
        f = note(nm)
        f = round(f * 10.0) / 10.0  # whole cycles in 10 s, so it loops
        swell = periodic_curve(L.n, rng, 0.25, 1.0, (1, 2))
        tone = [math.sin(TAU * f * i / RATE) + 0.5 * math.sin(TAU * (f + 0.3) * i / RATE) for i in range(L.n)]
        L.layer(norm(mul(tone, swell)), 0.035 * g)
    breeze = round_loop(noise(L.n, rng), lambda s: biquad(biquad(s, "bp", 520, 0.5), "lp", 1400))
    L.layer(norm(mul(breeze, periodic_curve(L.n, rng, 0.3, 1.0))), 0.12)
    pent = ("D6", "E6", "G6", "A6", "B6", "D7")
    t, last = 0.3, None
    while t < 9.6:
        nm = rng.choice([p for p in pent if p != last])
        last = nm
        L.add(chime(note(nm), 2.4, decay=rng.uniform(0.55, 0.9), bright=0.45), t, 0.28 * rng.uniform(0.55, 1.0))
        t += rng.uniform(0.55, 1.5)
    return L.buf


def bed_hollow(rng):
    """Frostspire Hollow: a cold wind moaning softly round the ice (a hollow band of noise
    drifting in pitch, a thin whistle above it) and water dripping in the cave, each drip
    answered by a faint echo."""
    L = Loop(8.0)
    fc = periodic_curve(L.n, rng, 380, 820)
    moan = round_loop(noise(L.n, rng), lambda s: svf(s, fc + fc, 2.5))
    L.layer(norm(mul(moan, periodic_curve(L.n, rng, 0.55, 1.0))), 0.5)
    air = round_loop(noise(L.n, rng), lambda s: biquad(biquad(s, "lp", 900), "hp", 120))
    L.layer(norm(air), 0.16)
    fw = periodic_curve(L.n, rng, 1300, 1900, (1, 2))
    whistle = round_loop(noise(L.n, rng), lambda s: svf(s, fw + fw, 9.0))
    L.layer(norm(mul(whistle, periodic_curve(L.n, rng, 0.2, 1.0, (1, 3)))), 0.1)
    for at in (0.9, 2.7, 4.6, 6.4):
        at += rng.uniform(-0.2, 0.2)
        d = droplet(rng.uniform(850, 1350), tau=0.02, rise=1.9)
        L.add(d, at, 0.28)
        for delay, g in ((0.13, 0.35), (0.27, 0.15)):  # the cave answers
            L.add(biquad(d, "lp", 2200), at + delay + rng.uniform(-0.01, 0.01), 0.28 * g)
    return L.buf


def bed_rush(rng):
    """Rushing wind for speed: even and featureless (the game raises it with speed), a roar
    low down, a brighter hiss above and a faint flutter."""
    L = Loop(4.0)
    base = round_loop(noise(L.n, rng), lambda s: biquad(biquad(s, "hp", 250), "lp", 4000))
    roar = round_loop(noise(L.n, rng), lambda s: biquad(s, "bp", 450, 0.7))
    hiss = round_loop(noise(L.n, rng), lambda s: biquad(s, "bp", 2500, 0.8))
    swell = periodic_curve(L.n, rng, 0.85, 1.0, (1, 2))
    flutter = [1.0 + 0.1 * math.sin(TAU * 39 * i / L.n) for i in range(L.n)]  # 9.75 Hz, whole cycles
    env = mul(swell, flutter)
    L.layer(mul(norm(base), env), 0.45)
    L.layer(mul(norm(roar), env), 0.6)
    L.layer(mul(norm(hiss), env), 0.3)
    return L.buf


BEDS = {  # slug: (maker, RMS dBFS)
    "amb-cove": (bed_cove, -26.0),
    "amb-caldera": (bed_caldera, -26.0),
    "amb-glade-night": (bed_glade, -27.0),
    "amb-hollow": (bed_hollow, -26.0),
    "amb-rush": (bed_rush, -26.0),
}


def finish_loop(x: list, target_rms: float) -> list:
    """The loops' finish (process_sfx's 'loop' kind): high-passed round the loop, levelled by
    overall RMS with the peak kept under -1 dBFS."""
    x = round_loop(x, lambda s: biquad(s, "hp", KINDS["loop"]["hp"]))
    g = min(target_rms - db(rms(x)), PEAK_CEILING - db(max(abs(v) for v in x)))
    return [v * 10 ** (g / 20) for v in x]


# ---------------------------------------------------------------------------------- the jingles
JINGLE_RATE = 32000


def jingle_battle_won(rng):
    """A battle won (~2.5 s): a bouncy marimba run up (C E G C E G), a bright chord (C E G C)
    with bells, then a little tag (G A B C) landing on a ringing C."""
    notes = [("C5", 0.0), ("E5", 0.1), ("G5", 0.2), ("C6", 0.3), ("E6", 0.4), ("G6", 0.5)]
    parts = [(note(n), t, "mallet", 0.5, 0.18) for n, t in notes]
    parts += [(note(n), 0.66, "mallet", 0.42, 0.45) for n in ("C5", "E5", "G5", "C6")]
    parts += [(note(n), 0.67, "chime", 0.22, 0.5) for n in ("G6", "C7")]
    parts += [(note(n), 1.1 + 0.1 * i, "kalimba", 0.45, 0.18) for i, n in enumerate(("G5", "A5", "B5"))]
    parts += [(note(n), 1.42, "mallet", 0.42, 0.7) for n in ("C5", "G5", "C6", "E6")]
    parts += [(note("C7"), 1.43, "chime", 0.25, 0.8)]
    return parts, 2.7


def jingle_level_up(rng):
    """A level up (~2 s): kalimba notes climbing two octaves (C D E G A C D E G) and a
    shimmering chord of chimes (C E G) with fairy dust."""
    run = ("C5", "D5", "E5", "G5", "A5", "C6", "D6", "E6", "G6")
    parts = [(note(n), 0.06 * i, "kalimba", 0.45, 0.22) for i, n in enumerate(run)]
    parts += [(note(n), 0.58, "chime", 0.38, 0.8) for n in ("C6", "E6", "G6")]
    parts += [(note(n), 0.58, "mallet", 0.35, 0.5) for n in ("C4", "G4", "C5")]
    for k in range(9):
        parts.append((note(rng.choice(("C7", "D7", "E7", "G7", "A7"))), 0.62 + 0.08 * k + rng.uniform(0, 0.03),
                      "chime", 0.1, 0.08))
    return parts, 2.2


def jingle_ribbon(rng):
    """A ribbon won (~2.8 s): a strummed chord of bells (C E G C), a gentle melody over it
    (E D C G E) and a warm last chord (F A C, then C) ringing out."""
    parts = [(note(n), 0.02 * i, "chime", 0.4, 0.8) for i, n in enumerate(("C5", "E5", "G5", "C6"))]
    for i, n in enumerate(("E6", "D6", "C6", "G5", "E6")):
        parts.append((note(n), 0.35 + 0.2 * i, "kalimba", 0.45, 0.3))
    parts += [(note(n), 1.35 + 0.02 * i, "chime", 0.32, 0.9) for i, n in enumerate(("F4", "A4", "C5", "F5"))]
    parts += [(note(n), 1.85 + 0.02 * i, "chime", 0.35, 1.0) for i, n in enumerate(("C5", "E5", "G5", "C6"))]
    return parts, 3.2


JINGLES = {"battle-won": jingle_battle_won, "new-level": jingle_level_up, "ribbon-won": jingle_ribbon}


def render_jingle(slug: str) -> Path:
    """Renders a jingle's notes (each panned a little by pitch, for a gentle stereo spread)
    into a 32 kHz stereo WAV in build/synth/, ready for make_loop.py --no-loop."""
    rng = random.Random(zlib.crc32(slug.encode()))
    parts, length = JINGLES[slug](rng)
    left, right = [0.0] * n_of(length), [0.0] * n_of(length)
    for f, t, inst, gain, decay in parts:
        if inst == "mallet":
            s = mallet(f, decay * 5, decay, 0.8, rng)
        elif inst == "kalimba":
            s = kalimba(f, decay * 5, decay)
        else:
            s = chime(f, decay * 5, decay, 0.55)
        pan = max(-0.6, min(0.6, math.log2(f / 700.0) * 0.3))  # low notes a little left, high a little right
        add(left, s, t, gain * math.cos((pan + 1) * math.pi / 4))
        add(right, s, t, gain * math.sin((pan + 1) * math.pi / 4))
    n = max(len(left), len(right))
    left += [0.0] * (n - len(left))
    right += [0.0] * (n - len(right))
    left, right = biquad(left, "hp", 90), biquad(right, "hp", 90)
    fade_out(left, 0.25)
    fade_out(right, 0.25)
    peak = max(max(abs(v) for v in left), max(abs(v) for v in right))
    k = 10 ** (-3 / 20) / peak
    ratio = RATE / JINGLE_RATE  # up to the music's rate (linear interpolation; the notes are well below it)
    frames = []
    for i in range(int(n / ratio)):
        p = i * ratio
        j = int(p)
        fr = p - j
        for ch in (left, right):
            a = ch[j]
            b = ch[j + 1] if j + 1 < n else 0.0
            frames.append(max(-32767, min(32767, int(round((a + (b - a) * fr) * k * 32767)))))
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    out = BUILD_DIR / f"{slug}.wav"
    with wave.open(str(out), "wb") as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(JINGLE_RATE)
        w.writeframes(struct.pack(f"<{len(frames)}h", *frames))
    return out


# ------------------------------------------------------------------------------------- output
def write_wav(path: Path, x: list) -> None:
    pcm = [max(-32767, min(32767, int(round(v * 32767)))) for v in x]
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(struct.pack(f"<{len(pcm)}h", *pcm))


def make_effect(slug: str) -> list[str]:
    kind, takes, maker, gain_db = EFFECTS[slug]
    lines = []
    for take in range(takes):
        rng = random.Random(zlib.crc32(f"{slug}:{take}".encode()))
        y = finish(maker(rng, take), kind, gain_db)
        out = OUT_DIR / (f"{slug}.wav" if take == 0 else f"{slug}-{take + 1}.wav")
        write_wav(out, y)
        lines.append(f"[synth] {out.name:22s} {len(y) / RATE:5.2f} s  level {db(loudest_rms(y)):6.1f} dB"
                     f"  peak {db(max(abs(v) for v in y)):5.1f} dBFS  ({kind})")
    return lines


def make_bed(slug: str) -> list[str]:
    maker, target = BEDS[slug]
    rng = random.Random(zlib.crc32(slug.encode()))
    y = finish_loop(maker(rng), target)
    write_wav(OUT_DIR / f"{slug}.wav", y)
    seam = abs(y[0] - y[-1])
    step = sorted(abs(y[i] - y[i - 1]) for i in range(1, len(y)))[int(0.999 * (len(y) - 1))]
    return [f"[synth] {slug + '.wav':22s} {len(y) / RATE:5.2f} s  rms {db(rms(y)):6.1f} dB"
            f"  peak {db(max(abs(v) for v in y)):5.1f} dBFS  seam step {seam * 32767:5.0f} (99.9% of steps < {step * 32767:.0f})"]


def make_jingle(slug: str) -> list[str]:
    wav = render_jingle(slug)
    subprocess.run([sys.executable, str(Path(__file__).with_name("make_loop.py")), str(wav), "--no-loop",
                    "--slug", slug, "--out-dir", str(BUILD_DIR)], check=True)
    return [f"[synth] build/synth/{slug}.ogg (to hear first; ship it by copying it into romfs/music)"]


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("slugs", nargs="*", help="only these (default: every effect and bed)")
    ap.add_argument("--list", action="store_true", help="list the sounds and what each should sound like")
    ap.add_argument("--jingles", action="store_true", help="also make the stingers (into build/synth; needs ffmpeg)")
    args = ap.parse_args()
    makers = {**{s: v[2] for s, v in EFFECTS.items()}, **{s: v[0] for s, v in BEDS.items()}, **JINGLES}
    if args.list:
        for slug, fn in makers.items():
            takes = EFFECTS[slug][1] if slug in EFFECTS else 1
            doc = " ".join((fn.__doc__ or "").split())
            print(f"{slug:16s} x{takes}  {doc}")
        return
    wanted = args.slugs or [*EFFECTS, *BEDS, *(JINGLES if args.jingles else [])]
    unknown = [s for s in wanted if s not in makers]
    if unknown:
        sys.exit(f"error: unknown slug(s) {unknown}")
    # Sounds generated since (tools/audio/eleven_sfx.py, recorded in the manifest) keep their files
    # unless named: the synth is their fallback, not their replacement.
    manifest = json.loads((Path(__file__).with_name("sfx_manifest.json")).read_text(encoding="utf-8"))
    generated = {s for s, e in manifest.items() if isinstance(e, dict) and any(t.startswith("eleven:") for t in e.get("takes", []))}
    if not args.slugs and generated & set(wanted):
        print(f"[synth] keeping the generated {', '.join(sorted(generated & set(wanted)))} (name one to make its synth)")
        wanted = [s for s in wanted if s not in generated]
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for slug in wanted:
        lines = make_effect(slug) if slug in EFFECTS else make_bed(slug) if slug in BEDS else make_jingle(slug)
        for line in lines:
            print(line)
    total = sum((OUT_DIR / f"{s}.wav").stat().st_size + sum(p.stat().st_size for p in OUT_DIR.glob(f"{s}-[0-9].wav"))
                for s in [*EFFECTS, *BEDS] if (OUT_DIR / f"{s}.wav").exists())
    print(f"[synth] the synthesised set in romfs/sfx: {total / 1024:.0f} KB")


if __name__ == "__main__":
    main()
