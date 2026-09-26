"""Write a person as an .ecm v4, the dragons' model format (read by src/core/model.cpp; the
layout is tools/blender/dragonkit/export.py's write_ecm, field for field):

  "ECM1" u16 version 4, u16 bone count; per bone: char[16] name, s8 parent, u8 flags, 2 pad,
  3x4 f32 rest matrix (rows; columns X, Y, Z axes and the joint); growth tables: per bone the
  scale at t = 0 (3 f32), per build (sturdy, sleek, long) per bone (girth, length), per bone the
  idle pose (Euler XYZ degrees), per bone the young head lift; u16 mesh count; per mesh:
  char[16] name, u8 kind, group, variant, sex, u8 palette count, u8[32] palette (bone indices),
  u8 key count, 3 pad, f32[4] key growth t, u16 vertex count, u16 index count; per key the
  positions then the normals (3 f32 each); per vertex skin (bone0, bone1, w0, w1: palette-local,
  w0 + w1 = 255), paint (palette A, B, mix, emissive), f32 UV, u8 region; u16 indices; u8 piece
  count (0 here).

People don't grow and have no builds: every growth scale and build multiplier is 1, the idle
pose is the rest pose (all zeros), each mesh has one key (t = 1). UVs sit on the skin texture's
clean corner (kCleanUv 0.97) and regions are kRegionClean (8): vertex paint only, never dirty.
"""
import struct

from looks import PAINT
from rig import BONE_ORDER, INDEX, PARENT

KIND_BODY, KIND_WINGS, KIND_PART = 0, 1, 2
GROUP_EYES, GROUP_HAIR, GROUP_BODY = 0, 10, 255
PALETTE_FIELD = 32
MAX_PALETTE = 25
CLEAN_UV = (0.97, 0.97)
REGION_CLEAN = 8


def quantize(w, local):
    """[(bone, weight)] -> (b0, b1, w0, w1) palette-local, w0 + w1 == 255."""
    pairs = sorted(((local[b], wt) for b, wt in w if wt > 0), key=lambda p: -p[1])[:2]
    assert pairs, "vertex without weights"
    if len(pairs) == 1:
        pairs.append((pairs[0][0], 0.0))
    total = pairs[0][1] + pairs[1][1] or 1.0
    w0 = int(round(255 * pairs[0][1] / total))
    return pairs[0][0], pairs[1][0], w0, 255 - w0


class OutMesh:
    def __init__(self, name, kind, group, variant, mesh):
        assert len(name) < 16, name
        self.name, self.kind, self.group, self.variant, self.mesh = name, kind, group, variant, mesh
        self.palette = [INDEX[b] for b in BONE_ORDER if b in set(mesh.bones())]
        assert len(self.palette) <= MAX_PALETTE, f"{name}: {len(self.palette)} bones in one draw (max 25)"

    @property
    def tris(self):
        return len(self.mesh.tris)


def write_ecm(path, skel, meshes):
    out = bytearray(b"ECM1" + struct.pack("<HH", 4, len(BONE_ORDER)))
    for i, name in enumerate(BONE_ORDER):
        parent = INDEX[PARENT[name]] if PARENT[name] else -1
        out += struct.pack("<16sbB2x", name.encode(), parent, 0)
        for r in range(3):
            out += struct.pack("<4f", *skel.rest[i][r])
    n = len(BONE_ORDER)
    out += struct.pack(f"<{3 * n}f", *([1.0] * 3 * n))       # scale at t = 0: grown already
    out += struct.pack(f"<{6 * n}f", *([1.0] * 6 * n))       # builds: none
    out += struct.pack(f"<{3 * n}f", *([0.0] * 3 * n))       # idle pose = the rest pose
    out += struct.pack(f"<{n}f", *([0.0] * n))               # young head lift: none
    out += struct.pack("<H", len(meshes))
    for om in meshes:
        m = om.mesh
        local = {BONE_ORDER[b]: i for i, b in enumerate(om.palette)}
        nv, ni = len(m.pos), 3 * len(m.tris)
        assert nv < 65536 and ni < 65536, om.name
        pal = bytes(om.palette + [0] * (PALETTE_FIELD - len(om.palette)))
        out += struct.pack(f"<16sBBBBB{PALETTE_FIELD}sB3x4fHH", om.name.encode(), om.kind, om.group, om.variant, 0,
                           len(om.palette), pal, 1, 1.0, 0.0, 0.0, 0.0, nv, ni)
        for p in m.pos:
            out += struct.pack("<3f", *p)
        for nrm in m.normals():
            out += struct.pack("<3f", *nrm)
        for w in m.w:
            out += struct.pack("<4B", *quantize(w, local))
        for mat in m.mat:
            out += struct.pack("<4B", *PAINT[mat])
        out += struct.pack("<2f", *CLEAN_UV) * nv
        out += bytes([REGION_CLEAN]) * nv
        out += struct.pack(f"<{ni}H", *[i for t in m.tris for i in t])
        out += struct.pack("<B", 0)
    with open(path, "wb") as f:
        f.write(bytes(out))
    return len(out)
