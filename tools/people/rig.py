"""The people's skeleton and the pose maths (pure Python), as the game does it.

One skeleton for everyone (docs/tech/people-kit.md): 18 bones, parents first. Each person has
their own bone positions (a child is smaller, the keeper stoops), as the dragons' two forms
share one plan. Conventions are the dragons' (tools/anim/eca.py): metres, Z up, the person
faces -Y, "_R" bones sit at +X and "_L" at -X (so "_L" is the person's own right side, as
Blender's front view names it).

Rest matrices follow Blender's bone convention (roll 0: vec_roll_to_mat3), so the file reads
like the dragons'. The pose maths reproduce src/core/skeleton.cpp (no scale: people don't grow)
and src/core/anim.cpp (clip deltas in armature axes, turned into each bone's local frame with
its rest rotation), so the previews show exactly what the game will.
"""
import math

# (name, parent) in file order: parents first. The body draw uses all but "eyes" (17 <= 25).
BONES = [
    ("hips", None), ("spine", "hips"), ("chest", "spine"), ("neck", "chest"), ("head", "neck"),
]
for _side in ("L", "R"):
    BONES += [(f"arm_up_{_side}", "chest"), (f"arm_lo_{_side}", f"arm_up_{_side}"),
              (f"hand_{_side}", f"arm_lo_{_side}"), (f"leg_up_{_side}", "hips"),
              (f"leg_lo_{_side}", f"leg_up_{_side}"), (f"foot_{_side}", f"leg_lo_{_side}")]
BONES.append(("eyes", "head"))
BONE_ORDER = [b[0] for b in BONES]
PARENT = dict(BONES)
INDEX = {n: i for i, n in enumerate(BONE_ORDER)}


# ------------------------------------------------------------------------------ vectors
def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def mul(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def length(a):
    return math.sqrt(dot(a, a))


def norm(a):
    n = length(a) or 1.0
    return (a[0] / n, a[1] / n, a[2] / n)


def lerp(a, b, t):
    return (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t)


# ------------------------------------------------------------------------------ matrices
# A 3x4 matrix is ((r00, r01, r02, tx), (r10, ...), (r20, ...)): columns 0-2 are the bone's
# X, Y (along the bone) and Z axes, column 3 its joint. Exactly what the .ecm stores.
def bone_matrix(head, tail):
    """Blender's rest matrix for a bone with roll 0 (vec_roll_to_mat3_normalized)."""
    x, y, z = norm(sub(tail, head))
    theta = 1.0 + y
    theta_alt = x * x + z * z
    if theta > 6.1e-3 or theta_alt > 2.5e-4 * 2.5e-4:
        if theta <= 6.1e-3:
            theta = theta_alt * 0.5 + theta_alt * theta_alt * 0.125
        cx = (1 - x * x / theta, -x, -x * z / theta)
        cz = (-x * z / theta, -z, 1 - z * z / theta)
    else:
        cx, cz = (-1.0, 0.0, 0.0), (0.0, 0.0, 1.0)
    cy = (x, y, z)
    return tuple((cx[r], cy[r], cz[r], head[r]) for r in range(3))


def m_mul(a, b):
    out = []
    for r in range(3):
        row = [sum(a[r][k] * b[k][c] for k in range(3)) for c in range(3)]
        row.append(sum(a[r][k] * b[k][3] for k in range(3)) + a[r][3])
        out.append(tuple(row))
    return tuple(out)


def m_inv(m):
    """Inverse of a rigid 3x4 (rotation + translation)."""
    rt = [[m[c][r] for c in range(3)] for r in range(3)]
    t = [m[0][3], m[1][3], m[2][3]]
    return tuple(tuple(rt[r]) + (-sum(rt[r][k] * t[k] for k in range(3)),) for r in range(3))


def m_point(m, p):
    return tuple(m[r][0] * p[0] + m[r][1] * p[1] + m[r][2] * p[2] + m[r][3] for r in range(3))


def m_dir(m, d):
    return tuple(m[r][0] * d[0] + m[r][1] * d[1] + m[r][2] * d[2] for r in range(3))


def m_from_quat(q, t=(0.0, 0.0, 0.0)):
    w, x, y, z = q
    return ((1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y), t[0]),
            (2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x), t[1]),
            (2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y), t[2]))


def quat_from_m(m):
    tr = m[0][0] + m[1][1] + m[2][2]
    if tr > 0:
        s = math.sqrt(tr + 1.0) * 2
        q = (0.25 * s, (m[2][1] - m[1][2]) / s, (m[0][2] - m[2][0]) / s, (m[1][0] - m[0][1]) / s)
    elif m[0][0] > m[1][1] and m[0][0] > m[2][2]:
        s = math.sqrt(1.0 + m[0][0] - m[1][1] - m[2][2]) * 2
        q = ((m[2][1] - m[1][2]) / s, 0.25 * s, (m[0][1] + m[1][0]) / s, (m[0][2] + m[2][0]) / s)
    elif m[1][1] > m[2][2]:
        s = math.sqrt(1.0 + m[1][1] - m[0][0] - m[2][2]) * 2
        q = ((m[0][2] - m[2][0]) / s, (m[0][1] + m[1][0]) / s, 0.25 * s, (m[1][2] + m[2][1]) / s)
    else:
        s = math.sqrt(1.0 + m[2][2] - m[0][0] - m[1][1]) * 2
        q = ((m[1][0] - m[0][1]) / s, (m[0][2] + m[2][0]) / s, (m[1][2] + m[2][1]) / s, 0.25 * s)
    return q_norm(q)


def q_mul(a, b):
    aw, ax, ay, az = a
    bw, bx, by, bz = b
    return (aw * bw - ax * bx - ay * by - az * bz, aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx, aw * bz + ax * by - ay * bx + az * bw)


def q_conj(q):
    return (q[0], -q[1], -q[2], -q[3])


def q_norm(q):
    n = math.sqrt(sum(c * c for c in q)) or 1.0
    return tuple(c / n for c in q)


# ------------------------------------------------------------------------------ skeleton
class Skeleton:
    """A person's bones: joints and tails in metres (rest pose = standing at ease)."""

    def __init__(self, joints):
        """joints: {bone: (head, tail)} for every bone in BONE_ORDER."""
        self.joints = joints
        self.rest = [bone_matrix(*joints[n]) for n in BONE_ORDER]
        self.inv_rest = [m_inv(m) for m in self.rest]
        self.rest_q = [quat_from_m(m) for m in self.rest]
        self.offs = []
        for i, n in enumerate(BONE_ORDER):
            p = PARENT[n]
            self.offs.append(self.rest[i] if p is None else m_mul(self.inv_rest[INDEX[p]], self.rest[i]))

    def pose(self, local_rot):
        """local_rot: per bone (in BONE_ORDER) a bone-local quaternion. Returns the skin matrices
        (pose * inverse rest), as src/core/skeleton.cpp evaluatePose does without scale."""
        pose_m = []
        for i, n in enumerate(BONE_ORDER):
            p = PARENT[n]
            local = m_mul(self.offs[i], m_from_quat(local_rot[i]))
            pose_m.append(local if p is None else m_mul(pose_m[INDEX[p]], local))
        return [m_mul(pm, ir) for pm, ir in zip(pose_m, self.inv_rest)], pose_m

    def clip_pose(self, clip, t):
        """Local rotations for a clip at time t (the idle pose is the rest pose: identity):
        delta_local = rest^-1 * delta_armature * rest (src/core/anim.cpp sampleClip)."""
        out = []
        for i, n in enumerate(BONE_ORDER):
            q = clip.sample_q(n, t) if clip is not None else (1.0, 0.0, 0.0, 0.0)
            rq = self.rest_q[i]
            out.append(q_norm(q_mul(q_mul(q_conj(rq), q), rq)))
        return out


def skin_points(skin, pos, weights):
    """Linear blend skinning with two bones a vertex (the shader's)."""
    out = []
    for p, w in zip(pos, weights):
        acc = (0.0, 0.0, 0.0)
        for b, wt in w:
            if wt > 0:
                acc = add(acc, mul(m_point(skin[INDEX[b]], p), wt))
        out.append(acc)
    return out


def skin_normals(skin, nrm, weights):
    out = []
    for n, w in zip(nrm, weights):
        acc = (0.0, 0.0, 0.0)
        for b, wt in w:
            if wt > 0:
                acc = add(acc, mul(m_dir(skin[INDEX[b]], n), wt))
        out.append(norm(acc))
    return out
