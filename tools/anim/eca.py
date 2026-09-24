"""Animation clips for the dragon: authoring helpers, 30 Hz sampling and the .eca writer.

Pure Python (no Blender), so `python tools/anim/build_anims.py` can rebuild the clips
anywhere; tools/blender/preview_anims.py renders them for review.

Conventions (docs/tech/architecture.md section 4, "Animation"):
  * A clip stores, per bone and frame, a rotation DELTA that the runtime applies on top of
    the dragon's idle pose. One clip set serves both body forms and every growth stage.
  * Deltas are authored as (pitch, yaw, roll) in degrees about ARMATURE axes, which read
    the same for every bone:  pitch +  tip swings up / forward
                               yaw   +  tip turns to the dragon's left (+X)
                               roll  +  top leans to the dragon's right (-X)
    The runtime turns them into bone-local rotations with each form's rest pose
    (q_local = q_rest^-1 * q * q_rest).
  * The dragon faces -Y with Z up. "_R" bones sit at +X, "_L" at -X; sym() mirrors a key
    onto both (yaw and roll flip).
  * Optional root track: (forward, up) offsets in adult units for hops and pounces.
  * Keys are joined by Catmull-Rom curves (wrapping for looping clips).
"""
import math
import struct

FPS = 30
EVENTS = {"footstep": 1, "chomp": 2, "swallow": 3, "flap": 4, "yawn": 5, "thump": 6, "land": 7,
          "sniff": 8, "shake": 9, "purr": 10}


# ------------------------------------------------------------------------------ quaternions
def q_axis(axis, degrees):
    h = math.radians(degrees) * 0.5
    s = math.sin(h)
    return (math.cos(h), axis[0] * s, axis[1] * s, axis[2] * s)


def q_mul(a, b):
    aw, ax, ay, az = a
    bw, bx, by, bz = b
    return (aw * bw - ax * bx - ay * by - az * bz,
            aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw)


def q_from_pyr(pitch, yaw, roll):
    """Armature-space rotation: roll (about -Y) first, then pitch (about -X), then yaw (+Z)."""
    q = q_axis((0.0, -1.0, 0.0), roll)
    q = q_mul(q_axis((-1.0, 0.0, 0.0), pitch), q)
    return q_mul(q_axis((0.0, 0.0, 1.0), yaw), q)


# ------------------------------------------------------------------------------ clips
class Clip:
    """A dragon animation. Add keys with key()/sym()/pose(), procedural motion with wave(),
    a root track with root(), and markers with event()."""

    def __init__(self, name, length, loop=False, speed=0.0):
        assert len(name) < 16, name
        self.name, self.length, self.loop, self.speed = name, length, loop, speed
        self.keys = {}        # bone -> {time: (pitch, yaw, roll)}
        self.waves = []       # f(t) -> {bone: (p, y, r)}, added on top of the keys
        self.root_keys = {}   # time -> (forward, up)
        self.events = []      # (time, name)
        self.pose_times, self.pose_bones = set(), set()
        self._filled = None

    def key(self, t, **bones):
        assert not self.loop or t < self.length, f"{self.name}: loop keys must be < length"
        for bone, value in bones.items():
            self.keys.setdefault(bone, {})[t] = tuple(float(v) for v in value)
        self._filled = None
        return self

    def sym(self, t, name, pitch=0.0, yaw=0.0, roll=0.0):
        """The same motion on both sides of the body."""
        self.key(t, **{f"{name}_R": (pitch, yaw, roll), f"{name}_L": (pitch, -yaw, -roll)})
        return self

    def pose(self, t, pose):
        """Key a whole pose: {bone or 'name*' (both sides): (p, y, r)}. A pose is complete:
        any bone posed elsewhere in the clip but missing here is at rest (0) at time t."""
        self.pose_times.add(t)
        for bone, value in pose.items():
            if bone.endswith("*"):
                self.sym(t, bone[:-1], *value)
                self.pose_bones.update((f"{bone[:-1]}_R", f"{bone[:-1]}_L"))
            else:
                self.key(t, **{bone: value})
                self.pose_bones.add(bone)
        return self

    def filled_keys(self):
        if self._filled is None:
            keys = {bone: dict(k) for bone, k in self.keys.items()}
            for bone in self.pose_bones:
                for t in self.pose_times:
                    keys[bone].setdefault(t, (0.0, 0.0, 0.0))
            self._filled = keys
        return self._filled

    def wave(self, fn):
        self.waves.append(fn)
        return self

    def root(self, t, forward=0.0, up=0.0):
        self.root_keys[t] = (float(forward), float(up))
        return self

    def event(self, t, name):
        self.events.append((t, name))
        return self

    # ---------------------------------------------------------------- sampling
    def frame_count(self):
        n = int(round(self.length * FPS))
        return n if self.loop else n + 1

    def sample(self, bone, t):
        value = [0.0, 0.0, 0.0]
        keys = self.filled_keys()
        if bone in keys:
            value = list(_curve(keys[bone], t, self.length, self.loop, 3))
        for fn in self.waves:
            extra = fn(t).get(bone)
            if extra:
                value = [a + b for a, b in zip(value, extra)]
        return tuple(value)

    def sample_root(self, t):
        if not self.root_keys:
            return (0.0, 0.0)
        return tuple(_curve(self.root_keys, t, self.length, self.loop, 2))


def _curve(keys, t, length, loop, dims):
    """Catmull-Rom through the keys (wrapping for loops, clamped otherwise)."""
    times = sorted(keys)
    if len(times) == 1:
        return keys[times[0]]
    pts = [(tk, keys[tk]) for tk in times]
    if loop:  # periodic: repeat the keys one period before and after
        t = t % length
        pts = [(tk - length, v) for tk, v in pts[-2:]] + pts + [(tk + length, v) for tk, v in pts[:2]]
    else:
        if t <= pts[0][0]:
            return pts[0][1]
        if t >= pts[-1][0]:
            return pts[-1][1]
    for i in range(len(pts) - 1):
        (t1, p1), (t2, p2) = pts[i], pts[i + 1]
        if t1 <= t <= t2:
            p0 = pts[i - 1][1] if i > 0 else p1
            p3 = pts[i + 2][1] if i + 2 < len(pts) else p2
            u = (t - t1) / (t2 - t1) if t2 > t1 else 0.0
            return tuple(_catmull(p0[k], p1[k], p2[k], p3[k], u) for k in range(dims))
    return pts[-1][1]


def _catmull(p0, p1, p2, p3, u):
    return 0.5 * ((2 * p1) + (-p0 + p2) * u + (2 * p0 - 5 * p1 + 4 * p2 - p3) * u * u +
                  (-p0 + 3 * p1 - 3 * p2 + p3) * u * u * u)


# ------------------------------------------------------------------------------ writer
def _quantize(q):
    return tuple(max(-32767, min(32767, int(round(c * 32767)))) for c in q)


def write_eca(path, clips, bone_order):
    """.eca v1 (read by src/core/anim.cpp):
      "ECA1" u16 version u16 boneCount u16 clipCount, boneCount x char[16] names;
      per clip: char[16] name, f32 fps, u16 frames, u8 flags (1 loop, 2 root track),
      u8 eventCount, f32 speed; per bone u8 mode (0 identity, 1 constant, 2 animated) then
      its quaternions as s16 (w, x, y, z) / 32767; if a root track, frames x (f32 forward,
      f32 up); events: u16 frame, u8 id, u8 pad. Rotations are armature-space deltas."""
    out = bytearray(b"ECA1" + struct.pack("<HHH", 1, len(bone_order), len(clips)))
    for name in bone_order:
        out += struct.pack("<16s", name.encode())
    for c in clips:
        unknown = set(c.filled_keys()) - set(bone_order)
        assert not unknown, f"{c.name}: unknown bones {sorted(unknown)}"
        n = c.frame_count()
        frames = [i / FPS for i in range(n)]
        root = [c.sample_root(t) for t in frames]
        has_root = any(abs(v) > 1e-6 for r in root for v in r)
        flags = (1 if c.loop else 0) | (2 if has_root else 0)
        out += struct.pack("<16sfHBBf", c.name.encode(), float(FPS), n, flags, len(c.events), c.speed)
        for bone in bone_order:
            qs, prev = [], None
            for t in frames:
                q = q_from_pyr(*c.sample(bone, t))
                if prev is not None and sum(a * b for a, b in zip(q, prev)) < 0:
                    q = tuple(-v for v in q)  # stay on one hemisphere so frames interpolate
                qs.append(q)
                prev = q
            quant = [_quantize(q) for q in qs]
            if all(q == (32767, 0, 0, 0) for q in quant):
                out += struct.pack("<B", 0)
            elif all(q == quant[0] for q in quant):
                out += struct.pack("<B4h", 1, *quant[0])
            else:
                out += struct.pack("<B", 2)
                for q in quant:
                    out += struct.pack("<4h", *q)
        if has_root:
            for fwd, up in root:
                out += struct.pack("<2f", fwd, up)
        for t, name in sorted(c.events):
            out += struct.pack("<HBx", min(n - 1, int(round(t * FPS))), EVENTS[name])
    with open(path, "wb") as f:
        f.write(bytes(out))
    return len(out)
