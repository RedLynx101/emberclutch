#include "app/render3d.hpp"

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>
#include <tex3ds.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "app/autotest.hpp"
#include "app/perf.hpp"
#include "app/theme.hpp"
#include "app/ui_draw.hpp"
#include "core/anim.hpp"
#include "core/care.hpp"
#include "core/daylight.hpp"
#include "core/dragon_mesh.hpp"
#include "core/egg.hpp"
#include "core/prop_mesh.hpp"
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

// GPU memory is freed only once the GPU is done with it. The last frame is still being drawn
// while the next one's update runs, and a change mid-frame (the dev menu draws with the bottom
// screen) comes after the top screen's draws were queued: freeing straight away left the GPU
// reading memory already handed out again, and on the 3DS it hung (Next style, run 3; the
// emulator never minded). What's retired is freed in frameBegun(), after C3D_FrameBegin has
// waited for every frame before.
std::vector<void*> g_graveLinear;
std::vector<C3D_Tex> g_graveTex;

void retire(void* p) {
    if (p) g_graveLinear.push_back(p);
}

void retireTex(const C3D_Tex& t) { g_graveTex.push_back(t); }

void bury() {
    for (void* p : g_graveLinear) linearFree(p);
    g_graveLinear.clear();
    for (C3D_Tex& t : g_graveTex) C3D_TexDelete(&t);
    g_graveTex.clear();
}

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
        retire(vbo);
        retire(ibo);
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
    BoneCapsule caps[kMaxCapsules];      // touch picking (core/care)
    int capCount = 0;
    int headBone = -1, chestBone = -1, eyesBone = -1, jawBone = -1;
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
        retire(pos);
        pos = nullptr;
        for (u8*& c : color) {
            retire(c);
            c = nullptr;
        }
        for (Run& r : runs) {
            retire(r.idx);
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
    float lift = 0;             // DenActor::lift (a hatchling climbing out of its shell)
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
EggForm g_egg;      // full detail: the close-up, and a lone egg in the den
EggForm g_eggLod1;  // the den's egg when there's more to draw (tools/blender/egg_model.py --lod 1)
Vec3 g_camTarget;                // smoothed den camera target
float g_camRadius = 0;           // smoothed den framing radius (0: not set yet)
Vec3 g_camEye;                   // last den camera position: where "the player" is
C3D_Mtx g_denView;               // last den camera, for projecting particles and the egg
bool g_denViewSet = false;
Vec3 g_heads[kDenShown];         // den dragons' heads in the last drawDen (an egg's top)
bool g_headSet[kDenShown] = {};
Vec3 g_mouths[kDenShown];        // ...and their mouths (a carried ball rides there)
bool g_mouthSet[kDenShown] = {};

// Poses made ahead of the frame (poseAhead), for the frame whose app.t they were made at.
struct Ahead {
    const Dragon* dragon = nullptr;
    const DenActor* actor = nullptr;
    int lod = -1;
    bool ok = false;
    Posed posed;
};
Ahead g_ahead[kDenShown];
float g_aheadAt = -1.0f;

// The last close-up: hands-on care picks against what it showed (WP7).
struct CloseUpState {
    bool set = false;
    C3D_Mtx view, model;
    Vec3 eye;
    Posed posed;
};
CloseUpState g_close;

// Props (WP7): the ball and the bath tub, drawn with the dragon program (one bone).
GpuMesh g_ballMesh, g_tubMesh;
const Ball* g_ball = nullptr;
bool g_tubOut = false;
Vec2 g_tubAt;
float g_tubSize = 0.95f;
Quat g_ballSpin{0, 0, 0, 1};
Vec3 g_follow;          // the den camera also watches this (a thrown ball)
// The den's bought things (WP7): meshes built on first use, one per toy and decor item.
const DenThings* g_things = nullptr;
GpuMesh g_toyMeshes[kToys], g_bowlFoodMesh, g_decorMeshes[kItems], g_homeRugMesh;
float g_followWeight = 0;

// Toon ramp on L.N (signed): plum shadow, a mid band, full light.
float toonRamp(float x, float) { return x < 0.12f ? 0.0f : (x < 0.45f ? 0.62f : 1.0f); }
// Rim on N.V: a thin bright band on the silhouette.
float rimBand(float x, float) { return x < 0.28f ? 1.0f : (x < 0.38f ? 0.35f : 0.0f); }
// Review R5's styles: v1 softer, three-band shading and a wider rim; v3 harder, darker.
float toonRampSoft(float x, float) { return x < 0.05f ? 0.0f : (x < 0.3f ? 0.5f : (x < 0.6f ? 0.82f : 1.0f)); }
float rimWide(float x, float) { return x < 0.33f ? 1.0f : (x < 0.46f ? 0.45f : 0.0f); }
float toonRampHard(float x, float) { return x < 0.2f ? 0.0f : (x < 0.5f ? 0.5f : 1.0f); }

int g_style = 0;  // review R5 (D47): 0 current, 1 surface, 2 shape, 3 bold
const char* const kStyleDir[kStyleCount] = {"romfs:/models/", "romfs:/models/v1/", "romfs:/models/v2/",
                                            "romfs:/models/v3/"};

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
    f.jawBone = f.model.skel.find("jaw");
    f.capCount = buildCapsules(f.model, f.caps, kMaxCapsules);
    f.chestBone = f.model.skel.find("chest");
    f.ok = true;
    return true;
}

bool loadEgg(const char* path, EggForm& egg) {
    std::vector<u8> bytes;
    if (!readFile(path, bytes) || !loadModel(bytes.data(), bytes.size(), egg.model)) return false;
    const MeshData* shell = egg.model.findMesh(kMeshBody, kGroupBody, 0);
    egg.ok = shell && egg.model.skel.count == 2 && fillStatic(egg.shell, *shell);
    return egg.ok;
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

// Lowest point of the animated body, so the feet (or belly, or back) rest on the floor. The
// tail doesn't hold it up: a big dragon's wag swung its tail below its feet and the whole
// dragon rose with it (Noah, run 3), so vertices mostly on tail bones don't count.
float animatedGround(const ModelData& m, const Mat34* skin) {
    const MeshData* body = m.findMesh(kMeshBody, kGroupBody, 0);
    if (!body) return 0;
    bool tail[kMaxPalette] = {};
    for (const char* name : {"tail1", "tail2", "tail3", "tail4"}) {
        const int bone = m.skel.find(name);
        for (int i = 0; i < body->paletteCount && bone >= 0; ++i) tail[i] = tail[i] || body->palette[i] == bone;
    }
    float low = 1e9f;
    for (int v = 0; v < body->vertexCount; v += 3) {
        const u8* w = &body->skin[std::size_t(v) * 4];
        if (tail[w[0]] && w[2] >= 128) continue;
        const Vec3 p = skinPoint(*body, v, body->pos[v], skin);
        if (p.z < low) low = p.z;
    }
    return low < 1e8f ? low : 0.0f;
}

bool pose(App& app, const Dragon& d, const DenActor* actor, s64 now, int lod, Posed& out);

// This frame's pose made ahead for this dragon (poseAhead), else posed now.
bool posedFor(App& app, const Dragon& d, const DenActor* actor, s64 now, int lod, Posed& out) {
    if (g_aheadAt == app.t) {
        for (const Ahead& a : g_ahead) {
            if (a.ok && a.dragon == &d && a.actor == actor && a.lod == lod) {
                out = a.posed;
                return true;
            }
        }
    }
    return pose(app, d, actor, now, lod, out);
}

// Poses a dragon: idle pose + its actor's animation, placed where its behavior stands.
bool pose(App& app, const Dragon& d, const DenActor* actor, s64 now, int lod, Posed& out) {
    perf::Scope timed(perf::Pose);
    Cache* c = cacheFor(d, now, lod);
    if (!c) return false;
    const Form& f = g_forms[c->form][lod];
    updateDust(*c, f, d);
    out.size = sizeScale(d.genome);
    out.scale = growthScale(growthFor(d.stage, stageProgress(d, now))) * out.size;
    out.pos = actor ? actor->behavior.pos : Vec2{};
    out.heading = actor ? actor->behavior.heading : 0.0f;
    out.lift = actor ? actor->lift : 0.0f;
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
            const Vec3 rel{g_camEye.x - out.pos.x, g_camEye.y - out.pos.y,
                           g_camEye.z - out.root[1] * out.scale - out.lift};
            Vec3 local{rel.x * ch + rel.y * sh, -rel.x * sh + rel.y * ch, rel.z};
            local.y += out.root[0] * out.scale;
            local = local * (1.0f / out.size);
            local.z += c->groundNow;
            applyLookAt(f.model.skel, g_bind[c->form], bones, local, actor->look * (1.0f - actor->gazeWeight));
        }
        // Hands-on care: it looks at the food you hold out, or leans toward your hand.
        if (actor->gazeWeight > 0.01f)
            applyLookAt(f.model.skel, g_bind[c->form], bones, actor->gazeLocal, actor->gazeWeight);
    }
    if (actor && f.jawBone >= 0 && actor->jawOpen > 0.01f) {  // opening for the food
        const Quat q = quatFromPitchYawRoll(-actor->jawOpen * 30.0f * kDegToRad, 0, 0);
        const Quat& rest = g_bind[c->form].rest[f.jawBone];
        bones[f.jawBone].rot = mul(bones[f.jawBone].rot, mul(mul(conjugate(rest), q), rest));
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
    Mtx_Translate(&out, p.pos.x, p.pos.y, p.root[1] * p.scale + p.lift, true);
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
    if (g_style == 3) {  // v3: no pattern on the scales; the veins (skin B) glow after the light, in place of the rim
        C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
        env = C3D_GetTexEnv(5);
        C3D_TexEnvInit(env);
        C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, GPU_CONSTANT, GPU_PREVIOUS);
        C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_B, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR);
        C3D_TexEnvFunc(env, C3D_RGB, GPU_MULTIPLY_ADD);
        C3D_TexEnvSrc(env, C3D_Alpha, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT);
        C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
        C3D_TexEnvColor(env, 0xFF000000u | (u32(color.b) << 16) | (u32(color.g) << 8) | color.r);
        return;
    }
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

// The style's colours on top of the genome's (review R5): v3 turns the scales dark and lets
// the fire show through: the veins (the pattern colour), the eyes and the wings glow.
void stylePalette(Rgb pal[kPalCount]) {
    if (g_style != 3) return;
    auto scale = [](Rgb c, float k, int add) {
        return Rgb{toByte(c.r * k / 255.0f + add / 255.0f), toByte(c.g * k / 255.0f + add / 255.0f),
                   toByte(c.b * k / 255.0f + add / 255.0f)};
    };
    const Rgb glow = pal[kPalGlow];
    const Rgb base = scale(pal[kPalBase], 0.2f, 8);
    pal[kPalAccent] = {static_cast<u8>(base.r / 2 + pal[kPalAccent].r / 8), static_cast<u8>(base.g / 2 + pal[kPalAccent].g / 8),
                       static_cast<u8>(base.b / 2 + pal[kPalAccent].b / 8)};
    pal[kPalBase] = base;
    pal[kPalHorn] = {33, 23, 26};
    pal[kPalIris] = glow;
    pal[kPalPattern] = scale(glow, 1.1f, 0);
    pal[kPalMembrane] = scale(glow, 0.62f, 0);
}

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
        retire(c.dust);
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
        loadEgg("romfs:/models/egg.ecm", g_egg);  // without it, eggs stay 2D
        loadEgg("romfs:/models/egg_lod1.ecm", g_eggLod1);
    }
    return g_ready;
}

void frameBegun() { bury(); }

void shutdown() {
    for (auto& lods : g_forms)
        for (Form& f : lods) {
            f.body.release();
            for (GpuMesh& w : f.wings) w.release();
            if (f.skinOk) retireTex(f.skin);
            f.skinOk = false;
        }
    for (Cache& c : g_caches) {
        c.parts.release();
        retire(c.dust);
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
    g_eggLod1.shell.release();
    g_ballMesh.release();
    g_tubMesh.release();
    for (GpuMesh& m : g_toyMeshes) m.release();
    for (GpuMesh& m : g_decorMeshes) m.release();
    g_bowlFoodMesh.release();
    g_homeRugMesh.release();
    g_egg.ok = g_eggLod1.ok = false;
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
    bury();  // the loop is over: nothing is drawn any more
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
    // Texture unit 1 (the dust ramp) stays bound: citro2d's stages never sample it, and
    // citro3d can't unbind units 1-2. C3D_TexBind(1, nullptr) reads the null texture's type
    // first: the emulator reads 0 there, the 3DS faults (the first hardware crash, 2026-09-24).
    for (int i = 0; i < 6; ++i) C3D_TexEnvInit(C3D_GetTexEnv(i));
    C2D_Prepare();
    prepare2D();
}

// The hearth's flicker: two uneven waves.
float flicker(float t) { return 0.84f + 0.1f * std::sin(t * 13.0f) + 0.06f * std::sin(t * 31.0f + 1.0f); }

// The den room with the static program: its opaque parts (one draw) or, after the dragons,
// its additive glows (sunbeam, flames), lit by the two lighting sets of the time of day.
void drawRoom(App& app, const C3D_Mtx& projection, const C3D_Mtx& view, const DayBlend& blend, bool glows) {
    perf::Scope timed(perf::Room);
    if (app.gpuProbe == 1) return;
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
    perf::Scope timed(perf::Submit);
    C3D_Mtx modelView;
    Mtx_Multiply(&modelView, &view, &model);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_locModelView, &modelView);
    const Dragon& d = *p.dragon;
    Rgb pal[kPalCount];
    dragonPalette(d.genome, pal);
    stylePalette(pal);
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
void submitEgg(App& app, const EggForm& egg, const Dragon& d, const EggMotion& motion, const C3D_Mtx& view, Vec3 at) {
    Mat34 skin[2];
    eggSkin(egg.model, motion, skin);
    C3D_Mtx model, modelView;
    Mtx_Identity(&model);
    Mtx_Translate(&model, at.x, at.y, at.z - groundOffset(egg.model, skin), true);
    Mtx_Multiply(&modelView, &view, &model);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_locModelView, &modelView);
    Rgb pal[kPalCount];
    float glow[kPalCount];
    eggPalette(d, 0.85f + 0.15f * std::sin(app.t * 2.2f), pal, glow, app.t);
    for (int i = 0; i < kPalCount; ++i)
        C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locPalette + i, pal[i].r / 255.0f, pal[i].g / 255.0f, pal[i].b / 255.0f,
                      glow[i]);
    bindSkin(nullptr);  // the shell sits on the clean corner: no pattern, no dust
    dragonPattern(kPatternSolid, {0, 0, 0});
    drawMesh(app, egg.shell, skin);
}

// ------------------------------------------------------------------------------ props (WP7)
constexpr float kPi = 3.14159265f;

void propVertex(std::vector<Vec3>& pos, std::vector<Vec3>& nrm, std::vector<u8>& paint, Vec3 p, Vec3 n, u8 slot) {
    pos.push_back(p);
    nrm.push_back(n);
    paint.insert(paint.end(), {slot, slot, 0, 0});
}

bool fillProp(GpuMesh& g, const std::vector<Vec3>& pos, const std::vector<Vec3>& nrm, const std::vector<u8>& paint,
              const std::vector<u16>& idx) {
    const std::size_t n = pos.size();
    std::vector<u8> skin;
    std::vector<float> uv;
    for (std::size_t v = 0; v < n; ++v) {
        skin.insert(skin.end(), {0, 0, 255, 0});
        uv.insert(uv.end(), {kCleanUv, kCleanUv});
    }
    const u8 palette[1] = {0};
    return fill(g, static_cast<int>(n), pos.data(), nrm.data(), skin.data(), paint.data(), uv.data(), idx.data(),
                static_cast<int>(idx.size()), palette, 1);
}

// A unit ball: red (palette slot 0) with a cream band (slot 1).
bool makeBallMesh(GpuMesh& g) {
    constexpr int kSeg = 12, kRing = 8;
    std::vector<Vec3> pos, nrm;
    std::vector<u8> paint;
    std::vector<u16> idx;
    for (int r = 0; r <= kRing; ++r) {
        const float th = kPi * r / kRing;
        for (int s = 0; s <= kSeg; ++s) {
            const float ph = 2 * kPi * s / kSeg;
            const Vec3 p{std::sin(th) * std::cos(ph), std::sin(th) * std::sin(ph), std::cos(th)};
            propVertex(pos, nrm, paint, p, p, std::fabs(p.x) < 0.22f ? 1 : 0);
        }
    }
    for (int r = 0; r < kRing; ++r)
        for (int s = 0; s < kSeg; ++s) {
            const u16 a = static_cast<u16>(r * (kSeg + 1) + s), b = static_cast<u16>(a + kSeg + 1);
            idx.insert(idx.end(), {a, b, static_cast<u16>(a + 1), static_cast<u16>(a + 1), b, static_cast<u16>(b + 1)});
        }
    return fillProp(g, pos, nrm, paint, idx);
}

// A wooden tub, radius 1, height 0.55: staves in two woods (slots 0 and 1), water (slot 2).
bool makeTubMesh(GpuMesh& g) {
    constexpr int kSeg = 16;
    constexpr float kH = 0.55f, kIn = 0.9f, kWater = 0.4f;
    std::vector<Vec3> pos, nrm;
    std::vector<u8> paint;
    std::vector<u16> idx;
    auto quad = [&](Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 n, u8 slot) {  // counter-clockwise, seen from n
        const u16 base = static_cast<u16>(pos.size());
        for (Vec3 p : {a, b, c, d}) propVertex(pos, nrm, paint, p, n, slot);
        idx.insert(idx.end(), {base, static_cast<u16>(base + 1), static_cast<u16>(base + 2), base,
                               static_cast<u16>(base + 2), static_cast<u16>(base + 3)});
    };
    for (int s = 0; s < kSeg; ++s) {
        const float a0 = 2 * kPi * s / kSeg, a1 = 2 * kPi * (s + 1) / kSeg, am = (a0 + a1) * 0.5f;
        const Vec3 o0{std::cos(a0), std::sin(a0), 0}, o1{std::cos(a1), std::sin(a1), 0}, om{std::cos(am), std::sin(am), 0};
        const u8 wood = static_cast<u8>(s % 2);
        quad(o0, o1, o1 + Vec3{0, 0, kH}, o0 + Vec3{0, 0, kH}, om, wood);                              // outside
        quad(o1 * kIn + Vec3{0, 0, kH}, o1 * kIn, o0 * kIn, o0 * kIn + Vec3{0, 0, kH}, om * -1.0f, wood);  // inside
        quad(o0 * kIn + Vec3{0, 0, kH}, o0 + Vec3{0, 0, kH}, o1 + Vec3{0, 0, kH}, o1 * kIn + Vec3{0, 0, kH},
             {0, 0, 1}, wood);                                                                          // rim
        quad(o0 * kIn + Vec3{0, 0, kWater}, Vec3{0, 0, kWater}, Vec3{0, 0, kWater}, o1 * kIn + Vec3{0, 0, kWater},
             {0, 0, 1}, 2);  // water: a fan of (degenerate) quads
    }
    return fillProp(g, pos, nrm, paint, idx);
}

void setPalette(std::initializer_list<Rgb> colours) {
    int i = 0;
    for (const Rgb& c : colours)
        C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locPalette + i++, c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f);
}

void propModelView(const C3D_Mtx& view, Vec3 at, float scale, const Quat* rot) {
    C3D_Mtx model, modelView;
    Mtx_Identity(&model);
    Mtx_Translate(&model, at.x, at.y, at.z, true);
    if (rot) {
        C3D_Mtx r, t;
        Mtx_FromQuat(&r, Quat_New(rot->x, rot->y, rot->z, rot->w));
        Mtx_Multiply(&t, &model, &r);
        model = t;
    }
    Mtx_Scale(&model, scale, scale, scale);
    Mtx_Multiply(&modelView, &view, &model);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_locModelView, &modelView);
}

bool uploadProp(GpuMesh& g, const PropMesh& m) {
    const std::size_t n = m.pos.size();
    std::vector<u8> skin;
    std::vector<float> uv;
    for (std::size_t v = 0; v < n; ++v) {
        skin.insert(skin.end(), {0, 0, 255, 0});
        uv.insert(uv.end(), {kCleanUv, kCleanUv});
    }
    const u8 palette[1] = {0};
    return fill(g, static_cast<int>(n), m.pos.data(), m.nrm.data(), skin.data(), m.paint.data(), uv.data(),
                m.idx.data(), static_cast<int>(m.idx.size()), palette, 1);
}

// An item's four colours; `glow` is how much its emissive parts shine (the palette alpha).
void setLook(const PropLook& look, float glow) {
    for (int i = 0; i < 4; ++i)
        C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locPalette + i, look.colour[i].r / 255.0f, look.colour[i].g / 255.0f,
                      look.colour[i].b / 255.0f, glow);
}

void modelView(const C3D_Mtx& view, const C3D_Mtx& model) {
    C3D_Mtx modelView;
    Mtx_Multiply(&modelView, &view, &model);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_locModelView, &modelView);
}

// Stands at `at`, turned `yaw` about +Z, scaled (and sx more along its own X).
C3D_Mtx placeMatrix(Vec3 at, float yaw, float sx = 1.0f, float scale = 1.0f) {
    C3D_Mtx model;
    Mtx_Identity(&model);
    Mtx_Translate(&model, at.x, at.y, at.z, true);
    Mtx_RotateZ(&model, yaw, true);
    Mtx_Scale(&model, sx * scale, scale, scale);
    return model;
}

// The rope's unit length along X, stretched from a to b (it stays level across).
C3D_Mtx spanMatrix(Vec3 a, Vec3 b) {
    const Vec3 x = b - a;
    const float len = std::fmax(1e-3f, length(x));
    const Vec3 dir = x * (1.0f / len);
    Vec3 side = cross(Vec3{0, 0, 1}, dir);
    side = length(side) < 1e-3f ? Vec3{0, 1, 0} : normalize(side);
    const Vec3 up = cross(dir, side);
    const Vec3 mid = (a + b) * 0.5f;
    C3D_Mtx m;
    Mtx_Zeros(&m);
    // citro3d rows: r[i].x/y/z/w = row i (w, z, y, x in memory; the accessors name them)
    m.r[0].x = x.x, m.r[0].y = side.x, m.r[0].z = up.x, m.r[0].w = mid.x;
    m.r[1].x = x.y, m.r[1].y = side.y, m.r[1].z = up.y, m.r[1].w = mid.y;
    m.r[2].x = x.z, m.r[2].y = side.z, m.r[2].z = up.z, m.r[2].w = mid.z;
    m.r[3].w = 1.0f;
    return m;
}

// The toys, the bowl's food and the decor (WP7).
void drawThings(App& app, const C3D_Mtx& view, const DenThings& t) {
    perf::Scope timed(perf::Room);
    if (app.gpuProbe == 1) return;
    const Mat34 identity[1] = {Mat34::identity()};
    const float night = 1.0f - t.daylight;
    for (int s = 0; s < kDecorSpots; ++s) {
        const Item it = t.decor[s];
        if (it >= Item::Count && s != 0) continue;  // an empty spot (the rug's has the den's own)
        GpuMesh& g = it < Item::Count ? g_decorMeshes[static_cast<int>(it)] : g_homeRugMesh;
        if (!g.vbo && !uploadProp(g, it < Item::Count ? decorMesh(it) : homeRugMesh())) continue;
        const PropLook look = propLook(it);
        setLook(look, look.glow * (0.3f + 0.7f * night));
        const DecorPlace p = decorPlace(s);
        modelView(view, placeMatrix(p.at, p.yaw, 1.0f, p.scale));
        drawMesh(app, g, identity);
    }
    for (int k = 0; k < kToys; ++k) {
        if (!t.toy[k]) continue;
        GpuMesh& g = g_toyMeshes[k];
        if (!g.vbo && !uploadProp(g, toyMesh(k))) continue;
        PropLook look = propLook(static_cast<Item>(k));
        setLook(look, 0);
        if (k == 1 && t.ropeSpan) {
            modelView(view, spanMatrix(t.ropeA, t.ropeB));
        } else if (k == 1) {
            modelView(view, placeMatrix(t.toyAt[k] + Vec3{0, 0, 0.05f}, t.toyYaw[k], kRopeLength));
        } else if (k == 2) {
            C3D_Mtx model, r, out;
            Mtx_Identity(&model);
            Mtx_Translate(&model, t.toyAt[k].x, t.toyAt[k].y, t.toyAt[k].z, true);
            Mtx_FromQuat(&r, Quat_New(t.orbSpin.x, t.orbSpin.y, t.orbSpin.z, t.orbSpin.w));
            Mtx_Multiply(&out, &model, &r);
            modelView(view, out);
        } else {
            modelView(view, placeMatrix(t.toyAt[k], t.toyYaw[k]));
        }
        drawMesh(app, g, identity);
        if (k == 3 && t.bowlFood < Food::Count) {  // the food heaped in the bowl
            if (!g_bowlFoodMesh.vbo && !uploadProp(g_bowlFoodMesh, bowlFoodMesh())) continue;
            static constexpr Rgb kFoodColour[static_cast<int>(Food::Count)] = {
                {214, 60, 40}, {170, 190, 210}, {110, 110, 210}, {214, 170, 80}, {190, 230, 170},
                {250, 214, 80}, {214, 170, 110}, {150, 90, 50}, {240, 110, 60}, {200, 150, 100}};
            look.colour[3] = kFoodColour[static_cast<int>(t.bowlFood)];
            setLook(look, 0);
            drawMesh(app, g_bowlFoodMesh, identity);
        }
    }
}

// Draws the ball and the tub (after bindDragons and a lightDragon), and the den's things.
void drawProps(App& app, const C3D_Mtx& view) {
    perf::Scope timed(perf::Room);
    if (!g_ballMesh.vbo) makeBallMesh(g_ballMesh);
    if (!g_tubMesh.vbo) makeTubMesh(g_tubMesh);
    const Mat34 identity[1] = {Mat34::identity()};
    bindSkin(nullptr);
    dragonPattern(kPatternSolid, {0, 0, 0});
    if (g_tubOut && g_tubMesh.vbo) {
        setPalette({{150, 98, 56}, {178, 124, 74}, {120, 178, 222}});
        propModelView(view, {g_tubAt.x, g_tubAt.y, 0}, g_tubSize, nullptr);
        drawMesh(app, g_tubMesh, identity);
    }
    if (g_ball && g_ball->active && g_ballMesh.vbo) {
        const Vec3 v = g_ball->vel;
        const float s = std::sqrt(v.x * v.x + v.y * v.y);
        if (s > 1e-3f && !g_ball->held)  // it rolls the way it moves
            g_ballSpin = normalize(mul(quatAxisAngle(normalize(Vec3{-v.y, v.x, 0}), s * app.dt / g_ball->radius), g_ballSpin));
        setPalette({{232, 82, 66}, {247, 234, 200}});
        propModelView(view, g_ball->pos, g_ball->radius, &g_ballSpin);
        drawMesh(app, g_ballMesh, identity);
    }
    static const DenThings kBare;  // scenes that set nothing still have the den's own rug
    drawThings(app, view, g_things ? *g_things : kBare);
}

// ------------------------------------------------------------------------------ picking (WP7)
// Den space -> a dragon's armature space (the inverse of modelMatrix), for points and directions.
Vec3 toArmature(const Posed& p, Vec3 w, bool point) {
    Vec3 v = point ? Vec3{w.x - p.pos.x, w.y - p.pos.y, w.z - p.root[1] * p.scale} : w;
    const float c = std::cos(p.heading), s = std::sin(p.heading);
    Vec3 r{v.x * c + v.y * s, -v.x * s + v.y * c, v.z};
    if (point) r.y += p.root[0] * p.scale;
    r = r * (1.0f / p.size);
    if (point) r.z += p.ground;
    return r;
}

// The bottom screen's projection (the close-up camera): pixels, and pixels per unit there.
bool projectBottom(const C3D_Mtx& view, Vec3 p, float& x, float& y, float& ppu) {
    const Vec3 v = apply(view, p);
    if (v.z > -0.02f) return false;
    ppu = (kScreenH * 0.5f / std::tan(kFovY * 0.5f)) / -v.z;
    x = kBotW * 0.5f + v.x * ppu;
    y = kScreenH * 0.5f - v.y * ppu;
    return true;
}

// The ray under a bottom-screen point, in den space.
Vec3 rayThrough(Vec2 touch) {
    const float f = kScreenH * 0.5f / std::tan(kFovY * 0.5f);
    const Vec3 d{(touch.x - kBotW * 0.5f) / f, (kScreenH * 0.5f - touch.y) / f, -1.0f};
    const C3D_Mtx& m = g_close.view;
    return normalize(Vec3{m.r[0].x * d.x + m.r[1].x * d.y + m.r[2].x * d.z, m.r[0].y * d.x + m.r[1].y * d.y + m.r[2].y * d.z,
                          m.r[0].z * d.x + m.r[1].z * d.y + m.r[2].z * d.z});
}

Mat34 inverseAffine(const Mat34& a) {
    const float(*m)[4] = a.m;
    const float det = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                      m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    const float k = std::fabs(det) > 1e-9f ? 1.0f / det : 0.0f;
    Mat34 r;
    r.m[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * k;
    r.m[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * k;
    r.m[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * k;
    r.m[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * k;
    r.m[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * k;
    r.m[1][2] = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) * k;
    r.m[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * k;
    r.m[2][1] = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) * k;
    r.m[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * k;
    for (int i = 0; i < 3; ++i)
        r.m[i][3] = -(r.m[i][0] * m[0][3] + r.m[i][1] * m[1][3] + r.m[i][2] * m[2][3]);
    return r;
}

// Where the mouth is, armature space: the end of the jaw's skin (or the head's).
bool mouthLocal(const Posed& p, Vec3& out) {
    const Form& f = *p.form;
    for (int pass = 0; pass < 2; ++pass) {
        const int bone = pass == 0 ? f.jawBone : f.headBone;
        for (int i = 0; i < f.capCount; ++i)
            if (f.caps[i].bone == bone) {
                out = transformPoint(p.poseMat[bone], {f.caps[i].cx, f.caps[i].t1, f.caps[i].cz});
                return true;
            }
    }
    return false;
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
    perf::Scope timed(perf::Fx);
    if (app.gpuProbe == 4) return;
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
    if (count > kDenShown) count = kDenShown;
    // A lone egg fills the view in full detail; with more in the den, eggs are small: LOD1.
    const EggForm& denEgg = count > 1 && g_eggLod1.ok ? g_eggLod1 : g_egg;
    const DenLayout den;
    auto nestAt = [&](s8 n) { return den.eggNests[n >= 0 && n < DenLayout::kNests ? n : 0]; };
    // Frame everyone: the centre of the dragons' positions, wide enough for the biggest one
    // and the spread between them. The camera follows smoothly as they wander. With no
    // dragon out yet (an egg), it looks at the egg nest.
    float maxRadius = 0, spread = 0;
    Vec2 mid{0, 0}, at[kDenShown];
    bool drawn[kDenShown] = {};
    int shown = 0;
    for (int i = 0; i < count; ++i) {
        const Dragon& d = *dragons[i].dragon;
        float r;
        if (d.stage == Stage::Egg) {  // eggs sit in the egg nest
            if (!g_egg.ok || !dragons[i].egg) continue;
            r = 1.2f;
            at[i] = nestAt(dragons[i].nest);
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
        mid = den.eggNests[0];
        maxRadius = 1.5f;
    } else {
        return;
    }
    float radius = maxRadius + spread * 0.9f;
    if (g_followWeight > 0.01f) {  // a thrown ball: keep it in view with the dragons
        const float apart = std::hypot(g_follow.x - mid.x, g_follow.y - mid.y);
        mid.x += (g_follow.x - mid.x) * 0.45f * g_followWeight;
        mid.y += (g_follow.y - mid.y) * 0.45f * g_followWeight;
        radius = std::fmax(radius, (apart * 0.6f + maxRadius * 0.6f) * g_followWeight + radius * (1 - g_followWeight));
    }
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
    for (int i = 0; i < kDenShown; ++i) g_headSet[i] = g_mouthSet[i] = false;
    for (int i = 0; i < count; ++i) {
        if (!drawn[i] || app.gpuProbe == 2) continue;
        if (dragons[i].dragon->stage == Stage::Egg) {
            float local[3];
            localLight(at[i], blend, local);
            lightDragon(light, local);
            submitEgg(app, denEgg, *dragons[i].dragon, *dragons[i].egg, view, {at[i].x, at[i].y, kNestFloor});
            g_heads[i] = {at[i].x, at[i].y, kNestFloor + 0.8f};  // effects rise from its top
            g_headSet[i] = true;
            continue;
        }
        // The first dragon is the one you're caring for: full detail. Others use LOD1.
        if (!posedFor(app, *dragons[i].dragon, dragons[i].actor, now, i == 0 ? 0 : 1, g_posed)) continue;
        modelMatrix(g_posed, model);
        float local[3];
        localLight(g_posed.pos, blend, local);
        lightDragon(light, local);
        submit(app, g_posed, view, model);
        if (dragons[i].egg && g_egg.ok) {  // just hatched: the empty shell is still in the nest
            const Vec2 n = nestAt(dragons[i].nest);
            float nest[3];
            localLight(n, blend, nest);
            lightDragon(light, nest);
            submitEgg(app, denEgg, *dragons[i].dragon, *dragons[i].egg, view, {n.x, n.y, kNestFloor});
        }
        if (g_posed.form->headBone >= 0) {
            g_heads[i] = apply(model, g_posed.poseMat[g_posed.form->headBone].translation());
            g_headSet[i] = true;
        }
        Vec3 mouth;
        if (mouthLocal(g_posed, mouth)) {
            g_mouths[i] = apply(model, mouth);
            g_mouthSet[i] = true;
        }
    }
    {
        const float plain[3] = {1, 1, 1};
        lightDragon(light, plain);
        drawProps(app, view);
    }
    drawRoom(app, projection, view, blend, true);
    end3D();
    if (fx) drawParticles(app, *fx, true);
}

void drawShowcase(App& app, const Dragon& d, const EggMotion* egg, s64 now, float spin, ClipId clip) {
    if (!g_ready) return;
    ++g_frame;
    C3D_Mtx projection, view;
    const float plain[3] = {1, 1, 1};
    const Vec3 dir = normalize(Vec3{-0.35f * std::cos(spin) - 0.9f * std::sin(spin), 0.35f * std::sin(spin) - 0.9f * std::cos(spin), 0.3f});
    if (d.stage == Stage::Egg) {
        if (!g_egg.ok || !egg) return;
        const Vec3 target{0, 0, 0.62f};
        const float dist = 1.0f / std::tan(kFovY * 0.5f);
        Mtx_PerspTilt(&projection, kFovY, C3D_AspectRatioTop, 0.05f, dist * 4.0f, false);
        lookAt(view, target + dir * dist, target);
        C2D_Flush();
        bindDragons(projection);
        lightDragon(dragonLight(dayBlend(now)), plain);
        submitEgg(app, g_egg, d, *egg, view, {0, 0, 0});
        end3D();
        return;
    }
    // Standing at ease: the idle clip (not the rest pose, whose wings are spread), blinking.
    static DenActor show;
    static u32 showId = 0;
    static int showForm = -1;
    static ClipId showClip = ClipId::Idle;
    const int form = growthFor(d.stage, stageProgress(d, now)).form;
    const int* clips = clipIndex(form);
    if (g_animsOk && (showId != d.id || showForm != form || showClip != clip)) {
        show = DenActor{};
        int index = clips[static_cast<int>(clip)];
        if (index < 0) index = clips[static_cast<int>(ClipId::Idle)];
        show.anim.play(index, 0.0f, true);
        showId = d.id;
        showForm = form;
        showClip = clip;
    }
    if (g_animsOk) {
        show.anim.update(g_anims, app.dt, nullptr, 0);
        show.eyes.update(0.0f, app.dt);
    }
    if (!pose(app, d, g_animsOk ? &show : nullptr, now, 0, g_posed)) return;
    C3D_Mtx model;
    modelMatrix(g_posed, model);
    const Vec3 hips = apply(model, g_posed.poseMat[0].translation());
    const float radius = g_posed.cache->radius * g_posed.size;
    const Vec3 target{hips.x, hips.y, hips.z + radius * 0.25f};
    const float dist = radius * 1.05f / std::tan(kFovY * 0.5f);
    Mtx_PerspTilt(&projection, kFovY, C3D_AspectRatioTop, 0.05f, dist * 4.0f, false);
    lookAt(view, target + dir * dist, target);
    C2D_Flush();
    bindDragons(projection);
    lightDragon(dragonLight(dayBlend(now)), plain);
    submit(app, g_posed, view, model);
    end3D();
}

void drawPair(App& app, const Dragon& a, const DenActor& actorA, const Dragon& b, const DenActor& actorB, s64 now) {
    if (!g_ready) return;
    ++g_frame;
    const Dragon* ds[2] = {&a, &b};
    const DenActor* as[2] = {&actorA, &actorB};
    // Frame both: their middle, wide enough for both bodies.
    Vec2 mid{0, 0};
    float reach = 0;
    for (int i = 0; i < 2; ++i) {
        mid.x += as[i]->behavior.pos.x * 0.5f;
        mid.y += as[i]->behavior.pos.y * 0.5f;
    }
    for (int i = 0; i < 2; ++i) {
        const Cache* c = cacheFor(*ds[i], now, 0);
        if (!c) return;
        reach = std::fmax(reach, std::hypot(as[i]->behavior.pos.x - mid.x, as[i]->behavior.pos.y - mid.y) +
                                     viewRadius(*c, sizeScale(ds[i]->genome)) * 0.8f);
    }
    C3D_Mtx projection, view, model;
    const Vec3 target{mid.x, mid.y, reach * 0.45f};
    const float dist = reach / std::tan(kFovY * 0.5f) * 0.95f;
    Mtx_PerspTilt(&projection, kFovY, C3D_AspectRatioTop, 0.05f, dist * 4.0f, false);
    lookAt(view, target + normalize(Vec3{-0.2f, -0.95f, 0.3f}) * dist, target);
    C2D_Flush();
    bindDragons(projection);
    const float plain[3] = {1, 1, 1};
    lightDragon(dragonLight(dayBlend(now)), plain);
    for (int i = 0; i < 2; ++i) {
        if (!pose(app, *ds[i], as[i], now, 0, g_posed)) continue;
        modelMatrix(g_posed, model);
        submit(app, g_posed, view, model);
        if (g_posed.form->headBone >= 0) {
            g_heads[i] = apply(model, g_posed.poseMat[g_posed.form->headBone].translation());
            g_headSet[i] = true;
        }
    }
    g_denView = view;  // for project(): hearts over their heads
    g_denViewSet = true;
    end3D();
}

void poseAhead(App& app, const DenDragon* dragons, int count, s64 now) {
    if (!g_ready) return;
    g_aheadAt = app.t;
    for (int i = 0; i < kDenShown; ++i) {
        Ahead& a = g_ahead[i];
        a.ok = false;
        if (i >= count || dragons[i].dragon->stage == Stage::Egg) continue;
        a.dragon = dragons[i].dragon;
        a.actor = dragons[i].actor;
        a.lod = i == 0 ? 0 : 1;  // as drawDen: the one you care for in full detail
        a.ok = pose(app, *a.dragon, a.actor, now, a.lod, a.posed);
    }
}

void drawCloseUp(App& app, const Dragon& d, const DenActor* actor, const EggMotion* egg, s64 now, CloseUpView mode) {
    if (!g_ready || app.gpuProbe == 3) return;
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
        submitEgg(app, g_egg, d, *egg, view, {0, 0, 0});
        end3D();
        return;
    }
    if (!posedFor(app, d, actor, now, 0, g_posed) || g_posed.form->headBone < 0 || g_posed.form->chestBone < 0) return;
    C3D_Mtx projection, view, model;
    modelMatrix(g_posed, model);
    const Vec3 head = apply(model, g_posed.poseMat[g_posed.form->headBone].translation());
    const Vec3 chest = apply(model, g_posed.poseMat[g_posed.form->chestBone].translation());
    Vec3 target, dir;
    float radius;
    if (mode == CloseUpView::Face || mode == CloseUpView::Feed) {
        // Head and chest, seen from in front of the dragon wherever it stands: the parts you pet.
        // Feeding centres on the mouth, clear of the food row under it.
        target = lerp(head, chest, 0.3f);
        radius = length(head - chest) * 0.75f;
        Vec3 mouth;
        const bool hasMouth = mouthLocal(g_posed, mouth);
        if (g_posed.cache->form == kFormHatchling) {  // its head sits right on its chest: the front of it
            radius = std::fmax(radius, g_posed.cache->radius * g_posed.size * 0.42f);
        } else if (hasMouth) {  // a long neck: the face and the top of the neck, close (it was
            // framed so wide that a big dragon's face was small: Noah, run 3)
            const float span = length(apply(model, mouth) - head);
            if (radius > span * 1.6f) {
                radius = span * 1.6f;
                target = lerp(head, chest, 0.08f);
            }
        }
        if (mode == CloseUpView::Feed && hasMouth) target = lerp(apply(model, mouth), head, 0.25f);
        const float ch = std::cos(g_posed.heading), sh = std::sin(g_posed.heading);
        const Vec3 local = normalize(Vec3{-0.3f, -0.95f, 0.18f});  // front-left of the face
        dir = {local.x * ch - local.y * sh, local.x * sh + local.y * ch, local.z};
    } else {
        // The whole dragon from where you stand (it turns a flank to you while groomed).
        const Vec3 hips = apply(model, g_posed.poseMat[0].translation());
        target = lerp(chest, hips, 0.4f);
        radius = g_posed.cache->radius * g_posed.size * 0.8f;
        dir = normalize(Vec3{-0.2f, -1.0f, 0.45f});
    }
    const float dist = radius / std::tan(kFovY * 0.5f);
    Mtx_PerspTilt(&projection, kFovY, C3D_AspectRatioBot, 0.05f, dist * 4.0f, false);
    const Vec3 eye = target + dir * dist;
    if (autotest::shooting())
        autotest::log("closeup view %d form %d heading %.2f head (%.2f %.2f %.2f) chest (%.2f %.2f %.2f) target (%.2f %.2f %.2f) radius %.2f eye (%.2f %.2f %.2f)",
                      static_cast<int>(mode), g_posed.cache->form, g_posed.heading, head.x, head.y, head.z, chest.x, chest.y,
                      chest.z, target.x, target.y, target.z, radius, eye.x, eye.y, eye.z);
    lookAt(view, eye, target);
    C2D_Flush();
    bindDragons(projection);
    const float plain[3] = {1, 1, 1};
    lightDragon(dragonLight(dayBlend(now)), plain);
    submit(app, g_posed, view, model);
    drawProps(app, view);
    end3D();
    g_close.set = true;
    g_close.view = view;
    g_close.model = model;
    g_close.eye = eye;
    g_close.posed = g_posed;
}

bool pickCloseUp(Vec2 touch, TouchHit& out) {
    if (!g_close.set || !g_close.posed.form) return false;
    const Posed& p = g_close.posed;
    const Form& f = *p.form;
    ScreenCapsule sc[kMaxCapsules];
    int which[kMaxCapsules], n = 0;
    for (int i = 0; i < f.capCount; ++i) {
        const BoneCapsule& c = f.caps[i];
        const Mat34& bm = p.poseMat[c.bone];
        const Vec3 a = apply(g_close.model, transformPoint(bm, {c.cx, c.t0, c.cz}));
        const Vec3 b = apply(g_close.model, transformPoint(bm, {c.cx, c.t1, c.cz}));
        const Vec3 side = transformPoint(bm, {c.cx + c.radius, (c.t0 + c.t1) * 0.5f, c.cz}) -
                          transformPoint(bm, {c.cx, (c.t0 + c.t1) * 0.5f, c.cz});
        float ax, ay, appu, bx, by, bppu;
        if (!projectBottom(g_close.view, a, ax, ay, appu) || !projectBottom(g_close.view, b, bx, by, bppu)) continue;
        const Vec3 va = apply(g_close.view, a), vb = apply(g_close.view, b);
        sc[n] = {{ax, ay}, {bx, by}, length(side) * p.size * (appu + bppu) * 0.5f, -(va.z + vb.z) * 0.5f};
        which[n++] = i;
    }
    float t = 0, across = 0;
    const int k = pickCapsule(sc, n, touch, t, across);
    if (k < 0) return false;
    const BoneCapsule& c = f.caps[which[k]];
    const Mat34& bm = p.poseMat[c.bone];
    // The ray under the stylus, into the bone's own frame, against the capsule's cylinder.
    const Mat34 inv = inverseAffine(bm);
    const Vec3 o = transformPoint(inv, toArmature(p, g_close.eye, true));
    const Vec3 d = transformDir(inv, toArmature(p, rayThrough(touch), false));
    const float ox = o.x - c.cx, oz = o.z - c.cz;
    const float qa = d.x * d.x + d.z * d.z, qb = 2 * (ox * d.x + oz * d.z), qc = ox * ox + oz * oz - c.radius * c.radius;
    const float disc = qb * qb - 4 * qa * qc;
    Vec3 hit;
    if (qa > 1e-8f && disc >= 0) {
        const float s = (-qb - std::sqrt(disc)) / (2 * qa);
        hit = o + d * s;
    } else {  // grazing the rounded end: the axis point, pushed toward the camera
        const Vec3 axisPt{c.cx, c.t0 + (c.t1 - c.t0) * t, c.cz};
        hit = axisPt + normalize(o - axisPt) * c.radius;
    }
    const float y = hit.y < c.t0 ? c.t0 : (hit.y > c.t1 ? c.t1 : hit.y);
    out.local = transformPoint(bm, hit);
    out.outward = normalize(out.local - transformPoint(bm, {c.cx, y, c.cz}));
    out.bone = f.model.skel.name[c.bone];
    out.zone = zoneOf(out.bone, out.outward, t);
    out.region = regionOf(out.bone, out.outward);
    // The scales lie head to tail: spine, neck and head bones point head-ward, tail and leg
    // bones point away from the body.
    const bool outwardBone = std::strncmp(out.bone, "tail", 4) == 0 || std::strncmp(out.bone, "arm", 3) == 0 ||
                             std::strncmp(out.bone, "hand", 4) == 0 || std::strncmp(out.bone, "leg", 3) == 0 ||
                             std::strncmp(out.bone, "foot", 4) == 0;
    const float gx = sc[k].b.x - sc[k].a.x, gy = sc[k].b.y - sc[k].a.y, gl = std::sqrt(gx * gx + gy * gy);
    out.grain = gl > 1e-3f ? Vec2{(outwardBone ? gx : -gx) / gl, (outwardBone ? gy : -gy) / gl} : Vec2{0, 1};
    return true;
}

bool mouthOnCloseUp(Vec2& at) {
    if (!g_close.set || !g_close.posed.form) return false;
    Vec3 mouth;
    float ppu;
    return mouthLocal(g_close.posed, mouth) && projectBottom(g_close.view, apply(g_close.model, mouth), at.x, at.y, ppu);
}

Vec3 closeUpLocal(Vec2 touch) {
    if (!g_close.set || !g_close.posed.form) return {0, 0, 0};
    const Posed& p = g_close.posed;
    const Vec3 head = apply(g_close.model, p.poseMat[p.form->headBone].translation());
    const Vec3 w = g_close.eye + rayThrough(touch) * length(head - g_close.eye);
    return toArmature(p, w, true);
}

bool mouthOf(int i, Vec3& out) {
    if (i < 0 || i >= kDenShown || !g_mouthSet[i]) return false;
    out = g_mouths[i];
    return true;
}

void setProps(const Ball* ball, bool tubOut, Vec2 tubAt, float tubSize) {
    g_ball = ball;
    g_tubOut = tubOut;
    g_tubAt = tubAt;
    g_tubSize = tubSize;
}

void setDenThings(const DenThings* things) { g_things = things; }

namespace {

void releaseForm(Form& f) {
    f.body.release();
    for (GpuMesh& w : f.wings) w.release();
    if (f.skinOk) retireTex(f.skin);
    f.skinOk = false;
    f.ok = false;
    f.model = ModelData{};
}

void releaseForms() {
    for (auto& lods : g_forms)
        for (Form& f : lods) releaseForm(f);
    for (Cache& c : g_caches) c.valid = false;  // every dragon is rebuilt from the new forms
}

Form g_probe[kStyleCount][kFormCount][2];  // probeAllLooks
bool g_probing = false;

bool loadForms(int s) {
    auto one = [s](const char* form, int lod, Form& f) {
        char ecm[64], skin[64];
        std::snprintf(ecm, sizeof(ecm), "%s%s%s.ecm", kStyleDir[s], form, lod ? "_lod1" : "");
        std::snprintf(skin, sizeof(skin), "%s%s%s_skin.t3x", kStyleDir[s], form, lod ? "_lod1" : "");
        return loadForm(ecm, skin, f);
    };
    return one("hatchling", 0, g_forms[kFormHatchling][0]) && one("hatchling", 1, g_forms[kFormHatchling][1]) &&
           one("grown", 0, g_forms[kFormGrown][0]) && one("grown", 1, g_forms[kFormGrown][1]);
}

void styleLight(int s) {
    LightLut_FromFunc(&g_lutToon, s == 1 ? toonRampSoft : (s == 3 ? toonRampHard : toonRamp), 0.0f, true);
    C3D_LightEnvLut(&g_lightEnv, GPU_LUT_D0, GPU_LUTINPUT_LN, true, &g_lutToon);
    LightLut_FromFunc(&g_lutRim, s == 1 ? rimWide : rimBand, 0.0f, false);
    C3D_LightEnvLut(&g_lightEnv, GPU_LUT_FR, GPU_LUTINPUT_NV, false, &g_lutRim);
}

}  // namespace

bool setStyle(int s) {
    if (!g_ready || s < 0 || s >= kStyleCount || s == g_style) return s == g_style;
    releaseForms();
    const bool ok = loadForms(s);
    if (!ok) {  // missing: back to the style that was
        releaseForms();
        loadForms(g_style);
    } else {
        g_style = s;
    }
    g_adultRadius = framingRadius(g_forms[kFormGrown][0].model, 1.0f, kBuildNeutral, nullptr, nullptr);
    for (int f = 0; f < kFormCount; ++f) bindAnims(g_anims, g_forms[f][0].model.skel, g_bind[f]);
    styleLight(g_style);
    return ok;
}

int style() { return g_style; }

bool probeAllLooks() {
    for (auto& forms : g_probe)
        for (auto& lods : forms)
            for (Form& f : lods) releaseForm(f);
    g_probing = !g_probing;
    if (!g_probing) return false;
    for (int s = 0; s < kStyleCount; ++s) {
        if (s == g_style) continue;  // already loaded
        for (int form = 0; form < kFormCount; ++form)
            for (int lod = 0; lod < 2; ++lod) {
                char ecm[64], skin[64];
                const char* name = form == kFormHatchling ? "hatchling" : "grown";
                std::snprintf(ecm, sizeof(ecm), "%s%s%s.ecm", kStyleDir[s], name, lod ? "_lod1" : "");
                std::snprintf(skin, sizeof(skin), "%s%s%s_skin.t3x", kStyleDir[s], name, lod ? "_lod1" : "");
                loadForm(ecm, skin, g_probe[s][form][lod]);
            }
    }
    return true;
}

const char* styleName(int s) {
    static const char* const kNames[kStyleCount] = {"current", "V1 surface", "V2 shape", "V3 bold"};
    return s >= 0 && s < kStyleCount ? kNames[s] : "?";
}

void followInDen(Vec3 at, float weight) {
    g_follow = at;
    g_followWeight = weight < 0 ? 0 : (weight > 1 ? 1 : weight);
}

bool project(Vec3 p, float& x, float& y, float& pixelsPerUnit) {
    return g_denViewSet && projectWith(g_denView, p, x, y, pixelsPerUnit);
}

bool headOf(int i, Vec3& out) {
    if (i < 0 || i >= kDenShown || !g_headSet[i]) return false;
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
