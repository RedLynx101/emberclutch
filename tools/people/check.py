"""Read the people's files back the way the game does and check the budgets (plain Python).

  python tools/people/check.py            every romfs/people/*.ecm, romfs/anims/person.eca, the portraits

The readers mirror src/core/model.cpp loadModel and src/core/anim.cpp loadAnims check for check
(magic and version, bone count <= 40 with parents first, palette <= 25 bones and in range, 1-4
keys, whole triangles, skin bones inside the palette, indices inside the mesh, regions <= 8,
v4 pieces; clip fps, frames, speed, bone modes, events inside the clip; no bytes left over).
Then the contract: the skeleton is rig.BONE_ORDER, the body mesh and eyes (variants 0 and 1) are
there, a player has hair group 10 variants 0-5, at most about 600 triangles drawn (body + eyes +
the largest hair), every clip the brief lists, footsteps on walk and run, 64 x 64 RGBA portraits.
Exit code 1 on any problem.
"""
import math
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tools", "anim"))
from person_clips import REQUIRED  # noqa: E402
from rig import BONE_ORDER  # noqa: E402

MAX_BONES, MAX_PALETTE, PALETTE_FIELD, MAX_KEYS, REGION_CLEAN = 40, 25, 32, 4, 8
TRI_BUDGET = 600
PLAYERS = ("player_a", "player_b")
VILLAGERS = ("keeper", "market", "sanctuary", "steward", "child", "traveller")
EVENT_FOOTSTEP = 1


class Reader:
    def __init__(self, data):
        self.b, self.at, self.ok = data, 0, True

    def take(self, fmt):
        n = struct.calcsize(fmt)
        if self.at + n > len(self.b):
            self.ok = False
            raise ValueError("ran past the end")
        v = struct.unpack_from(fmt, self.b, self.at)
        self.at += n
        return v


def load_model(path):
    """src/core/model.cpp loadModel, returning (bones, meshes) or raising ValueError."""
    c = Reader(open(path, "rb").read())
    magic, version, nb = c.take("<4sHH")
    if magic != b"ECM1" or version not in (3, 4):
        raise ValueError("not ECM1 v3/v4")
    if nb == 0 or nb > MAX_BONES:
        raise ValueError(f"{nb} bones")
    bones = []
    for i in range(nb):
        name, parent, flags = c.take("<16sbB2x")
        rest = c.take("<12f")
        if parent >= i:
            raise ValueError("a parent after its child")
        bones.append((name.split(b"\0")[0].decode(), parent, flags, rest))
    c.take(f"<{nb * 3}f")          # scale at t = 0
    c.take(f"<{3 * nb * 2}f")      # builds
    c.take(f"<{nb * 3}f")          # idle pose
    c.take(f"<{nb}f")              # young head lift
    (nm,) = c.take("<H")
    meshes = []
    for _ in range(nm):
        name, kind, group, variant, sex, npal = c.take("<16sBBBBB")
        pal = c.take(f"<{MAX_PALETTE}B")
        c.take(f"<{PALETTE_FIELD - MAX_PALETTE}x")
        (nkeys,) = c.take("<B")
        c.take("<3x")
        key_t = c.take("<4f")
        nv, ni = c.take("<HH")
        if npal > MAX_PALETTE or nkeys == 0 or nkeys > MAX_KEYS or ni % 3:
            raise ValueError(f"mesh header ({npal} palette, {nkeys} keys, {ni} indices)")
        if any(pal[i] >= nb for i in range(npal)):
            raise ValueError("palette bone out of range")
        pos = nrm = None
        for _k in range(nkeys):  # per key: the positions, then the normals
            p_k, n_k = c.take(f"<{nv * 3}f"), c.take(f"<{nv * 3}f")
            pos, nrm = pos or p_k, nrm or n_k
        skin = c.take(f"<{nv * 4}B")
        paint = c.take(f"<{nv * 4}B")
        c.take(f"<{nv * 2}f")
        region = c.take(f"<{nv}B")
        idx = c.take(f"<{ni}H")
        if version >= 4:
            (pieces,) = c.take("<B")
            if pieces:
                piece = c.take(f"<{nv}B")
                c.take(f"<{nkeys * 3 * pieces * 3}f")
                if any(p >= pieces for p in piece):
                    raise ValueError("piece out of range")
        for v in range(nv):
            if skin[v * 4] >= npal or skin[v * 4 + 1] >= npal:
                raise ValueError(f"{name}: skin bone outside the palette")
            if skin[v * 4 + 2] + skin[v * 4 + 3] != 255:
                raise ValueError(f"{name}: weights don't sum to 255")
        if any(i >= nv for i in idx):
            raise ValueError(f"{name}: index out of range")
        if any(r > REGION_CLEAN for r in region):
            raise ValueError(f"{name}: region out of range")
        for v in range(nv):
            n = nrm[v * 3:v * 3 + 3]
            if abs(math.sqrt(sum(x * x for x in n)) - 1.0) > 1e-3:
                raise ValueError(f"{name}: a normal isn't unit length")
            if paint[v * 4] > 9 or paint[v * 4 + 1] > 9:
                raise ValueError(f"{name}: paint slot out of range")
        meshes.append(dict(name=name.split(b"\0")[0].decode(), kind=kind, group=group, variant=variant,
                           palette=[pal[i] for i in range(npal)], tris=ni // 3, verts=nv, pos=pos, idx=idx))
    if c.at != len(c.b):
        raise ValueError(f"{len(c.b) - c.at} bytes left over")
    return bones, meshes


def load_anims(path):
    """src/core/anim.cpp loadAnims, returning (bone names, clips)."""
    c = Reader(open(path, "rb").read())
    magic, version, nb, nc = c.take("<4sHHH")
    if magic != b"ECA1" or version != 1:
        raise ValueError("not ECA1 v1")
    if nb == 0 or nb > MAX_BONES:
        raise ValueError(f"{nb} bones")
    names = [c.take("<16s")[0].split(b"\0")[0].decode() for _ in range(nb)]
    clips = []
    for _ in range(nc):
        name, fps, frames, flags, nev, speed = c.take("<16sfHBBf")
        name = name.split(b"\0")[0].decode()
        if frames == 0 or not fps > 0 or not speed >= 0:
            raise ValueError(f"{name}: bad header")
        for _b in range(nb):
            (mode,) = c.take("<B")
            if mode > 2:
                raise ValueError(f"{name}: bone mode {mode}")
            n = 0 if mode == 0 else (1 if mode == 1 else frames)
            for q in [c.take("<4h") for _k in range(n)]:
                if sum((v / 32767.0) ** 2 for v in q) < 0.5:
                    raise ValueError(f"{name}: a degenerate quaternion")
        root = c.take(f"<{frames * 2}f") if flags & 2 else ()
        events = []
        for _e in range(nev):
            frame, eid = c.take("<HBx")
            if frame >= frames:
                raise ValueError(f"{name}: event past the end")
            events.append((frame, eid))
        clips.append(dict(name=name, fps=fps, frames=frames, loop=bool(flags & 1), speed=speed, events=events,
                          root=bool(root)))
    if c.at != len(c.b):
        raise ValueError(f"{len(c.b) - c.at} bytes left over")
    return names, clips


def png_size(path):
    b = open(path, "rb").read()
    if b[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG")
    w, h, depth, colour = struct.unpack(">IIBB", b[16:26])
    return w, h, colour  # colour 6 = RGBA


def check_person(pid, problems):
    path = os.path.join(ROOT, "romfs", "people", f"{pid}.ecm")
    try:
        bones, meshes = load_model(path)
    except (ValueError, OSError, struct.error) as e:
        problems.append(f"{pid}: {e}")
        return
    names = [b[0] for b in bones]
    if names != BONE_ORDER:
        problems.append(f"{pid}: skeleton differs from rig.BONE_ORDER")
    for need in ("hand_L", "hand_R", "foot_L", "foot_R", "head", "eyes", "chest"):
        if need not in names:
            problems.append(f"{pid}: no bone {need}")
    eyes = names.index("eyes")
    er = bones[eyes][3]
    if abs(er[8] - 0.0) > 1e-6 or abs(er[10] - 1.0) > 1e-4:  # rows: (r00 r01 r02 tx) (r10..) (r20 r21 r22 tz)
        problems.append(f"{pid}: the eyes bone's Z isn't world up (blinks would squash the wrong way)")
    have = {(m["kind"], m["group"], m["variant"]): m for m in meshes}
    for need in ((0, 255, 0), (2, 0, 0), (2, 0, 1)):
        if need not in have:
            problems.append(f"{pid}: missing mesh kind {need[0]} group {need[1]} variant {need[2]}")
    hair = [have[(2, 10, v)]["tris"] for v in range(6) if (2, 10, v) in have]
    if pid in PLAYERS and len(hair) != 6:
        problems.append(f"{pid}: {len(hair)} hair styles (want 6: group 10, variants 0-5)")
    for m in meshes:
        if len(m["palette"]) > MAX_PALETTE:
            problems.append(f"{pid}/{m['name']}: {len(m['palette'])} bones in one draw")
    if (2, 0, 0) in have and set(have[(2, 0, 0)]["palette"]) != {eyes}:
        problems.append(f"{pid}: the eyes aren't on the eyes bone")
    body = have.get((0, 255, 0), {}).get("tris", 0)
    total = body + have.get((2, 0, 0), {}).get("tris", 0) + (max(hair) if hair else 0)
    if total > TRI_BUDGET:
        problems.append(f"{pid}: {total} triangles (budget {TRI_BUDGET})")
    low = min(have[(0, 255, 0)]["pos"][2::3]) if (0, 255, 0) in have else 0
    print(f"  {pid:10s} {len(bones)} bones, body draw {len(have.get((0, 255, 0), {}).get('palette', []))} bones, "
          f"triangles body {body} + eyes {have.get((2, 0, 0), {}).get('tris', 0)}"
          + (f" + hair {min(hair)}-{max(hair)}" if hair else "") + f" = {total} (lowest point z {low:+.3f}), "
          f"{os.path.getsize(path) // 1024} KB")


def main():
    problems = []
    print("[check] people")
    for pid in PLAYERS + VILLAGERS:
        check_person(pid, problems)
    try:
        names, clips = load_anims(os.path.join(ROOT, "romfs", "anims", "person.eca"))
        if names != BONE_ORDER:
            problems.append("person.eca: bones differ from rig.BONE_ORDER")
        by = {c["name"]: c for c in clips}
        missing = [n for n in REQUIRED if n not in by]
        if missing:
            problems.append(f"person.eca: missing clips {missing}")
        for n in ("walk", "run"):
            c = by.get(n)
            if c and (not c["loop"] or sum(1 for _, e in c["events"] if e == EVENT_FOOTSTEP) < 2 or c["speed"] <= 0):
                problems.append(f"person.eca: {n} should loop with two footsteps and a speed")
        print(f"  person.eca: {len(clips)} clips, " + ", ".join(
            f"{c['name']} {c['frames'] / c['fps'] if c['loop'] else (c['frames'] - 1) / c['fps']:.2f}s" for c in clips))
    except (ValueError, OSError, struct.error) as e:
        problems.append(f"person.eca: {e}")
    for pid in VILLAGERS:
        path = os.path.join(ROOT, "assets", "sprites", "people", f"{pid}.png")
        try:
            w, h, colour = png_size(path)
            if (w, h, colour) != (64, 64, 6):
                problems.append(f"{pid}.png: {w}x{h} colour type {colour} (want 64x64 RGBA)")
        except (ValueError, OSError) as e:
            problems.append(f"{pid}.png: {e}")
    print("[check] " + ("OK" if not problems else "PROBLEMS"))
    for p in problems:
        print(f"  PROBLEM {p}")
    sys.exit(1 if problems else 0)


if __name__ == "__main__":
    main()
