"""Checks a kind's exported files against the budgets and the contract (plain Python).

  python tools/dragons/check.py pouncer [crestwing ...]      (or --all)

For each kind: romfs/dragons/<kind>/{hatchling,grown}[_lod1].ecm and their skins exist; the
skeleton is the plan's (at most 40 bones, body bones first, the names the game looks up);
every draw uses at most 25 bones; eyes (round and slit pupils), heart and mouth are there;
the triangle budget holds for the common and the rare variant (LOD0 3,000, LOD1 1,200);
the plan's clips (romfs/anims/<plan>.eca) cover every required clip. Exit code 1 on failure.
"""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import dragons  # noqa: E402
from dragons.clip_names import REQUIRED  # noqa: E402

GROUP_NAMES = {0: "eyes", 1: "horns", 2: "frill", 3: "spikes", 4: "tail_tip", 5: "heart", 6: "wings", 7: "mouth",
               9: "runes", 255: "body"}
BUDGET = {0: 3000, 1: 1200}
NEEDED_BONES = ("head", "snout", "jaw", "eyes", "chest")


def read_ecm(path):
    b = open(path, "rb").read()
    at = 0

    def take(fmt):
        nonlocal at
        v = struct.unpack_from(fmt, b, at)
        at += struct.calcsize(fmt)
        return v

    magic, version, nb = take("<4sHH")
    assert magic == b"ECM1" and version == 4, f"{path}: not an ECM v4"
    bones = []
    for _ in range(nb):
        name, parent, flags = take("<16sbB2x")
        take("<12f")
        bones.append((name.split(b"\0")[0].decode(), parent, flags))
    take(f"<{nb * 3}f")
    take(f"<{3 * nb * 2}f")
    take(f"<{nb * 3}f")
    take(f"<{nb}f")
    (nm,) = take("<H")
    meshes = []
    for _ in range(nm):
        name, kind, group, variant, sex, npal, pal, nkeys, t0, t1, t2, t3, nv, ni = take("<16sBBBBB32sB3x4fHH")
        take(f"<{nkeys * nv * 6}f")
        take(f"<{nv * 4}B")
        take(f"<{nv * 4}B")
        take(f"<{nv * 2}f")
        take(f"<{nv}B")
        take(f"<{ni}H")
        (pieces,) = take("<B")
        if pieces:
            take(f"<{nv}B")
            take(f"<{nkeys * 3 * pieces * 3}f")
        meshes.append(dict(name=name.split(b"\0")[0].decode(), kind=kind, group=group, variant=variant,
                           palette=npal, tris=ni // 3, verts=nv))
    assert at == len(b), f"{path}: {len(b) - at} trailing bytes"
    return bones, meshes


def read_eca_names(path):
    b = open(path, "rb").read()
    magic, version, nb, nc = struct.unpack_from("<4sHHH", b, 0)
    assert magic == b"ECA1"
    at = 10 + 16 * nb
    names = []
    for _ in range(nc):
        name, fps, frames, flags, nev, speed = struct.unpack_from("<16sfHBBf", b, at)
        at += struct.calcsize("<16sfHBBf")
        for _bone in range(nb):
            (mode,) = struct.unpack_from("<B", b, at)
            at += 1
            if mode == 1:
                at += 8
            elif mode == 2:
                at += 8 * frames
        if flags & 2:
            at += 8 * frames
        at += 4 * nev
        names.append(name.split(b"\0")[0].decode())
    return names


def budget(meshes, rare):
    """Triangles drawn: body + wings (the rare variant's if it has them) + every part group
    (rare: its variant-1 meshes replace or add, as the kind says)."""
    body = sum(m["tris"] for m in meshes if m["kind"] == 0)
    wings = {m["variant"]: m["tris"] for m in meshes if m["kind"] == 1}
    w = wings.get(1, wings.get(0, 0)) if rare else wings.get(0, 0)
    parts = 0
    groups = {}
    for m in meshes:
        if m["kind"] == 2 and not (m["group"] == 0 and m["variant"] == 1):  # the slit eyes replace the round ones
            groups.setdefault(m["group"], {})[m["variant"]] = m["tris"]
    for g, vs in groups.items():
        if rare and 1 in vs:
            parts += vs[1] + (vs.get(0, 0) if not RARE_REPLACES else 0)
        else:
            parts += vs.get(0, 0)
    return body + w + parts


RARE_REPLACES = True


def check_kind(name):
    global RARE_REPLACES
    k = dragons.kind(name)
    plan = dragons.plan(k.META["plan"])
    RARE_REPLACES = k.META.get("rare_replaces", True)
    problems, lines = [], []
    folder = os.path.join(ROOT, "romfs", "dragons", name)
    assert len(k.VARIANTS) == 4 and k.META.get("rare_variant", 3) == 3, f"{name}: four variants, the rare one last"
    for form in ("hatchling", "grown"):
        for lod in (0, 1):
            stem = form + ("_lod1" if lod else "")
            ecm, skin = os.path.join(folder, stem + ".ecm"), os.path.join(folder, stem + "_skin.t3x")
            if not os.path.exists(ecm) or not os.path.exists(skin):
                problems.append(f"{stem}: missing .ecm or skin")
                continue
            bones, meshes = read_ecm(ecm)
            names = [b[0] for b in bones]
            if names != list(plan.BONE_ORDER):
                problems.append(f"{stem}: skeleton differs from plan {plan.NAME}")
            if len(bones) > 40:
                problems.append(f"{stem}: {len(bones)} bones (max 40)")
            for need in NEEDED_BONES + tuple(plan.CONTACTS):
                if need not in names:
                    problems.append(f"{stem}: no bone {need}")
            for m in meshes:
                if m["palette"] > 25:
                    problems.append(f"{stem}/{m['name']}: {m['palette']} bones in one draw (max 25)")
            have = {(m["kind"], m["group"], m["variant"]) for m in meshes}
            for need in ((0, 255, 0), (1, 6, 0), (2, 0, 0), (2, 0, 1), (2, 5, 0), (2, 7, 0)):
                if need not in have:
                    problems.append(f"{stem}: missing mesh kind {need[0]} group {GROUP_NAMES[need[1]]} variant {need[2]}")
            common, rare = budget(meshes, False), budget(meshes, True)
            if max(common, rare) > BUDGET[lod]:
                problems.append(f"{stem}: {max(common, rare)} triangles (budget {BUDGET[lod]})")
            lines.append(f"  {stem:15s} {len(bones)} bones, triangles common {common}, rare {rare} "
                         f"(budget {BUDGET[lod]}), {os.path.getsize(ecm) // 1024} KB")
    eca = os.path.join(ROOT, "romfs", "anims", f"{plan.NAME}.eca")
    if not os.path.exists(eca):
        problems.append(f"no {eca} (python tools/anim/build_anims.py --plan {plan.NAME})")
    else:
        clips = read_eca_names(eca)
        missing = [c for c in REQUIRED if c not in clips]
        if missing:
            problems.append(f"plan {plan.NAME}: missing clips {missing}")
        lines.append(f"  plan {plan.NAME}: {len(clips)} clips")
    print(f"[check] {name} ({k.META['title']}, {k.META['element']}, {k.META['rarity']}): "
          f"{'OK' if not problems else 'PROBLEMS'}")
    for ln in lines:
        print(ln)
    for p in problems:
        print(f"  PROBLEM {p}")
    return not problems


def main():
    names = [m.META["name"] for m in dragons.all_kinds()] if "--all" in sys.argv else \
        [a for a in sys.argv[1:] if not a.startswith("--")]
    ok = all([check_kind(n) for n in names])
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
