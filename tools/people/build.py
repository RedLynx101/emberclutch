"""Build the people's game files (pure Python, no Blender):

  python tools/people/build.py            romfs/people/<id>.ecm for everyone, romfs/anims/person.eca
  python tools/people/build.py --only keeper,player_a
  python tools/people/build.py --anims    just the clip library

Then `python tools/people/check.py` reads them back the way the game does.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tools", "anim"))
import person_clips  # noqa: E402
import people  # noqa: E402
from ecm import GROUP_BODY, GROUP_BROWS, GROUP_EYES, GROUP_HAIR, GROUP_MOUTH, KIND_BODY, KIND_PART, OutMesh,     write_ecm  # noqa: E402
from eca import write_eca  # noqa: E402
from rig import BONE_ORDER  # noqa: E402

OUT = os.path.join(ROOT, "romfs", "people")
ANIMS = os.path.join(ROOT, "romfs", "anims", "person.eca")


def meshes_of(p):
    """The body, every face variant (faces.py: eyes, mouths, brows; an empty one left out) and hair."""
    out = [OutMesh("body", KIND_BODY, GROUP_BODY, 0, p.body)]
    for name, group, parts in (("eyes", GROUP_EYES, p.eyes), ("mouth", GROUP_MOUTH, p.mouths),
                               ("brows", GROUP_BROWS, p.brows)):
        for k, m in enumerate(parts):
            if m.tris:
                out.append(OutMesh(f"{name}_{k}", KIND_PART, group, k, m))
    for k, h in enumerate(p.hair):
        out.append(OutMesh(f"hair_{k}", KIND_PART, GROUP_HAIR, k, h))
    return out


def stats(p, meshes):
    """Triangles drawn at most: the body, the largest of each face part and of the hair."""
    def most(group):
        return max([m.tris for m in meshes if m.group == group] or [0])
    body = meshes[0].tris
    eyes = most(GROUP_EYES) + most(GROUP_MOUTH) + most(GROUP_BROWS)
    hair = [m.tris for m in meshes if m.group == GROUP_HAIR]
    total = body + eyes + (max(hair) if hair else 0)
    return body, eyes, hair, total


def build_person(pid):
    p = people.build(pid)
    meshes = meshes_of(p)
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, f"{pid}.ecm")
    size = write_ecm(path, p.skel, meshes)
    body, eyes, hair, total = stats(p, meshes)
    hz = p.joints["leg_up_R"][0][2]
    top = max(q[2] for m in [p.body] + p.hair for q in m.pos)
    print(f"[people] {path}: {size} bytes, {len(BONE_ORDER)} bones, body palette {len(meshes[0].palette)}, "
          f"tris body {body} + face {eyes}" + (f" + hair {min(hair)}-{max(hair)}" if hair else "") +
          f" = {total} max; height {top:.3f} m, hips {hz:.3f} m")
    if "--verbose" in sys.argv:
        by = {}
        for t in p.body.tris:
            by[p.body.mat[t[0]]] = by.get(p.body.mat[t[0]], 0) + 1
        print("[people]   body by material: " + ", ".join(f"{k} {v}" for k, v in sorted(by.items())))
        if hair:
            print(f"[people]   hair styles: {hair}")
    return p, total


def build_anims():
    os.makedirs(os.path.dirname(ANIMS), exist_ok=True)
    size = write_eca(ANIMS, person_clips.CLIPS, BONE_ORDER)
    print(f"[people] {ANIMS}: {len(person_clips.CLIPS)} clips, {size} bytes")
    for c in person_clips.CLIPS:
        print(f"[people]   {c.name:16s} {c.length:4.2f} s {'loop' if c.loop else 'once'} "
              f"speed {c.speed:5.3f} events {[(round(t, 2), e) for t, e in sorted(c.events)]}")


def main():
    only = sys.argv[sys.argv.index("--only") + 1].split(",") if "--only" in sys.argv else list(people.PEOPLE)
    if "--anims" not in sys.argv:
        for pid in only:
            build_person(pid)
    build_anims()


if __name__ == "__main__":
    main()
