#include "app/render3d.hpp"

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>
#include <tex3ds.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/anim.hpp"
#include "core/daylight.hpp"
#include "core/dragon_mesh.hpp"
#include "core/egg.hpp"
#include "core/rig.hpp"
#include "core/static_mesh.hpp"
#include "dragon_shbin.h"
#include "static_shbin.h"

namespace ec::r3d {
namespace {

constexpr float kDegToRad = 3.14159265f / 180.0f;
constexpr float kFovY = 38.0f * kDegToRad;

// Matches the attribute loaders below and the shader's inputs v0..v4 (v5, the dust level,
// comes from a per-dragon buffer for the body and is a fixed value for everything else).
struct GpuVertex {
    float pos[3];
    float nrm[3];
    u8 skin[4];   // bone0, bone1 (palette-local), w0, w1
    u8 paint[4];  // palette A, palette B, mix, emissive
    float uv[2];  // the form's skin texture
};
static_assert(sizeof(GpuVertex) == 40, "shared with the shader's attribute layout");

// Dust (D46): a fully dirty region moves this far toward the dust colour (dragon_texture.py
// DIRT_MAX, DIRT_COLOR).
constexpr float kDirtMax = 0.4f;
constexpr u32 kDirtColor = 0xFF758594;  // ABGR of (148, 133, 117)

struct GpuMesh {
    GpuVertex* vbo = nullptr;  // linear memory, read by the GPU
    u16* ibo = nullptr;
    int vertexCount = 0, indexCount = 0, vertexCapacity = 0, indexCapacity = 0;
    u8 paletteCount = 0;
    u8 palette[kMaxPalette] = {};

    bool reserve(int verts, int indices) {
        if (verts <= vertexCapacity && indices <= indexCapacity) return true;
        release();
        vbo = static_cast<GpuVertex*>(linearAlloc(sizeof(GpuVertex) * verts));
        ibo = static_cast<u16*>(linearAlloc(sizeof(u16) * indices));
        if (!vbo || !ibo) {
            release();
            return false;
        }
        vertexCapacity = verts;
        indexCapacity = indices;
        return true;
    }
    void release() {
        if (vbo) linearFree(vbo);
        if (ibo) linearFree(ibo);
        *this = GpuMesh{};
    }
};

// Copies CPU mesh arrays into a GPU mesh and flushes them out of the CPU cache.
bool fill(GpuMesh& g, int n, const Vec3* pos, const Vec3* nrm, const u8* skin, const u8* paint, const float* uv,
          const u16* idx, int idxCount, const u8* palette, u8 paletteCount) {
    if (!g.reserve(n, idxCount)) return false;
    for (int v = 0; v < n; ++v) {
        GpuVertex& o = g.vbo[v];
        o.pos[0] = pos[v].x, o.pos[1] = pos[v].y, o.pos[2] = pos[v].z;
        o.nrm[0] = nrm[v].x, o.nrm[1] = nrm[v].y, o.nrm[2] = nrm[v].z;
        std::memcpy(o.skin, skin + std::size_t(v) * 4, 4);
        std::memcpy(o.paint, paint + std::size_t(v) * 4, 4);
        o.uv[0] = uv[std::size_t(v) * 2], o.uv[1] = uv[std::size_t(v) * 2 + 1];
    }
    std::memcpy(g.ibo, idx, sizeof(u16) * idxCount);
    g.vertexCount = n;
    g.indexCount = idxCount;
    g.paletteCount = paletteCount;
    std::memcpy(g.palette, palette, paletteCount);
    GSPGPU_FlushDataCache(g.vbo, sizeof(GpuVertex) * n);
    GSPGPU_FlushDataCache(g.ibo, sizeof(u16) * idxCount);
    return true;
}

bool fillStatic(GpuMesh& g, const MeshData& m) {
    return fill(g, m.vertexCount, m.pos.data(), m.nrm.data(), m.skin.data(), m.paint.data(), m.uv.data(),
                m.indices.data(), static_cast<int>(m.indices.size()), m.palette, m.paletteCount);
}

bool readFile(const char* path, std::vector<u8>& out) {
    FILE* file = std::fopen(path, "rb");
    if (!file) return false;
    std::fseek(file, 0, SEEK_END);
    out.resize(static_cast<std::size_t>(std::ftell(file)));
    std::fseek(file, 0, SEEK_SET);
    const bool read = std::fread(out.data(), 1, out.size(), file) == out.size();
    std::fclose(file);
    return read;
}

struct Form {
    ModelData model;
    const MeshData* bodyData = nullptr;  // its vertex regions build each dragon's dust stream
    GpuMesh body;
    GpuMesh wings[kWingsCount];
    C3D_Tex skin;                        // romfs:/models/<form>[_lod1]_skin.t3x
    bool skinOk = false;
    int headBone = -1, chestBone = -1, eyesBone = -1;
    bool ok = false;
};

// What was last built for a dragon on screen: one slot per den dragon plus a spare.
struct Cache {
    bool valid = false;
    u32 lastUsed = 0;  // frame counter, for least-recently-used replacement
    u32 id = 0;
    int form = -1;
    int lod = 0;
    float t = -1;
    Genome genome{};
    Sex sex = Sex::Female;
    GpuMesh parts;
    float ground = 0;          // lowest body vertex in the idle pose (armature space)
    Vec3 center{0, 0, 0};      // camera framing, armature space
    float radius = 1;
    float groundNow = 0;       // this frame's (smoothed) floor contact under the animated pose
    bool groundSet = false;
    u8* dust = nullptr;        // per body vertex: its region's dust level (4 bytes each, linear memory)
    int dustCount = 0;
    float dustShown[kRegionCount] = {};  // the levels in `dust` (-1: not built)
};

void updateDust(Cache& c, const Form& f, const Dragon& d);

// The den room (WP6, romfs:/models/den.esm). Each attribute array has its own linear buffer,
// so any two lighting sets can feed the static shader's colours A and B. The index list is
// split into runs of parts that draw together (all the opaque room in one call; the glows).
struct Room {
    struct Run {
        u16* idx = nullptr;
        int count = 0;
        u8 flags = 0;
    };
    static constexpr int kMaxRuns = 8;
    StaticScene scene;
    float* pos = nullptr;
    u8* color[kLightSets] = {};
    Run runs[kMaxRuns];
    int runCount = 0;
    bool ok = false;

    void release() {
        if (pos) linearFree(pos);
        pos = nullptr;
        for (u8*& c : color) {
            if (c) linearFree(c);
            c = nullptr;
        }
        for (Run& r : runs) {
            if (r.idx) linearFree(r.idx);
            r = Run{};
        }
        runCount = 0;
        ok = false;
    }
};

// The egg (romfs:/models/egg.ecm): one skinned shell mesh with two bones (core/egg).
struct EggForm {
    ModelData model;
    GpuMesh shell;
    bool ok = false;
};

constexpr float kNestFloor = 0.07f;  // eggs sit on the egg nest's straw

DVLB_s* g_dvlb = nullptr;
shaderProgram_s g_program;
int g_locProjection = -1, g_locModelView = -1, g_locBones = -1, g_locPalette = -1;
C3D_AttrInfo g_attr;       // the body: v5 (dust) from each dragon's buffer
C3D_AttrInfo g_attrFixed;  // everything else: v5 fixed (the wings' dust, or none)
int g_dustFixed = -1;      // that fixed attribute's index
C3D_Tex g_dustRamp;        // 256x8 L8: texel i = i, so the dust stream (as u) reads back as a factor
C3D_Tex g_cleanSkin;       // 8x8 white: stands in when a form's skin texture is missing
bool g_texOk = false;
DVLB_s* g_staticDvlb = nullptr;
shaderProgram_s g_staticProgram;
int g_locSProjection = -1, g_locSModelView = -1, g_locSBlend = -1, g_locSTint = -1;
C3D_AttrInfo g_staticAttr;
C3D_LightEnv g_lightEnv;
C3D_Light g_light;
C3D_LightLut g_lutToon, g_lutRim;
constexpr int kCacheSlots = 4;

// Lighting (architecture section 4): primary = plum-tinted ambient, secondary = toon ramp on
// L.N (specular 0 through LUT D0), secondary alpha = rim (Fresnel LUT on N.V). The ambient
// and the key colour follow the time of day (core/daylight); these are the midday values.
constexpr C3D_Material kMaterial = {
    {0.42f, 0.36f, 0.44f},  // ambient
    {0.0f, 0.0f, 0.0f},     // diffuse (the toon LUT replaces it)
    {1.0f, 1.0f, 1.0f},     // specular0
    {0.0f, 0.0f, 0.0f},     // specular1
    {0.0f, 0.0f, 0.0f},     // emission
};

// One dragon posed for this frame (static storage: the matrices are ~3.5 KB).
struct Posed {
    const Form* form = nullptr;
    const Cache* cache = nullptr;
    const Dragon* dragon = nullptr;
    float size = 1;             // genome size scale
    float scale = 1;            // size relative to an adult (root motion)
    Vec2 pos;                   // den floor position (adult units)
    float heading = 0;          // 0 faces -Y
    float root[2] = {0, 0};     // clip root offset (forward, up), adult units
    float ground = 0;           // floor contact, armature space
    Mat34 poseMat[kMaxBones], skin[kMaxBones];
};

Form g_forms[kFormCount][2];  // [form][lod]: LOD1 draws background dragons in a full den
Cache g_caches[kCacheSlots];
u32 g_frame = 0;
Posed g_posed;
float g_adultRadius = 1;  // framing radius of a neutral adult: the camera's reference size
PartsMesh g_parts;
bool g_ready = false;
AnimLibrary g_anims;
int g_clipIndex[kFormCount][static_cast<int>(ClipId::Count)];
bool g_animsOk = false;
AnimBinding g_bind[kFormCount];  // LOD1 shares its form's skeleton
Room g_room;
EggForm g_egg;
Vec3 g_camTarget;                // smoothed den camera target
float g_camRadius = 0;           // smoothed den framing radius (0: not set yet)
Vec3 g_camEye;                   // last den camera position: where "the player" is
C3D_Mtx g_denView;               // last den camera, for projecting particles and the egg
bool g_denViewSet = false;
Vec3 g_heads[3];                 // den dragons' heads in the last drawDen
bool g_headSet[3] = {};

// Toon ramp on L.N (signed): plum shadow, a mid band, full light.
float toonRamp(float x, float) { return x < 0.12f ? 0.0f : (x < 0.45f ? 0.62f : 1.0f); }
// Rim on N.V: a thin bright band on the silhouette.
float rimBand(float x, float) { return x < 0.28f ? 1.0f : (x < 0.38f ? 0.35f : 0.0f); }

bool loadTexture(const char* path, C3D_Tex& tex) {
    FILE* file = std::fopen(path, "rb");
    if (!file) return false;
    Tex3DS_Texture t3x = Tex3DS_TextureImportStdio(file, &tex, nullptr, false);
    std::fclose(file);
    if (!t3x) return false;
    Tex3DS_TextureFree(t3x);
    C3D_TexSetFilter(&tex, GPU_LINEAR, GPU_LINEAR);
    C3D_TexSetFilterMipmap(&tex, GPU_LINEAR);
    C3D_TexSetWrap(&tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    return true;
}

// Texel (x, y) of an 8x8-tiled L8 texture (the GPU's Morton order inside each tile).
std::size_t tiledIndex(int x, int y, int width) {
    const int tile = (y / 8) * (width / 8) + x / 8;
    int m = 0;
    for (int b = 0; b < 3; ++b) m |= (((x >> b) & 1) << (2 * b)) | (((y >> b) & 1) << (2 * b + 1));
    return std::size_t(tile) * 64 + m;
}

// The dust ramp and the clean stand-in skin (rgba8: no pattern in R, G, B; detail 1 in A).
bool makeTextures() {
    if (!C3D_TexInit(&g_dustRamp, 256, 8, GPU_L8) || !C3D_TexInit(&g_cleanSkin, 8, 8, GPU_RGBA8)) return false;
    u8* ramp = static_cast<u8*>(g_dustRamp.data);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 256; ++x) ramp[tiledIndex(x, y, 256)] = static_cast<u8>(x);
    C3D_TexFlush(&g_dustRamp);
    C3D_TexSetFilter(&g_dustRamp, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&g_dustRamp, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    u32* clean = static_cast<u32*>(g_cleanSkin.data);
    for (int i = 0; i < 8 * 8; ++i) clean[i] = 0x000000FFu;  // GPU_RGBA8 texels are 0xRRGGBBAA
    C3D_TexFlush(&g_cleanSkin);
    return true;
}

bool loadForm(const char* path, const char* skinPath, Form& f) {
    std::vector<u8> bytes;
    if (!readFile(path, bytes) || !loadModel(bytes.data(), bytes.size(), f.model)) return false;
    const MeshData* body = f.model.findMesh(kMeshBody, kGroupBody, 0);
    if (!body || !fillStatic(f.body, *body)) return false;
    f.bodyData = body;
    f.skinOk = loadTexture(skinPath, f.skin);  // without it the dragons are plain, not broken
    for (const MeshData& m : f.model.meshes)
        if (m.kind == kMeshWings && m.variant < kWingsCount && !fillStatic(f.wings[m.variant], m)) return false;
    f.headBone = f.model.skel.find("head");
    f.eyesBone = f.model.skel.find("eyes");
    f.chestBone = f.model.skel.find("chest");
    f.ok = true;
    return true;
}

bool loadEgg(const char* path) {
    std::vector<u8> bytes;
    if (!readFile(path, bytes) || !loadModel(bytes.data(), bytes.size(), g_egg.model)) return false;
    const MeshData* shell = g_egg.model.findMesh(kMeshBody, kGroupBody, 0);
    g_egg.ok = shell && g_egg.model.skel.count == 2 && fillStatic(g_egg.shell, *shell);
    return g_egg.ok;
}

bool loadRoom(const char* path) {
    std::vector<u8> bytes;
    Room& r = g_room;
    if (!readFile(path, bytes) || !loadStaticScene(bytes.data(), bytes.size(), r.scene) || r.scene.sets != kLightSets)
        return false;
    const StaticScene& s = r.scene;
    r.pos = static_cast<float*>(linearAlloc(sizeof(float) * 3 * s.vertexCount));
    bool ok = r.pos != nullptr;
    for (int k = 0; k < kLightSets && ok; ++k) {
        r.color[k] = static_cast<u8*>(linearAlloc(std::size_t(s.vertexCount) * 4));
        ok = r.color[k] != nullptr;
        if (ok) {
            std::memcpy(r.color[k], s.colors(k), std::size_t(s.vertexCount) * 4);
            GSPGPU_FlushDataCache(r.color[k], std::size_t(s.vertexCount) * 4);
        }
    }
    // Runs of consecutive parts with the same flags, each with its own (aligned) index buffer.
    for (std::size_t i = 0; i < s.parts.size() && ok;) {
        std::size_t j = i + 1;
        while (j < s.parts.size() && s.parts[j].flags == s.parts[i].flags) ++j;
        if (r.runCount == Room::kMaxRuns) {
            ok = false;
            break;
        }
        Room::Run& run = r.runs[r.runCount++];
        const int first = s.parts[i].firstIndex;
        run.count = s.parts[j - 1].firstIndex + s.parts[j - 1].indexCount - first;
        run.flags = s.parts[i].flags;
        run.idx = static_cast<u16*>(linearAlloc(sizeof(u16) * run.count));
        ok = run.idx != nullptr;
        if (ok) {
            std::memcpy(run.idx, s.indices.data() + first, sizeof(u16) * run.count);
            GSPGPU_FlushDataCache(run.idx, sizeof(u16) * run.count);
        }
        i = j;
    }
    if (!ok) {
        r.release();
        return false;
    }
    for (int v = 0; v < s.vertexCount; ++v) {
        r.pos[v * 3] = s.pos[v].x;
        r.pos[v * 3 + 1] = s.pos[v].y;
        r.pos[v * 3 + 2] = s.pos[v].z;
    }
    GSPGPU_FlushDataCache(r.pos, sizeof(float) * 3 * s.vertexCount);
    r.ok = true;
    return true;
}

// Idle-pose framing: joint bounding box from the ground up (wings reach past their last
// joints, hence the margin). Optionally returns the box centre and the ground offset.
float framingRadius(const ModelData& m, float t, int build, Vec3* center, float* ground) {
    BonePose pose[kMaxBones];
    Mat34 poseMat[kMaxBones], skin[kMaxBones];
    idlePose(m, t, build, pose);
    evaluatePose(m.skel, pose, poseMat, skin);
    Vec3 lo{1e9f, 1e9f, 1e9f}, hi{-1e9f, -1e9f, -1e9f};
    for (int i = 0; i < m.skel.count; ++i) {
        const Vec3 p = poseMat[i].translation();
        lo = {std::fmin(lo.x, p.x), std::fmin(lo.y, p.y), std::fmin(lo.z, p.z)};
        hi = {std::fmax(hi.x, p.x), std::fmax(hi.y, p.y), std::fmax(hi.z, p.z)};
    }
    lo.z = groundOffset(m, skin);
    if (center) *center = (lo + hi) * 0.5f;
    if (ground) *ground = lo.z;
    return length(hi - lo) * 0.5f * 1.25f;
}

// Rebuilds the merged part mesh, ground offset and framing when the dragon, its growth or
// its genome changes (growth is slow: days, not frames).
void refreshCache(Cache& c, const Dragon& d, const Growth& gr, int build, int lod) {
    const bool same = c.valid && c.id == d.id && c.form == gr.form && c.lod == lod && std::fabs(c.t - gr.t) < 0.002f &&
                      c.sex == d.sex && std::memcmp(&c.genome, &d.genome, sizeof(Genome)) == 0;
    if (same) return;
    const Form& f = g_forms[gr.form][lod];
    c.valid = false;
    if (!buildParts(f.model, d.genome, d.sex, gr.t, g_parts)) return;
    if (!fill(c.parts, static_cast<int>(g_parts.pos.size()), g_parts.pos.data(), g_parts.nrm.data(),
              g_parts.skin.data(), g_parts.paint.data(), g_parts.uv.data(), g_parts.indices.data(),
              static_cast<int>(g_parts.indices.size()), g_parts.palette, g_parts.paletteCount))
        return;

    c.radius = framingRadius(f.model, gr.t, build, &c.center, &c.ground);

    c.id = d.id;
    c.form = gr.form;
    c.lod = lod;
    c.t = gr.t;
    c.genome = d.genome;
    c.sex = d.sex;
    c.valid = true;
}

int buildOf(const Dragon& d) { return d.genome.build < kModelBuilds ? d.genome.build : kBuildNeutral; }

// The dragon's cache slot, refreshed if its growth or genome changed. nullptr if the
// dragon has no model yet (an egg) or its parts could not be built.
Cache* cacheFor(const Dragon& d, s64 now, int lod) {
    if (d.stage == Stage::Egg) return nullptr;
    Cache* slot = nullptr;
    for (Cache& c : g_caches)
        if (c.valid && c.id == d.id) slot = &c;
    if (!slot) {
        slot = &g_caches[0];
        for (Cache& c : g_caches) {
            if (!c.valid) {
                slot = &c;
                break;
            }
            if (c.lastUsed < slot->lastUsed) slot = &c;
        }
    }
    refreshCache(*slot, d, growthFor(d.stage, stageProgress(d, now)), buildOf(d), lod);
    slot->lastUsed = g_frame;
    return slot->valid ? slot : nullptr;
}

// Blends a dragon's own framing radius with the adult's, so babies read as small and adults
// as big while a hatchling still fills a good part of the screen.
float viewRadius(const Cache& c, float size) { return (0.8f * c.radius + 0.2f * g_adultRadius) * size; }

// Lowest point of the animated body, so the feet (or belly, or back) rest on the floor.
float animatedGround(const ModelData& m, const Mat34* skin) {
    const MeshData* body = m.findMesh(kMeshBody, kGroupBody, 0);
    if (!body) return 0;
    float low = 1e9f;
    for (int v = 0; v < body->vertexCount; v += 3) {
        const Vec3 p = skinPoint(*body, v, body->pos[v], skin);
        if (p.z < low) low = p.z;
    }
    return low;
}

// Poses a dragon: idle pose + its actor's animation, placed where its behavior stands.
bool pose(App& app, const Dragon& d, const DenActor* actor, s64 now, int lod, Posed& out) {
    Cache* c = cacheFor(d, now, lod);
    if (!c) return false;
    const Form& f = g_forms[c->form][lod];
    updateDust(*c, f, d);
    out.size = sizeScale(d.genome);
    out.scale = growthScale(growthFor(d.stage, stageProgress(d, now))) * out.size;
    out.pos = actor ? actor->behavior.pos : Vec2{};
    out.heading = actor ? actor->behavior.heading : 0.0f;
    BonePose bones[kMaxBones];
    idlePose(f.model, c->t, buildOf(d), bones);
    out.root[0] = out.root[1] = 0;
    if (actor && g_animsOk) {
        Quat delta[kMaxBones];
        actor->anim.sample(g_anims, g_bind[c->form], f.model.skel.count, delta, out.root);
        applyDeltas(bones, delta, f.model.skel.count);
        // Look at the player: the den camera, brought into the dragon's armature space
        // (the inverse of modelMatrix, with last frame's floor contact).
        if (actor->look > 0.01f && g_camRadius > 0) {
            const float ch = std::cos(out.heading), sh = std::sin(out.heading);
            const Vec3 rel{g_camEye.x - out.pos.x, g_camEye.y - out.pos.y, g_camEye.z - out.root[1] * out.scale};
            Vec3 local{rel.x * ch + rel.y * sh, -rel.x * sh + rel.y * ch, rel.z};
            local.y += out.root[0] * out.scale;
            local = local * (1.0f / out.size);
            local.z += c->groundNow;
            applyLookAt(f.model.skel, g_bind[c->form], bones, local, actor->look);
        }
    }
    if (actor && f.eyesBone >= 0) bones[f.eyesBone].scale.z *= 1.0f - kBlinkSquash * actor->eyes.shut;  // blinks
    evaluatePose(f.model.skel, bones, out.poseMat, out.skin);
    // Floor contact follows the pose (sitting, lying, rolling over), smoothed so a swinging
    // foot does not make the body bob.
    const float low = animatedGround(f.model, out.skin);
    if (!c->groundSet) {
        c->groundNow = low;
        c->groundSet = true;
    } else {
        const float k = std::fmin(1.0f, app.dt * 14.0f);
        c->groundNow += (low - c->groundNow) * k;
    }
    out.form = &f;
    out.cache = c;
    out.dragon = &d;
    out.ground = c->groundNow;
    return true;
}

// Model matrix: at its den position, turned to its heading, lifted/leaping by the clip's root
// offset, genome size, feet on the floor.
void modelMatrix(const Posed& p, C3D_Mtx& out) {
    Mtx_Identity(&out);
    Mtx_Translate(&out, p.pos.x, p.pos.y, p.root[1] * p.scale, true);
    Mtx_RotateZ(&out, p.heading, true);
    Mtx_Translate(&out, 0, -p.root[0] * p.scale, 0, true);  // forward is -Y
    Mtx_Scale(&out, p.size, p.size, p.size);
    Mtx_Translate(&out, 0, 0, -p.ground, true);
}

Vec3 apply(const C3D_Mtx& m, Vec3 v) {
    return {m.r[0].x * v.x + m.r[0].y * v.y + m.r[0].z * v.z + m.r[0].w,
            m.r[1].x * v.x + m.r[1].y * v.y + m.r[1].z * v.z + m.r[1].w,
            m.r[2].x * v.x + m.r[2].y * v.y + m.r[2].z * v.z + m.r[2].w};
}

void dragonPattern(u8 pattern, Rgb color);

// The dragon colour chain (architecture §4), texture 0 = the form's skin (R stripes, G
// spots, B dapple, A scale detail), texture 1 = the dust ramp read at the dust stream:
//   0: albedo = vertex colour, patterned (dragonPattern sets it per dragon)
//   1: dusted: toward the dust colour by the dust level
//   2: x scale detail
//   3: lit: x (ambient + toon), alpha = rim (Fresnel)
//   4: + vertex colour x emissive (vertex alpha): heartglow, eye glints
//   5: + warm rim colour (per dragon, lightDragon) x rim; opaque
void setupTexEnv() {
    C3D_TexEnv* env = C3D_GetTexEnv(1);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, GPU_PREVIOUS, GPU_TEXTURE1);
    C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_R);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_INTERPOLATE);
    C3D_TexEnvColor(env, kDirtColor);
    env = C3D_GetTexEnv(2);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_PREVIOUS, GPU_TEXTURE0, GPU_PRIMARY_COLOR);
    C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_ALPHA, GPU_TEVOP_RGB_SRC_COLOR);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_MODULATE);
    env = C3D_GetTexEnv(3);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_FRAGMENT_PRIMARY_COLOR, GPU_FRAGMENT_SECONDARY_COLOR, GPU_PREVIOUS);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_ADD_MULTIPLY);
    C3D_TexEnvSrc(env, C3D_Alpha, GPU_FRAGMENT_SECONDARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
    env = C3D_GetTexEnv(4);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PREVIOUS);
    C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_ALPHA, GPU_TEVOP_RGB_SRC_COLOR);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_MULTIPLY_ADD);
    C3D_TexEnvSrc(env, C3D_Alpha, GPU_PREVIOUS, GPU_PREVIOUS, GPU_PREVIOUS);
    C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
    env = C3D_GetTexEnv(5);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, GPU_PREVIOUS, GPU_PREVIOUS);
    C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_ALPHA, GPU_TEVOP_RGB_SRC_COLOR);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_MULTIPLY_ADD);
    C3D_TexEnvSrc(env, C3D_Alpha, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT);
    C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
    C3D_TexEnvColor(env, 0xFF28405A);  // ABGR: warm rim (90, 64, 40), alpha 255
    dragonPattern(kPatternSolid, {0, 0, 0});
}

// Stage 0 for one dragon: its Pattern gene picks a skin channel and blends the pattern
// colour in by it (Runes, until Alpha 2 draws them, and Solid show no pattern).
void dragonPattern(u8 pattern, Rgb color) {
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    GPU_TEVOP_RGB channel;
    switch (pattern) {
        case kPatternStripes: channel = GPU_TEVOP_RGB_SRC_R; break;
        case kPatternSpots: channel = GPU_TEVOP_RGB_SRC_G; break;
        case kPatternDapple: channel = GPU_TEVOP_RGB_SRC_B; break;
        default:
            C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
            return;
    }
    C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, GPU_PRIMARY_COLOR, GPU_TEXTURE0);
    C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, channel);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_INTERPOLATE);
    C3D_TexEnvSrc(env, C3D_Alpha, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
    C3D_TexEnvColor(env, 0xFF000000u | (u32(color.b) << 16) | (u32(color.g) << 8) | color.r);
}

// Binds a form's skin (or the clean stand-in) and the dust ramp.
void bindSkin(const Form* f) {
    C3D_TexBind(0, f && f->skinOk ? const_cast<C3D_Tex*>(&f->skin) : &g_cleanSkin);
    C3D_TexBind(1, &g_dustRamp);
}

u8 toByte(float v) { return static_cast<u8>(v <= 0 ? 0 : (v >= 1 ? 255 : v * 255.0f + 0.5f)); }

// The light on one dragon: the time of day, scaled per channel by `local` (the room's light
// where it stands, relative to the rug's).
void lightDragon(const DragonLight& light, const float local[3]) {
    C3D_Material m = kMaterial;
    for (int k = 0; k < 3; ++k) m.ambient[k] = light.ambient[k] * local[k];
    C3D_LightEnvMaterial(&g_lightEnv, &m);
    C3D_LightSpecular0(&g_light, light.key[0] * local[0], light.key[1] * local[1], light.key[2] * local[2]);
    const u32 rim = 0xFF000000u | (u32(toByte(light.rim[2])) << 16) | (u32(toByte(light.rim[1])) << 8) |
                    toByte(light.rim[0]);  // ABGR
    C3D_TexEnvColor(C3D_GetTexEnv(5), rim);
}

// The room's floor light near `at` for the time of day (0..1 per channel).
bool floorLight(Vec2 at, const DayBlend& b, float out[3]) {
    float a[3], c[3];
    if (!g_room.ok || !g_room.scene.lightNear("floor", at, 2.2f, b.a, a) ||
        !g_room.scene.lightNear("floor", at, 2.2f, b.b, c))
        return false;
    for (int k = 0; k < 3; ++k) out[k] = a[k] + (c[k] - a[k]) * b.t;
    return true;
}

// A dragon picks up the room's light: darker in the sulk nook, warmer by the hearth,
// brighter in the sunbeam. Relative to the rug, where the dragons were designed to look right.
void localLight(Vec2 at, const DayBlend& b, float out[3]) {
    out[0] = out[1] = out[2] = 1.0f;
    const DenLayout den;
    float here[3], home[3];
    if (!floorLight(at, b, here) || !floorLight(den.home, b, home)) return;
    for (int k = 0; k < 3; ++k) {
        const float r = home[k] > 0.02f ? here[k] / home[k] : 1.0f;
        out[k] = r < 0.55f ? 0.55f : (r > 1.35f ? 1.35f : r);
    }
}

// A dragon's dust level for one region, in the dust stream's units (0..255 = none..kDirtMax).
float dustValue(const Dragon& d, int region) {
    return region < kRegionCount ? d.dirt[region] * (255.0f / 100.0f) * kDirtMax : 0.0f;
}

// Rebuilds a dragon's per-vertex dust stream when its dirt has visibly changed.
void updateDust(Cache& c, const Form& f, const Dragon& d) {
    const MeshData* body = f.bodyData;
    if (!body) return;
    if (c.dustCount != body->vertexCount) {
        if (c.dust) linearFree(c.dust);
        c.dust = static_cast<u8*>(linearAlloc(std::size_t(body->vertexCount) * 4));
        c.dustCount = c.dust ? body->vertexCount : 0;
        for (float& s : c.dustShown) s = -1.0f;
    }
    if (!c.dust) return;
    bool changed = false;
    for (int r = 0; r < kRegionCount; ++r) changed |= std::fabs(d.dirt[r] - c.dustShown[r]) > 0.5f;
    if (!changed) return;
    for (int r = 0; r < kRegionCount; ++r) c.dustShown[r] = d.dirt[r];
    for (int v = 0; v < body->vertexCount; ++v) {
        u8* o = c.dust + std::size_t(v) * 4;
        o[0] = static_cast<u8>(dustValue(d, body->region[v]) + 0.5f);
        o[1] = o[2] = o[3] = 0;
    }
    GSPGPU_FlushDataCache(c.dust, std::size_t(c.dustCount) * 4);
}

void uploadBones(const GpuMesh& g, const Mat34* skin) {
    C3D_FVec* rows = C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, g_locBones, g.paletteCount * 3);
    for (int i = 0; i < g.paletteCount; ++i) {
        const Mat34& m = skin[g.palette[i]];
        for (int r = 0; r < 3; ++r) rows[i * 3 + r] = FVec4_New(m.m[r][0], m.m[r][1], m.m[r][2], m.m[r][3]);
    }
}

// Draws a mesh whose dust level is one value for every vertex (wings, parts, the egg).
void drawMesh(App& app, const GpuMesh& g, const Mat34* skin, float dust = 0.0f) {
    if (!g.vbo || g.indexCount == 0) return;
    uploadBones(g, skin);
    C3D_SetAttrInfo(&g_attrFixed);
    C3D_FixedAttribSet(g_dustFixed, dust, 0, 0, 0);
    C3D_BufInfo* buf = C3D_GetBufInfo();
    BufInfo_Init(buf);
    BufInfo_Add(buf, g.vbo, sizeof(GpuVertex), 5, 0x43210);
    C3D_DrawElements(GPU_TRIANGLES, g.indexCount, C3D_UNSIGNED_SHORT, g.ibo);
    app.stats.tris += g.indexCount / 3;
    app.stats.draws += 1;
    if (g.paletteCount > app.stats.maxBonesPerDraw) app.stats.maxBonesPerDraw = g.paletteCount;
}

// Draws a dragon's body with its own dust stream (per-vertex, by region).
void drawBody(App& app, const GpuMesh& g, const Mat34* skin, const Cache& c) {
    if (!g.vbo || g.indexCount == 0) return;
    if (!c.dust || c.dustCount != g.vertexCount) {  // no stream: clean
        drawMesh(app, g, skin, 0.0f);
        return;
    }
    uploadBones(g, skin);
    C3D_SetAttrInfo(&g_attr);
    C3D_BufInfo* buf = C3D_GetBufInfo();
    BufInfo_Init(buf);
    BufInfo_Add(buf, g.vbo, sizeof(GpuVertex), 5, 0x43210);
    BufInfo_Add(buf, c.dust, 4, 1, 0x5);
    C3D_DrawElements(GPU_TRIANGLES, g.indexCount, C3D_UNSIGNED_SHORT, g.ibo);
    app.stats.tris += g.indexCount / 3;
    app.stats.draws += 1;
    if (g.paletteCount > app.stats.maxBonesPerDraw) app.stats.maxBonesPerDraw = g.paletteCount;
}

}  // namespace

bool init() {
    g_dvlb = DVLB_ParseFile(reinterpret_cast<u32*>(const_cast<u8*>(dragon_shbin)), dragon_shbin_size);
    shaderProgramInit(&g_program);
    shaderProgramSetVsh(&g_program, &g_dvlb->DVLE[0]);
    g_locProjection = shaderInstanceGetUniformLocation(g_program.vertexShader, "projection");
    g_locModelView = shaderInstanceGetUniformLocation(g_program.vertexShader, "modelView");
    g_locBones = shaderInstanceGetUniformLocation(g_program.vertexShader, "bones");
    g_locPalette = shaderInstanceGetUniformLocation(g_program.vertexShader, "palette");

    for (C3D_AttrInfo* a : {&g_attr, &g_attrFixed}) {
        AttrInfo_Init(a);
        AttrInfo_AddLoader(a, 0, GPU_FLOAT, 3);          // position
        AttrInfo_AddLoader(a, 1, GPU_FLOAT, 3);          // normal
        AttrInfo_AddLoader(a, 2, GPU_UNSIGNED_BYTE, 4);  // skin
        AttrInfo_AddLoader(a, 3, GPU_UNSIGNED_BYTE, 4);  // paint
        AttrInfo_AddLoader(a, 4, GPU_FLOAT, 2);          // skin texture UV
    }
    AttrInfo_AddLoader(&g_attr, 5, GPU_UNSIGNED_BYTE, 4);  // dust level (the dragon's own buffer)
    g_dustFixed = AttrInfo_AddFixed(&g_attrFixed, 5);
    g_texOk = makeTextures();

    g_staticDvlb = DVLB_ParseFile(reinterpret_cast<u32*>(const_cast<u8*>(static_shbin)), static_shbin_size);
    shaderProgramInit(&g_staticProgram);
    shaderProgramSetVsh(&g_staticProgram, &g_staticDvlb->DVLE[0]);
    g_locSProjection = shaderInstanceGetUniformLocation(g_staticProgram.vertexShader, "projection");
    g_locSModelView = shaderInstanceGetUniformLocation(g_staticProgram.vertexShader, "modelView");
    g_locSBlend = shaderInstanceGetUniformLocation(g_staticProgram.vertexShader, "blend");
    g_locSTint = shaderInstanceGetUniformLocation(g_staticProgram.vertexShader, "tint");
    AttrInfo_Init(&g_staticAttr);
    AttrInfo_AddLoader(&g_staticAttr, 0, GPU_FLOAT, 3);          // position
    AttrInfo_AddLoader(&g_staticAttr, 1, GPU_UNSIGNED_BYTE, 4);  // colour, lighting set A
    AttrInfo_AddLoader(&g_staticAttr, 2, GPU_UNSIGNED_BYTE, 4);  // colour, lighting set B

    C3D_LightEnvInit(&g_lightEnv);
    C3D_LightEnvMaterial(&g_lightEnv, &kMaterial);
    LightLut_FromFunc(&g_lutToon, toonRamp, 0.0f, true);
    C3D_LightEnvLut(&g_lightEnv, GPU_LUT_D0, GPU_LUTINPUT_LN, true, &g_lutToon);
    LightLut_FromFunc(&g_lutRim, rimBand, 0.0f, false);
    C3D_LightEnvLut(&g_lightEnv, GPU_LUT_FR, GPU_LUTINPUT_NV, false, &g_lutRim);
    C3D_LightEnvFresnel(&g_lightEnv, GPU_SEC_ALPHA_FRESNEL);
    C3D_LightInit(&g_light, &g_lightEnv);
    C3D_LightDiffuse(&g_light, 0, 0, 0);
    C3D_LightSpecular0(&g_light, 0.57f, 0.62f, 0.53f);  // lit = ambient + this: warm white
    C3D_LightSpecular1(&g_light, 0, 0, 0);
    C3D_FVec lightDir = FVec4_New(-0.45f, 0.8f, 0.4f, 0.0f);  // view space, directional (w = 0)
    C3D_LightPosition(&g_light, &lightDir);

    g_ready = g_texOk &&
              loadForm("romfs:/models/hatchling.ecm", "romfs:/models/hatchling_skin.t3x",
                       g_forms[kFormHatchling][0]) &&
              loadForm("romfs:/models/hatchling_lod1.ecm", "romfs:/models/hatchling_lod1_skin.t3x",
                       g_forms[kFormHatchling][1]) &&
              loadForm("romfs:/models/grown.ecm", "romfs:/models/grown_skin.t3x", g_forms[kFormGrown][0]) &&
              loadForm("romfs:/models/grown_lod1.ecm", "romfs:/models/grown_lod1_skin.t3x", g_forms[kFormGrown][1]);
    if (g_ready) g_adultRadius = framingRadius(g_forms[kFormGrown][0].model, 1.0f, kBuildNeutral, nullptr, nullptr);
    if (g_ready) {
        std::vector<u8> bytes;
        g_animsOk = readFile("romfs:/anims/dragon.eca", bytes) && loadAnims(bytes.data(), bytes.size(), g_anims) &&
                    resolveClips(g_anims, kFormHatchling, g_clipIndex[kFormHatchling]) &&
                    resolveClips(g_anims, kFormGrown, g_clipIndex[kFormGrown]);
        for (int f = 0; f < kFormCount; ++f) bindAnims(g_anims, g_forms[f][0].model.skel, g_bind[f]);
        loadRoom("romfs:/models/den.esm");  // without it, dragons stand on the 2D backdrop
        loadEgg("romfs:/models/egg.ecm");   // without it, eggs stay 2D
    }
    return g_ready;
}

void shutdown() {
    for (auto& lods : g_forms)
        for (Form& f : lods) {
            f.body.release();
            for (GpuMesh& w : f.wings) w.release();
            if (f.skinOk) C3D_TexDelete(&f.skin);
            f.skinOk = false;
        }
    for (Cache& c : g_caches) {
        c.parts.release();
        if (c.dust) linearFree(c.dust);
        c.dust = nullptr;
        c.dustCount = 0;
        c.valid = false;
    }
    if (g_texOk) {
        C3D_TexDelete(&g_dustRamp);
        C3D_TexDelete(&g_cleanSkin);
        g_texOk = false;
    }
    g_room.release();
    g_egg.shell.release();
    g_egg.ok = false;
    if (g_staticDvlb) {
        shaderProgramFree(&g_staticProgram);
        DVLB_Free(g_staticDvlb);
        g_staticDvlb = nullptr;
    }
    if (g_dvlb) {
        shaderProgramFree(&g_program);
        DVLB_Free(g_dvlb);
        g_dvlb = nullptr;
    }
    g_ready = false;
}

bool ready() { return g_ready; }

void prepare2D() { C3D_DepthTest(true, GPU_ALWAYS, GPU_WRITE_COLOR); }

namespace {

// Switches the GPU from citro2d to the dragon pipeline (everything 2D so far draws first).
void bindDragons(const C3D_Mtx& projection) {
    C3D_BindProgram(&g_program);
    C3D_SetAttrInfo(&g_attr);
    C3D_LightEnvBind(&g_lightEnv);
    setupTexEnv();
    C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
    C3D_CullFace(GPU_CULL_BACK_CCW);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_locProjection, &projection);
}

// Hands the GPU back to citro2d.
void end3D() {
    C3D_LightEnvBind(nullptr);
    C3D_TexBind(1, nullptr);
    for (int i = 0; i < 6; ++i) C3D_TexEnvInit(C3D_GetTexEnv(i));
    C2D_Prepare();
    prepare2D();
}

// The hearth's flicker: two uneven waves.
float flicker(float t) { return 0.84f + 0.1f * std::sin(t * 13.0f) + 0.06f * std::sin(t * 31.0f + 1.0f); }

// The den room with the static program: its opaque parts (one draw) or, after the dragons,
// its additive glows (sunbeam, flames), lit by the two lighting sets of the time of day.
void drawRoom(App& app, const C3D_Mtx& projection, const C3D_Mtx& view, const DayBlend& blend, bool glows) {
    if (!g_room.ok) return;
    C3D_BindProgram(&g_staticProgram);
    C3D_SetAttrInfo(&g_staticAttr);
    C3D_LightEnvBind(nullptr);
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    for (int i = 1; i < 6; ++i) C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_locSProjection, &projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_locSModelView, &view);  // the room is modelled in den space
    C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locSBlend, blend.t, 0, 0, 0);
    C3D_BufInfo* buf = C3D_GetBufInfo();
    BufInfo_Init(buf);
    BufInfo_Add(buf, g_room.pos, sizeof(float) * 3, 1, 0x0);
    BufInfo_Add(buf, g_room.color[blend.a], 4, 1, 0x1);
    BufInfo_Add(buf, g_room.color[blend.b], 4, 1, 0x2);
    if (glows) {
        C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_COLOR);  // behind the dragons, never hiding them
        C3D_CullFace(GPU_CULL_NONE);
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ONE, GPU_ZERO, GPU_ONE);
    } else {
        C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
        C3D_CullFace(GPU_CULL_BACK_CCW);  // the near wall faces away from the camera: a cutaway
    }
    for (int i = 0; i < g_room.runCount; ++i) {
        const Room::Run& run = g_room.runs[i];
        if (((run.flags & kStaticAdditive) != 0) != glows) continue;
        const float k = (run.flags & kStaticFlicker) ? flicker(app.t) : 1.0f;
        C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locSTint, k / 255.0f, k / 255.0f, k / 255.0f, 1.0f / 255.0f);
        C3D_DrawElements(GPU_TRIANGLES, run.count, C3D_UNSIGNED_SHORT, run.idx);
        app.stats.tris += run.count / 3;
        app.stats.draws += 1;
    }
    if (glows)  // back to ordinary alpha blending (citro2d, dragons)
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA,
                       GPU_ONE_MINUS_SRC_ALPHA);
}

void submit(App& app, const Posed& p, const C3D_Mtx& view, const C3D_Mtx& model) {
    C3D_Mtx modelView;
    Mtx_Multiply(&modelView, &view, &model);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_locModelView, &modelView);
    const Dragon& d = *p.dragon;
    Rgb pal[kPalCount];
    dragonPalette(d.genome, pal);
    const float glow = 0.55f + 0.45f * heartglowLevel(d, app.t);  // the heartglow pulses with mood
    pal[kPalGlow] = {static_cast<u8>(pal[kPalGlow].r * glow), static_cast<u8>(pal[kPalGlow].g * glow),
                     static_cast<u8>(pal[kPalGlow].b * glow)};
    for (int i = 0; i < kPalCount; ++i)
        C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locPalette + i, pal[i].r / 255.0f, pal[i].g / 255.0f, pal[i].b / 255.0f,
                      1.0f);
    bindSkin(p.form);
    dragonPattern(d.genome.pattern, pal[kPalPattern]);
    drawBody(app, p.form->body, p.skin, *p.cache);
    drawMesh(app, p.cache->parts, p.skin);
    if (const MeshData* wings = selectWings(p.form->model, d.genome))
        drawMesh(app, p.form->wings[wings->variant], p.skin, dustValue(d, kRegionWings));
}

// An egg: its palette with each slot's glow in the alpha, rocking and cap from its motion,
// resting on the floor (z = 0 in model space) at `at`.
void submitEgg(App& app, const Dragon& d, const EggMotion& motion, const C3D_Mtx& view, Vec3 at) {
    Mat34 skin[2];
    eggSkin(g_egg.model, motion, skin);
    C3D_Mtx model, modelView;
    Mtx_Identity(&model);
    Mtx_Translate(&model, at.x, at.y, at.z - groundOffset(g_egg.model, skin), true);
    Mtx_Multiply(&modelView, &view, &model);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_locModelView, &modelView);
    Rgb pal[kPalCount];
    float glow[kPalCount];
    eggPalette(d, 0.85f + 0.15f * std::sin(app.t * 2.2f), pal, glow);
    for (int i = 0; i < kPalCount; ++i)
        C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locPalette + i, pal[i].r / 255.0f, pal[i].g / 255.0f, pal[i].b / 255.0f,
                      glow[i]);
    bindSkin(nullptr);  // the shell sits on the clean corner: no pattern, no dust
    dragonPattern(kPatternSolid, {0, 0, 0});
    drawMesh(app, g_egg.shell, skin);
}

void lookAt(C3D_Mtx& view, Vec3 eye, Vec3 target) {
    Mtx_LookAt(&view, FVec3_New(eye.x, eye.y, eye.z), FVec3_New(target.x, target.y, target.z), FVec3_New(0, 0, 1),
               false);
}

// Screen position of a den-space point under a view (camera looks down -Z): pixels on the
// 400x240 top screen and pixels per den unit at that depth.
bool projectWith(const C3D_Mtx& view, Vec3 p, float& x, float& y, float& ppu) {
    const Vec3 v = apply(view, p);
    if (v.z > -0.2f) return false;  // behind the camera
    ppu = (kScreenH * 0.5f / std::tan(kFovY * 0.5f)) / -v.z;
    x = kTopW * 0.5f + v.x * ppu;
    y = kScreenH * 0.5f - v.y * ppu;
    return true;
}

// Particles of one layer: the ambient ones between the room and the dragons, the care
// effects over them.
void drawParticles(App& app, const Particles& fx, bool foreground) {
    static const u32 kHeart = theme::rgba(0xF2, 0x6D, 0x85), kZ = theme::rgba(0xE8, 0xEE, 0xFF),
                     kCrumb = theme::rgba(0x9A, 0x62, 0x34), kSpark = theme::rgba(0xFF, 0xF4, 0xC8),
                     kPuff = theme::rgba(0xD8, 0xC2, 0xA4), kMote = theme::rgba(0xFF, 0xEC, 0xB0),
                     kEmberHot = theme::rgba(0xFF, 0xB0, 0x40), kEmberCool = theme::rgba(0xE0, 0x40, 0x18);
    for (int i = 0; i < fx.count(); ++i) {
        const Particle& p = fx[i];
        if (Particles::foreground(p.kind) != foreground) continue;
        float x, y, ppu;
        if (!projectWith(g_denView, p.pos, x, y, ppu) || x < -20 || x > kTopW + 20 || y < -20 || y > kScreenH + 20)
            continue;
        const float a = p.alpha(), s = std::fmax(1.0f, p.sizeNow() * ppu);
        switch (p.kind) {
            case Fx::Ember: {
                const u32 c = (p.age / p.life < 0.5f) ? kEmberHot : kEmberCool;
                C2D_DrawCircleSolid(x, y, 0, s * 1.8f, withAlpha(c, a * 0.25f));
                C2D_DrawCircleSolid(x, y, 0, s * 0.7f, withAlpha(c, a));
                break;
            }
            case Fx::Mote: C2D_DrawCircleSolid(x, y, 0, s * 0.6f, withAlpha(kMote, a * 0.7f)); break;
            case Fx::Glint:
            case Fx::Sparkle: {
                const u32 c = withAlpha(kSpark, a);
                C2D_DrawRectSolid(x - s, y - s * 0.12f, 0, s * 2, s * 0.24f, c);
                C2D_DrawRectSolid(x - s * 0.12f, y - s, 0, s * 0.24f, s * 2, c);
                if (p.kind == Fx::Sparkle) C2D_DrawCircleSolid(x, y, 0, s * 0.45f, withAlpha(kSpark, a * 0.4f));
                break;
            }
            case Fx::Heart: heart(x, y, s, withAlpha(kHeart, a)); break;
            case Fx::Zzz: text(app, "z", x, y - s * 0.5f, s / 30.0f, withAlpha(kZ, a)); break;
            case Fx::Crumb: C2D_DrawRectSolid(x - s * 0.5f, y - s * 0.5f, 0, s, s, withAlpha(kCrumb, a)); break;
            case Fx::Puff: C2D_DrawCircleSolid(x, y, 0, s * 0.5f, withAlpha(kPuff, a * 0.35f)); break;
            case Fx::Count: break;
        }
        ++app.stats.particles;
    }
}

}  // namespace

void drawDen(App& app, const DenDragon* dragons, int count, s64 now, const Particles* fx) {
    if (!g_ready) return;
    ++g_frame;
    if (count > 3) count = 3;
    const DenLayout den;
    // Frame everyone: the centre of the dragons' positions, wide enough for the biggest one
    // and the spread between them. The camera follows smoothly as they wander. With no
    // dragon out yet (an egg), it looks at the egg nest.
    float maxRadius = 0, spread = 0;
    Vec2 mid{0, 0}, at[3];
    bool drawn[3] = {};
    int shown = 0;
    for (int i = 0; i < count; ++i) {
        const Dragon& d = *dragons[i].dragon;
        float r;
        if (d.stage == Stage::Egg) {  // eggs sit in the egg nest
            if (!g_egg.ok || !dragons[i].egg) continue;
            r = 1.2f;
            at[i] = den.eggNest;
        } else {
            const Cache* c = cacheFor(d, now, i == 0 ? 0 : 1);
            if (!c) continue;
            r = viewRadius(*c, sizeScale(d.genome));
            at[i] = dragons[i].actor ? dragons[i].actor->behavior.pos : Vec2{};
        }
        maxRadius = std::fmax(maxRadius, r);
        mid.x += at[i].x;
        mid.y += at[i].y;
        drawn[i] = true;
        ++shown;
    }
    if (shown) {
        mid.x /= shown;
        mid.y /= shown;
        for (int i = 0; i < count; ++i)
            if (drawn[i]) spread = std::fmax(spread, std::hypot(at[i].x - mid.x, at[i].y - mid.y));
    } else if (g_room.ok) {
        mid = den.eggNest;
        maxRadius = 1.5f;
    } else {
        return;
    }
    const float radius = maxRadius + spread * 0.9f;
    const Vec3 want{mid.x, mid.y, radius * 0.62f};  // feet land low on the screen
    const float k = g_camRadius > 0 ? std::fmin(1.0f, app.dt * 2.5f) : 1.0f;
    g_camTarget = lerp(g_camTarget, want, k);
    g_camRadius += (radius - g_camRadius) * k;

    const Vec3 dir = normalize(Vec3{-0.35f, -0.9f, 0.32f});  // tools/blender/den_model.py CAM_DIR
    const float dist = g_camRadius / std::tan(kFovY * 0.5f) * 0.95f;
    C3D_Mtx projection, view, model;
    Mtx_PerspTilt(&projection, kFovY, C3D_AspectRatioTop, 0.25f, std::fmax(dist * 4.0f, 70.0f), false);
    g_camEye = g_camTarget + dir * dist;
    lookAt(view, g_camEye, g_camTarget);
    g_denView = view;
    g_denViewSet = true;
    const DayBlend blend = dayBlend(now);
    const DragonLight light = dragonLight(blend);

    C2D_Flush();
    drawRoom(app, projection, view, blend, false);
    if (fx) {
        end3D();
        drawParticles(app, *fx, false);
        C2D_Flush();
    }
    bindDragons(projection);
    for (int i = 0; i < 3; ++i) g_headSet[i] = false;
    for (int i = 0; i < count; ++i) {
        if (!drawn[i]) continue;
        if (dragons[i].dragon->stage == Stage::Egg) {
            float local[3];
            localLight(at[i], blend, local);
            lightDragon(light, local);
            submitEgg(app, *dragons[i].dragon, *dragons[i].egg, view, {at[i].x, at[i].y, kNestFloor});
            g_heads[i] = {at[i].x, at[i].y, kNestFloor + 0.8f};  // effects rise from its top
            g_headSet[i] = true;
            continue;
        }
        // The first dragon is the one you're caring for: full detail. Others use LOD1.
        if (!pose(app, *dragons[i].dragon, dragons[i].actor, now, i == 0 ? 0 : 1, g_posed)) continue;
        modelMatrix(g_posed, model);
        float local[3];
        localLight(g_posed.pos, blend, local);
        lightDragon(light, local);
        submit(app, g_posed, view, model);
        if (g_posed.form->headBone >= 0) {
            g_heads[i] = apply(model, g_posed.poseMat[g_posed.form->headBone].translation());
            g_headSet[i] = true;
        }
    }
    drawRoom(app, projection, view, blend, true);
    end3D();
    if (fx) drawParticles(app, *fx, true);
}

void drawCloseUp(App& app, const Dragon& d, const DenActor* actor, const EggMotion* egg, s64 now) {
    if (!g_ready) return;
    ++g_frame;
    if (d.stage == Stage::Egg) {  // the egg you rub, filling the view from the front-left
        if (!g_egg.ok || !egg) return;
        C3D_Mtx projection, view;
        const Vec3 target{0, 0, 0.5f};
        const float dist = 0.78f / std::tan(kFovY * 0.5f);
        Mtx_PerspTilt(&projection, kFovY, C3D_AspectRatioBot, 0.05f, dist * 4.0f, false);
        lookAt(view, target + normalize(Vec3{-0.3f, -0.95f, 0.25f}) * dist, target);
        C2D_Flush();
        bindDragons(projection);
        const float plain[3] = {1, 1, 1};
        lightDragon(dragonLight(dayBlend(now)), plain);
        submitEgg(app, d, *egg, view, {0, 0, 0});
        end3D();
        return;
    }
    if (!pose(app, d, actor, now, 0, g_posed) || g_posed.form->headBone < 0 || g_posed.form->chestBone < 0) return;
    C3D_Mtx projection, view, model;
    modelMatrix(g_posed, model);
    // Head and chest, seen from in front of the dragon wherever it stands: the parts you pet.
    const Vec3 head = apply(model, g_posed.poseMat[g_posed.form->headBone].translation());
    const Vec3 chest = apply(model, g_posed.poseMat[g_posed.form->chestBone].translation());
    const Vec3 target = lerp(head, chest, 0.3f);
    const float radius = length(head - chest) * 0.75f;
    const float ch = std::cos(g_posed.heading), sh = std::sin(g_posed.heading);
    const Vec3 local = normalize(Vec3{-0.3f, -0.95f, 0.18f});  // front-left of the face
    const Vec3 dir{local.x * ch - local.y * sh, local.x * sh + local.y * ch, local.z};
    const float dist = radius / std::tan(kFovY * 0.5f);
    Mtx_PerspTilt(&projection, kFovY, C3D_AspectRatioBot, 0.05f, dist * 4.0f, false);
    lookAt(view, target + dir * dist, target);
    C2D_Flush();
    bindDragons(projection);
    const float plain[3] = {1, 1, 1};
    lightDragon(dragonLight(dayBlend(now)), plain);
    submit(app, g_posed, view, model);
    end3D();
}

bool project(Vec3 p, float& x, float& y, float& pixelsPerUnit) {
    return g_denViewSet && projectWith(g_denView, p, x, y, pixelsPerUnit);
}

bool headOf(int i, Vec3& out) {
    if (i < 0 || i >= 3 || !g_headSet[i]) return false;
    out = g_heads[i];
    return true;
}

bool roomReady() { return g_room.ok; }
bool eggReady() { return g_egg.ok; }

u32 backdrop(s64 now) {
    if (!g_room.ok) return theme::kDenPlum;
    const DayBlend b = dayBlend(now);
    const u8* a = &g_room.scene.backdrop[b.a * 4];
    const u8* c = &g_room.scene.backdrop[b.b * 4];
    auto mixc = [&](int k) { return static_cast<u8>(a[k] + (c[k] - a[k]) * b.t); };
    return theme::rgba(mixc(0), mixc(1), mixc(2));
}

const AnimLibrary* anims() { return g_animsOk ? &g_anims : nullptr; }
const int* clipIndex(int form) { return g_clipIndex[form == kFormHatchling ? kFormHatchling : kFormGrown]; }
const ModelData* model(int form) { return g_ready && form >= 0 && form < kFormCount ? &g_forms[form][0].model : nullptr; }
const AnimBinding* binding(int form) { return g_animsOk && form >= 0 && form < kFormCount ? &g_bind[form] : nullptr; }

}  // namespace ec::r3d
