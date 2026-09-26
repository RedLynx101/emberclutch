#include "app/render3d.hpp"

#include "app/hitch.hpp"
#include "app/prefetch.hpp"

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>
#include <tex3ds.h>

#include <algorithm>
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
#include "core/mud.hpp"
#include "core/dragon_mesh.hpp"
#include "core/kinds.hpp"
#include "core/egg.hpp"
#include "core/shell_burst.hpp"
#include "core/prop_mesh.hpp"
#include "core/rig.hpp"
#include "core/people.hpp"
#include "core/place_layout.hpp"
#include "core/static_mesh.hpp"
#include "core/valley.hpp"
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
// DIRT_MAX, DIRT_COLOR; core/mud kDustColor). Mud goes over it in spots (core/mud).
constexpr float kDirtMax = 0.4f;
constexpr int kMudRows = 32;  // the dirt ramp's mud levels

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
    bool asked = false;  // read ahead on the loader's thread? (app/prefetch)
    if (prefetch::take(path, out, asked)) return true;
    if (asked) return false;
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
    static constexpr int kMaxTail = 8;
    s8 tail[kMaxTail] = {};              // tail1, tail2 ... (the wind bends them in flight, D84)
    int tailCount = 0;
    bool ok = false;
};

// What was last built for a dragon on screen: one slot per den dragon plus a spare.
struct Cache {
    bool valid = false;
    u32 lastUsed = 0;  // frame counter, for least-recently-used replacement
    u32 id = 0;
    int form = -1;
    int lod = 0;
    int look = 0;
    int variant = 0;   // a kind's colouring (the rare variant has its own parts)
    bool slit = false;  // a kind's pupils: slit when startled or cross (D77)
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
    float mudShown[kRegionCount] = {};
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

// The egg (romfs:/models/egg.ecm): one skinned shell mesh with two bones (core/egg), and the
// shell in pieces for the hatching, a bone each (core/shell_burst).
struct EggForm {
    ModelData model;
    GpuMesh shell;
    bool ok = false;
    GpuMesh shards;
    ShardShape shapes[kShards];
    bool shardsOk = false;
};

constexpr float kNestFloor = kEggNestFloor;  // eggs sit on the egg nest's straw

DVLB_s* g_dvlb = nullptr;
shaderProgram_s g_program;
int g_locProjection = -1, g_locModelView = -1, g_locBones = -1, g_locPalette = -1, g_locBlob = -1;
C3D_AttrInfo g_attr;       // the body: v5 (dust) from each dragon's buffer
C3D_AttrInfo g_attrFixed;  // everything else: v5 fixed (the wings' dust, or none)
int g_dustFixed = -1;      // that fixed attribute's index
C3D_Tex g_dustRamp;        // 256 x kMudRows RGBA8: (dust, mud) -> the colour to blend toward, alpha how far
C3D_Tex g_cleanSkin;       // 8x8 white: stands in when a form's skin texture is missing
bool g_texOk = false;
DVLB_s* g_staticDvlb = nullptr;
shaderProgram_s g_staticProgram;
int g_locSProjection = -1, g_locSModelView = -1, g_locSBlend = -1, g_locSTint = -1;
C3D_AttrInfo g_staticAttr;
C3D_LightEnv g_lightEnv;
C3D_Light g_light;
C3D_LightLut g_lutToon[3], g_lutRim[2];  // per look: the classic ramp, V1's soft one, V3's hard one; the rims
int g_litLook = -1;                        // the look whose ramps are bound
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
    float lift = 0;             // DenActor::lift
    float morph = 1, blobScale = 1;  // DenActor's: the hatchling taking shape (WP12a)
    Mat34 poseMat[kMaxBones], skin[kMaxBones];
};

// [look][form][lod] (D54: every look is in the game, per dragon; each look's models in its
// own folder). LOD1 draws background dragons in a full den. The classic look loads at start,
// the others one form a frame while the splash plays (loadNextLook), or when first needed.
// After the genome's looks come the new kinds (D77, core/kinds: romfs:/dragons/<kind>/), each
// with its plan's own clips; until DR3 they are shown only through the dev menu.
Form g_forms[kLookSlots][kFormCount][2];
bool g_lookLoaded[kLookSlots] = {};
bool g_lookFailed[kLookSlots] = {};  // a look whose models are missing: drawn classic, not retried
int g_forceLook = -1;  // dev: every dragon in one look (-1: their own)
Cache g_caches[kCacheSlots];
u32 g_frame = 0;
Posed g_posed;
float g_adultRadius = 1;  // framing radius of a neutral adult: the camera's reference size
PartsMesh g_parts;
bool g_ready = false;
AnimLibrary g_anims;
int g_clipIndex[kFormCount][static_cast<int>(ClipId::Count)];
bool g_animsOk = false;
AnimBinding g_bind[kLookSlots][kFormCount];  // LOD1 shares its form's skeleton; each look has its own
// The kinds' plans: romfs:/anims/<plan>.eca, loaded with the first kind that needs one.
AnimLibrary g_planAnims[kMaxPlans];
int g_planClips[kMaxPlans][kFormCount][static_cast<int>(ClipId::Count)];
bool g_planTried[kMaxPlans] = {}, g_planOk[kMaxPlans] = {};
int g_devVariant = 0;  // dev: the colouring the kinds are shown in (3: the rare one)
Room g_room;
EggForm g_egg;      // full detail: the close-up, and a lone egg in the den
EggForm g_eggLod1;  // the den's egg when there's more to draw (tools/blender/egg_model.py --lod 1)
Vec3 g_camTarget;                // smoothed den camera target
float g_camRadius = 0;           // smoothed den framing radius (0: not set yet)
Vec3 g_camEye;                   // last den camera position: where "the player" is
float g_nudgeYaw = 0, g_nudgePitch = 0;  // the circle pad's swing of the den's view (D85)
C3D_Mtx g_denView;               // last den camera, for projecting particles and the egg
bool g_denViewSet = false;
Vec3 g_heads[kDenShown];         // den dragons' heads in the last drawDen (an egg's top)
bool g_headSet[kDenShown] = {};
Vec3 g_mouths[kDenShown];        // ...and their mouths (a carried ball rides there)
bool g_mouthSet[kDenShown] = {};
Vec3 g_backs[kDenShown][2];      // ...and their backs, chest to hips (Starspeckle's glints)
bool g_backSet[kDenShown] = {};

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

// The hatching's burst egg (WP12a): its pieces, and the dragon whose egg it was (its colours).
const ShellBurst* g_burst = nullptr;
const Dragon* g_burstOf = nullptr;

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
GpuMesh g_toyMeshes[kToys], g_bowlFoodMesh, g_decorMeshes[kItems], g_homeRugMesh, g_breedBannerMesh;
float g_followWeight = 0;
bool g_denClose = false;  // photo mode's close framing: the one cared for alone
float g_eye = 0;          // the eye the top screen's 3D is for (setEye): 0 flat
// The eyes' separation, as a share of the distance to what the camera frames, at the slider's
// top: enough to feel the room's depth without straining (the citro3d sample's is ~1/6).
constexpr float kStereoDepth = 0.07f;
// This eye's shift in pixels at depth d (a multiple of the view's focus) is A / d + B
// (setEye measures A and B); the last top view's focus, for project().
float g_shiftA = 0, g_shiftB = 0, g_viewFocus = 1;

// Toon ramp on L.N (signed): plum shadow, a mid band, full light.
float toonRamp(float x, float) { return x < 0.12f ? 0.0f : (x < 0.45f ? 0.62f : 1.0f); }
// Rim on N.V: a thin bright band on the silhouette.
float rimBand(float x, float) { return x < 0.28f ? 1.0f : (x < 0.38f ? 0.35f : 0.0f); }
// Review R5's styles: v1 softer, three-band shading and a wider rim; v3 harder, darker.
float toonRampSoft(float x, float) { return x < 0.05f ? 0.0f : (x < 0.3f ? 0.5f : (x < 0.6f ? 0.82f : 1.0f)); }
float rimWide(float x, float) { return x < 0.33f ? 1.0f : (x < 0.46f ? 0.45f : 0.0f); }
float toonRampHard(float x, float) { return x < 0.2f ? 0.0f : (x < 0.5f ? 0.5f : 1.0f); }

const char* const kLookDir[kLookCount] = {"romfs:/models/", "romfs:/models/v1/", "romfs:/models/v2/",
                                          "romfs:/models/v3/"};

bool isKind(int slot) { return slot >= kLookCount; }
int kindOfSlot(int slot) { return slot - kLookCount; }
int planOfSlot(int slot) { return kindInfo(kindOfSlot(slot)).plan; }

// The clips a look slot animates with: the classic library, or its kind's plan.
bool animsOkFor(int slot) { return isKind(slot) ? g_planOk[planOfSlot(slot)] : g_animsOk; }
const AnimLibrary& libFor(int slot) { return isKind(slot) ? g_planAnims[planOfSlot(slot)] : g_anims; }
const int* clipsForSlot(int slot, int form) {
    const int f = form == kFormHatchling ? kFormHatchling : kFormGrown;
    return isKind(slot) ? g_planClips[planOfSlot(slot)][f] : g_clipIndex[f];
}

void planPath(int plan, char* path, std::size_t cap) {
    std::snprintf(path, cap, "romfs:/anims/%s.eca", planInfo(plan).name);
}

bool loadPlan(int plan) {
    if (g_planTried[plan]) return g_planOk[plan];
    g_planTried[plan] = true;
    std::vector<u8> bytes;
    char path[64];
    planPath(plan, path, sizeof(path));
    g_planOk[plan] = readFile(path, bytes) && loadAnims(bytes.data(), bytes.size(), g_planAnims[plan]) &&
                     resolveClips(g_planAnims[plan], kFormHatchling, g_planClips[plan][kFormHatchling]) &&
                     resolveClips(g_planAnims[plan], kFormGrown, g_planClips[plan][kFormGrown]);
    return g_planOk[plan];
}

bool loadTexture(const char* path, C3D_Tex& tex) {
    std::vector<u8> bytes;  // (through readFile: it may have been read ahead)
    if (!readFile(path, bytes)) return false;
    Tex3DS_Texture t3x = Tex3DS_TextureImport(bytes.data(), bytes.size(), &tex, nullptr, false);
    if (!t3x) return false;
    Tex3DS_TextureFree(t3x);
    C3D_TexSetFilter(&tex, GPU_LINEAR, GPU_LINEAR);
    C3D_TexSetFilterMipmap(&tex, GPU_LINEAR);
    C3D_TexSetWrap(&tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    return true;
}

u32 toByteFast(float v) { return static_cast<u32>(v <= 0 ? 0 : (v >= 1 ? 255 : v * 255.0f + 0.5f)); }

// Texel (x, y) of an 8x8-tiled texture (the GPU's Morton order inside each tile).
std::size_t tiledIndex(int x, int y, int width) {
    const int tile = (y / 8) * (width / 8) + x / 8;
    int m = 0;
    for (int b = 0; b < 3; ++b) m |= (((x >> b) & 1) << (2 * b)) | (((y >> b) & 1) << (2 * b + 1));
    return std::size_t(tile) * 64 + m;
}

// The dirt ramp and the clean stand-in skin (rgba8: no pattern in R, G, B; detail 1 in A).
bool makeTextures() {
    if (!C3D_TexInit(&g_dustRamp, 256, kMudRows, GPU_RGBA8) || !C3D_TexInit(&g_cleanSkin, 8, 8, GPU_RGBA8))
        return false;
    u32* ramp = static_cast<u32*>(g_dustRamp.data);
    for (int y = 0; y < kMudRows; ++y)
        for (int x = 0; x < 256; ++x) {
            Rgb c;
            float a;
            dirtTexel(x / 255.0f, y / static_cast<float>(kMudRows - 1), c, a);
            // the texture's first row is its top: v = 0 (no mud) reads the last
            ramp[tiledIndex(x, kMudRows - 1 - y, 256)] = (u32(c.r) << 24) | (u32(c.g) << 16) | (u32(c.b) << 8) | toByteFast(a);
        }
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
    f.tailCount = 0;
    for (int k = 1; k <= Form::kMaxTail; ++k) {
        char name[8];
        std::snprintf(name, sizeof(name), "tail%d", k);
        const int b = f.model.skel.find(name);
        if (b < 0) break;
        f.tail[f.tailCount++] = static_cast<s8>(b);
    }
    f.ok = true;
    return true;
}

bool loadEgg(const char* path, EggForm& egg) {
    std::vector<u8> bytes;
    if (!readFile(path, bytes) || !loadModel(bytes.data(), bytes.size(), egg.model)) return false;
    const MeshData* shell = egg.model.findMesh(kMeshBody, kGroupBody, 0);
    egg.ok = shell && egg.model.skel.count >= 2 && fillStatic(egg.shell, *shell);
    const MeshData* shards = egg.model.findMesh(kMeshPart, kGroupShards, 0);
    egg.shardsOk = egg.ok && shards && shardShapes(egg.model, egg.shapes) && fillStatic(egg.shards, *shards);
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

int variantOf(const Dragon& d, int look);  // (below, with lookOf)

// Rebuilds the merged part mesh, ground offset and framing when the dragon, its growth or
// its genome changes (growth is slow: days, not frames).
void refreshCache(Cache& c, const Dragon& d, const Growth& gr, int build, int lod, int look) {
    const int variant = variantOf(d, look);
    const bool slit = isKind(look) && moodOf(d) <= Mood::Sulky;
    const bool same = c.valid && c.id == d.id && c.form == gr.form && c.lod == lod && c.look == look &&
                      c.variant == variant && c.slit == slit && std::fabs(c.t - gr.t) < 0.002f && c.sex == d.sex &&
                      std::memcmp(&c.genome, &d.genome, sizeof(Genome)) == 0;
    if (same) return;
    const Form& f = g_forms[look][gr.form][lod];
    c.valid = false;
    if (isKind(look)) {
        const KindInfo& ki = kindInfo(kindOfSlot(look));
        if (!buildKindParts(f.model, variant == ki.rareVariant, ki.rareReplaces, slit, gr.t, build, g_parts)) return;
    } else if (!buildParts(f.model, d.genome, d.sex, gr.t, g_parts)) {
        return;
    }
    if (!fill(c.parts, static_cast<int>(g_parts.pos.size()), g_parts.pos.data(), g_parts.nrm.data(),
              g_parts.skin.data(), g_parts.paint.data(), g_parts.uv.data(), g_parts.indices.data(),
              static_cast<int>(g_parts.indices.size()), g_parts.palette, g_parts.paletteCount))
        return;

    c.radius = framingRadius(f.model, gr.t, build, &c.center, &c.ground);

    c.id = d.id;
    c.form = gr.form;
    c.lod = lod;
    c.look = look;
    c.variant = variant;
    c.slit = slit;
    c.t = gr.t;
    c.genome = d.genome;
    c.sex = d.sex;
    c.valid = true;
}

int buildOf(const Dragon& d) { return d.genome.build < kModelBuilds ? d.genome.build : kBuildNeutral; }

bool loadLook(int look);

// The look a dragon is drawn in: its kind (DR3, D80; or the dev menu's look or kind), loaded
// on the spot if the splash hasn't got to it yet; the classic look if its models are missing.
int lookOf(const Dragon& d) {
    int look = g_forceLook >= 0 ? g_forceLook : kLookCount + (d.kind < kindCount() ? d.kind : 0);
    if (!g_lookLoaded[look]) hitch::mark("kind on the spot");
    if (!g_lookLoaded[look] && !loadLook(look)) look = kLookClassic;
    return look;
}

// The colouring it's drawn in: its own, or the dev menu's while the dev menu shows a kind.
int variantOf(const Dragon& d, int look) {
    if (!isKind(look)) return 0;
    return g_forceLook >= kLookCount ? g_devVariant : (d.variant < kKindVariants ? d.variant : 0);
}

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
    refreshCache(*slot, d, growthFor(d.stage, stageProgress(d, now)), buildOf(d), lod, lookOf(d));
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
    for (int i = 0; i < body->paletteCount; ++i)  // every tail bone (a Ribbontail has eight)
        tail[i] = std::strncmp(m.skel.name[body->palette[i]], "tail", 4) == 0;
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
    const Form& f = g_forms[c->look][c->form][lod];
    const AnimBinding& bind = g_bind[c->look][c->form];
    const AnimLibrary& lib = libFor(c->look);
    updateDust(*c, f, d);
    out.size = kindSize(d);  // DR3: the kind's size (D77: +-50%), a little either way
    out.scale = growthScale(growthFor(d.stage, stageProgress(d, now))) * out.size;
    out.pos = actor ? actor->behavior.pos : Vec2{};
    out.heading = actor ? actor->behavior.heading : 0.0f;
    out.lift = actor ? actor->lift : 0.0f;
    out.morph = actor ? actor->morph : 1.0f;
    out.blobScale = actor ? actor->blobScale : 1.0f;
    BonePose bones[kMaxBones];
    idlePose(f.model, c->t, buildOf(d), bones);
    Quat tailRest[Form::kMaxTail];  // the tail before the clip moves it (the airflow straightens toward it)
    for (int k = 0; k < f.tailCount; ++k) tailRest[k] = bones[f.tail[k]].rot;
    out.root[0] = out.root[1] = 0;
    if (actor && animsOkFor(c->look)) {
        Quat delta[kMaxBones];
        actor->anim.sample(lib, bind, f.model.skel.count, delta, out.root);
        if (isKind(c->look)) {  // the plan's root motion is in its units; the kind's model is baked at its scale
            const float unit = kindInfo(kindOfSlot(c->look)).formScale[c->form];
            out.root[0] *= unit;
            out.root[1] *= unit;
        }
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
            applyLookAt(f.model.skel, bind, bones, local, actor->look * (1.0f - actor->gazeWeight));
        }
        // Hands-on care: it looks at the food you hold out, or leans toward your hand.
        if (actor->gazeWeight > 0.01f)
            applyLookAt(f.model.skel, bind, bones, actor->gazeLocal, actor->gazeWeight);
    }
    // The wind on the tail in flight (D84): the airflow straightens the clip's sway, and the
    // tail swings into a turn and lifts or drops with the dive, a share at each joint (an arc).
    if (actor && f.tailCount > 0 &&
        (actor->tailStraight > 0.01f || std::fabs(actor->tailYaw) > 0.005f || std::fabs(actor->tailPitch) > 0.005f)) {
        const Quat bend = mul(quatAxisAngle({0, 0, 1}, actor->tailYaw / f.tailCount),
                              quatAxisAngle({1, 0, 0}, actor->tailPitch / f.tailCount));
        for (int k = 0; k < f.tailCount; ++k) {
            BonePose& b = bones[f.tail[k]];
            if (actor->tailStraight > 0.01f) b.rot = nlerp(b.rot, tailRest[k], actor->tailStraight);
            const Quat& rest = bind.rest[f.tail[k]];
            b.rot = mul(b.rot, mul(mul(conjugate(rest), bend), rest));
        }
    }
    if (actor && f.jawBone >= 0 && actor->jawOpen > 0.01f) {  // opening for the food (a smaller bite, run 15)
        const Quat q = quatFromPitchYawRoll(-actor->jawOpen * 20.0f * kDegToRad, 0, 0);
        const Quat& rest = bind.rest[f.jawBone];
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
    const float s = p.size * p.blobScale;  // a hatchling's blob swells from its feet
    Mtx_Scale(&out, s, s, s);
    Mtx_Translate(&out, 0, 0, -p.ground, true);
}

Vec3 apply(const C3D_Mtx& m, Vec3 v) {
    return {m.r[0].x * v.x + m.r[0].y * v.y + m.r[0].z * v.z + m.r[0].w,
            m.r[1].x * v.x + m.r[1].y * v.y + m.r[1].z * v.z + m.r[1].w,
            m.r[2].x * v.x + m.r[2].y * v.y + m.r[2].z * v.z + m.r[2].w};
}

void dragonPattern(u8 pattern, Rgb color, bool veins = false);

// The dragon colour chain (architecture §4), texture 0 = the form's skin (R stripes, G
// spots, B dapple, A scale detail), texture 1 = the dust ramp read at the dust stream:
//   0: albedo = vertex colour, patterned (dragonPattern sets it per dragon)
//   1: dirty: toward the dirt ramp's colour (dust, and mud in spots) by its alpha
//   2: x scale detail
//   3: lit: x (ambient + toon), alpha = rim (Fresnel)
//   4: + vertex colour x emissive (vertex alpha): heartglow, eye glints
//   5: + warm rim colour (per dragon, lightDragon) x rim; opaque
void setupTexEnv() {
    C3D_TexEnv* env = C3D_GetTexEnv(1);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE1, GPU_PREVIOUS, GPU_TEXTURE1);
    C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_ALPHA);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_INTERPOLATE);
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
// colour in by it (Runes, until Alpha 2 draws them, and Solid show no pattern). The wild look
// (the V3 models) has no pattern: its veins (skin B) glow after the light, in place of the rim.
void dragonPattern(u8 pattern, Rgb color, bool veins) {
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    if (veins) {
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

// Stage 0 (and 5) for a kind's variant (D77): its pattern channel blended in the pattern
// colour, and its glow channel (the rare variant's marks) glowing after the light.
void kindPattern(int pattern, int glow, Rgb patternColour, Rgb glowColour) {
    static constexpr u8 kPatternOf[3] = {kPatternStripes, kPatternSpots, kPatternDapple};
    dragonPattern(pattern >= 0 && pattern < 3 ? kPatternOf[pattern] : static_cast<u8>(kPatternSolid), patternColour,
                  false);
    // Stage 5 set every time (a glowing dragon before this one leaves its own there): the warm
    // rim (lightDragon has set its colour), or the glow channel lighting up in the glow colour.
    C3D_TexEnv* env = C3D_GetTexEnv(5);
    if (glow < 0 || glow > 2) {
        C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, GPU_PREVIOUS, GPU_PREVIOUS);
        C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_ALPHA, GPU_TEVOP_RGB_SRC_COLOR);
        C3D_TexEnvFunc(env, C3D_RGB, GPU_MULTIPLY_ADD);
        return;
    }
    static constexpr GPU_TEVOP_RGB kChannel[3] = {GPU_TEVOP_RGB_SRC_R, GPU_TEVOP_RGB_SRC_G, GPU_TEVOP_RGB_SRC_B};
    C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, GPU_CONSTANT, GPU_PREVIOUS);
    C3D_TexEnvOpRgb(env, kChannel[glow], GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_MULTIPLY_ADD);
    C3D_TexEnvColor(env, 0xFF000000u | (u32(glowColour.b) << 16) | (u32(glowColour.g) << 8) | glowColour.r);
}

// Binds a form's skin (or the clean stand-in) and the dust ramp.
void bindSkin(const Form* f) {
    C3D_TexBind(0, f && f->skinOk ? const_cast<C3D_Tex*>(&f->skin) : &g_cleanSkin);
    C3D_TexBind(1, &g_dustRamp);
}

u8 toByte(float v) { return static_cast<u8>(v <= 0 ? 0 : (v >= 1 ? 255 : v * 255.0f + 0.5f)); }

// The look's colours on top of the genome's (D54). The wild look is its element showing
// through the cracks: the veins (the pattern colour), the eyes and the wings glow in the base
// element's light (allele A, as the body's colour) over scales that suit it: the Ember's are
// dark, embers underneath (V3); Tide's deep-sea dark with its light; Gale's storm-slate with
// lightning; Grove's dark bark with moss glow; Frost's pale crystal lit ice-blue; Lumen's
// night blue with starlight.
void lookPalette(int look, const Genome& g, Rgb pal[kPalCount]) {
    if (look != kLookWild) return;
    auto scale = [](Rgb c, float k, int add) {
        return Rgb{toByte(c.r * k / 255.0f + add / 255.0f), toByte(c.g * k / 255.0f + add / 255.0f),
                   toByte(c.b * k / 255.0f + add / 255.0f)};
    };
    auto blend = [](Rgb a, Rgb b, float t) {
        return Rgb{toByte((a.r + (b.r - a.r) * t) / 255.0f), toByte((a.g + (b.g - a.g) * t) / 255.0f),
                   toByte((a.b + (b.b - a.b) * t) / 255.0f)};
    };
    static constexpr Rgb kLight[kElementCount] = {
        {255, 140, 40},   // Ember: fire
        {70, 225, 235},   // Tide: deep-sea light
        {205, 240, 255},  // Gale: lightning
        {140, 245, 90},   // Grove: moss glow
        {95, 175, 255},   // Frost: ice-blue (deep enough to read on its pale scales)
        {255, 236, 170},  // Lumen: starlight
    };
    static constexpr Rgb kScales[kElementCount] = {
        {0, 0, 0},      // Ember: its own base colour, darkened (V3)
        {12, 30, 52},   // Tide: deep sea
        {38, 44, 62},   // Gale: storm slate
        {34, 30, 20},   // Grove: bark
        {226, 222, 240},// Frost: pale crystal
        {22, 20, 58},   // Lumen: night
    };
    const int e = g.elementA % kElementCount;
    const Rgb glow = kLight[e];
    const Rgb base = e == 0 ? scale(pal[kPalBase], 0.2f, 8) : blend(kScales[e], pal[kPalBase], 0.12f);
    pal[kPalAccent] = blend(base, pal[kPalAccent], e == 4 ? 0.3f : 0.12f);
    pal[kPalBase] = base;
    pal[kPalHorn] = e == 4 ? Rgb{190, 215, 245} : Rgb{33, 23, 26};
    pal[kPalIris] = glow;
    pal[kPalPattern] = scale(glow, 1.1f, 0);
    pal[kPalMembrane] = e == 4 ? blend(base, glow, 0.35f) : scale(glow, 0.62f, 0);
    pal[kPalGlow] = glow;
}

// The look's shading ramps: V1 (Pebbleback) softer with three bands and a wider rim, the
// wild look (V3) harder; bound only when the look changes between draws.
void lookShading(int look) {
    if (look == g_litLook) return;
    g_litLook = look;
    const bool soft = look == kLookPebbleback || isKind(look);  // the kinds: the storybook look (D75)
    C3D_LightEnvLut(&g_lightEnv, GPU_LUT_D0, GPU_LUTINPUT_LN, true, &g_lutToon[soft ? 1 : (look == kLookWild ? 2 : 0)]);
    C3D_LightEnvLut(&g_lightEnv, GPU_LUT_FR, GPU_LUTINPUT_NV, false, &g_lutRim[soft ? 1 : 0]);
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
        for (float& s : c.mudShown) s = -1.0f;
    }
    if (!c.dust) return;
    bool changed = false;
    for (int r = 0; r < kRegionCount; ++r)
        changed |= std::fabs(d.dirt[r] - c.dustShown[r]) > 0.5f || std::fabs(d.mud[r] - c.mudShown[r]) > 0.5f;
    if (!changed) return;
    for (int r = 0; r < kRegionCount; ++r) c.dustShown[r] = d.dirt[r], c.mudShown[r] = d.mud[r];
    for (int v = 0; v < body->vertexCount; ++v) {
        u8* o = c.dust + std::size_t(v) * 4;
        const int region = body->region[v];
        o[0] = static_cast<u8>(dustValue(d, region) + 0.5f);
        const float mud = region < kRegionCount && d.mud[region] > 0 ? d.mud[region] / 100.0f * mudSpots(body->pos[v]) : 0;
        o[1] = toByte(mud);
        o[2] = o[3] = 0;
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
    g_locBlob = shaderInstanceGetUniformLocation(g_program.vertexShader, "blob");

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
    LightLut_FromFunc(&g_lutToon[0], toonRamp, 0.0f, true);
    LightLut_FromFunc(&g_lutToon[1], toonRampSoft, 0.0f, true);
    LightLut_FromFunc(&g_lutToon[2], toonRampHard, 0.0f, true);
    LightLut_FromFunc(&g_lutRim[0], rimBand, 0.0f, false);
    LightLut_FromFunc(&g_lutRim[1], rimWide, 0.0f, false);
    g_litLook = -1;
    lookShading(kLookClassic);
    C3D_LightEnvFresnel(&g_lightEnv, GPU_SEC_ALPHA_FRESNEL);
    C3D_LightInit(&g_light, &g_lightEnv);
    C3D_LightDiffuse(&g_light, 0, 0, 0);
    C3D_LightSpecular0(&g_light, 0.57f, 0.62f, 0.53f);  // lit = ambient + this: warm white
    C3D_LightSpecular1(&g_light, 0, 0, 0);
    C3D_FVec lightDir = FVec4_New(-0.45f, 0.8f, 0.4f, 0.0f);  // view space, directional (w = 0)
    C3D_LightPosition(&g_light, &lightDir);

    g_ready = g_texOk && loadLook(kLookClassic);
    if (g_ready) g_adultRadius = framingRadius(g_forms[kLookClassic][kFormGrown][0].model, 1.0f, kBuildNeutral, nullptr, nullptr);
    if (g_ready) {
        std::vector<u8> bytes;
        g_animsOk = readFile("romfs:/anims/dragon.eca", bytes) && loadAnims(bytes.data(), bytes.size(), g_anims) &&
                    resolveClips(g_anims, kFormHatchling, g_clipIndex[kFormHatchling]) &&
                    resolveClips(g_anims, kFormGrown, g_clipIndex[kFormGrown]);
        for (int f = 0; f < kFormCount; ++f) bindAnims(g_anims, g_forms[kLookClassic][f][0].model.skel, g_bind[kLookClassic][f]);
        loadRoom("romfs:/models/den.esm");  // without it, dragons stand on the 2D backdrop
        loadEgg("romfs:/models/egg.ecm", g_egg);  // without it, eggs stay 2D
        loadEgg("romfs:/models/egg_lod1.ecm", g_eggLod1);
    }
    return g_ready;
}

void frameBegun() { bury(); }

void shutdown() {
    releaseValley();
    for (auto& forms : g_forms)
        for (auto& lods : forms)
            for (Form& f : lods) {
                f.body.release();
                for (GpuMesh& w : f.wings) w.release();
                if (f.skinOk) retireTex(f.skin);
                f.skinOk = false;
            }
    for (bool& l : g_lookLoaded) l = false;
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
    g_breedBannerMesh.release();
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
    C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locBlob, 0, 0, 0, 1);  // no blob: every dragon as it is
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
    const int look = p.cache->look;
    lookShading(look);
    Rgb pal[kPalCount];
    const Genome shown = (d.genome.rareFlags & kRareIridescent) ? shimmer(d.genome, app.t + d.id * 0.37f) : d.genome;
    if (isKind(look)) {
        kindPalette(kindOfSlot(look), p.cache->variant, d.id, pal);
    } else {
        dragonPalette(shown, pal);
        lookPalette(look, shown, pal);
        rarePalette(d.genome.rareFlags, pal);
    }
    const float glow = 0.55f + 0.45f * heartglowLevel(d, app.t);  // the heartglow pulses with mood
    pal[kPalGlow] = {static_cast<u8>(pal[kPalGlow].r * glow), static_cast<u8>(pal[kPalGlow].g * glow),
                     static_cast<u8>(pal[kPalGlow].b * glow)};
    for (int i = 0; i < kPalCount; ++i)
        C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locPalette + i, pal[i].r / 255.0f, pal[i].g / 255.0f, pal[i].b / 255.0f,
                      1.0f);
    bindSkin(p.form);
    if (isKind(look)) {
        const KindVariant& v = kindInfo(kindOfSlot(look)).variants[p.cache->variant];
        kindPattern(v.patternChannel, v.glowChannel, pal[kPalPattern], pal[kPalGlow]);
    } else {
        dragonPattern(d.genome.pattern, pal[kPalPattern], look == kLookWild);
    }
    const bool blob = p.morph != 1.0f && p.form->chestBone >= 0;
    if (blob) {  // taking shape out of a white blob round its chest (dragon.v.pica)
        const Vec3 c = p.poseMat[p.form->chestBone].translation();
        C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locBlob, c.x, c.y, c.z, p.morph);
    }
    drawBody(app, p.form->body, p.skin, *p.cache);
    drawMesh(app, p.cache->parts, p.skin);
    const MeshData* wings = isKind(look) ? kindWings(p.form->model, p.cache->variant ==
                                                     kindInfo(kindOfSlot(look)).rareVariant)
                                         : selectWings(p.form->model, d.genome);
    if (wings)
        drawMesh(app, p.form->wings[wings->variant], p.skin, dustValue(d, kRegionWings));
    if (blob) C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locBlob, 0, 0, 0, 1);
}

// An egg's palette with each slot's glow in the alpha (`boost` scales the pulse), on the
// skin's clean corner: no pattern, no dust.
void eggColours(App& app, const Dragon& d, float boost) {
    lookShading(kLookClassic);
    Rgb pal[kPalCount];
    float glow[kPalCount];
    eggPalette(d, (0.85f + 0.15f * std::sin(app.t * 2.2f)) * boost, pal, glow, app.t);
    for (int i = 0; i < kPalCount; ++i)
        C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locPalette + i, pal[i].r / 255.0f, pal[i].g / 255.0f, pal[i].b / 255.0f,
                      glow[i]);
    bindSkin(nullptr);
    dragonPattern(kPatternSolid, {0, 0, 0});
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
    eggColours(app, d, 1.0f + 1.4f * motion.glowBoost);  // the hatching turns its light up
    drawMesh(app, egg.shell, skin);
}

// The burst egg's pieces (WP12a), all in one draw: each bone is its piece's transform (den
// space) times the bone's inverse rest.
void submitShards(App& app, const EggForm& egg, const C3D_Mtx& view) {
    if (!g_burst || !g_burst->active || !g_burstOf || !egg.shardsOk) return;
    const MeshData* mesh = egg.model.findMesh(kMeshPart, kGroupShards, 0);
    if (!mesh) return;
    Mat34 skin[kMaxBones];
    for (int i = 0; i < kShards; ++i) {
        const int bone = mesh->palette[i];
        skin[bone] = mul(g_burst->transform(i), egg.model.skel.invRest[bone]);
    }
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_locModelView, &view);  // the pieces are in den space
    eggColours(app, *g_burstOf, 1.0f);
    drawMesh(app, egg.shards, skin);
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

// The top screen's projection, per eye when the 3D slider is up (WP11e): zero parallax a
// little in front of the point it frames (`focus` away), so the dragons stand just behind
// the screen with the room further back, and the interface (citro2d, flat) on it.
void topProjection(C3D_Mtx& p, float near, float far, float focus) {
    g_viewFocus = focus;
    if (g_eye == 0) {
        Mtx_PerspTilt(&p, kFovY, C3D_AspectRatioTop, near, far, false);
        return;
    }
    Mtx_PerspStereoTilt(&p, kFovY, C3D_AspectRatioTop, near, far, g_eye * focus * kStereoDepth, focus * 0.9f, false);
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
    const int bannerSpot = decorSpot(ItemKind::Banner);
    for (int s = 0; s < kDecorSpots; ++s) {
        if (s == bannerSpot && t.breedBanner) {  // a breed's banner takes the spot
            if (!g_breedBannerMesh.vbo && !uploadProp(g_breedBannerMesh, breedBannerMesh())) continue;
            setLook(t.breedLook, t.breedLook.glow * (0.3f + 0.7f * night));
            const DecorPlace p = decorPlace(s);
            modelView(view, placeMatrix(p.at, p.yaw, 1.0f, p.scale));
            drawMesh(app, g_breedBannerMesh, identity);
            continue;
        }
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
    lookShading(kLookClassic);  // the props' own shading, whatever look the last dragon had
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
    static const DenThings kBare{};  // scenes that set nothing still have the den's own rug
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
    x = kTopW * 0.5f + v.x * ppu + eyeShift(-v.z / g_viewFocus);  // at its own depth in 3D (WP11e)
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
    float rad[kDenShown] = {};
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
            r = viewRadius(*c, kindSize(d));
            at[i] = dragons[i].actor ? dragons[i].actor->behavior.pos : Vec2{};
        }
        maxRadius = std::fmax(maxRadius, r);
        rad[i] = r;
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
    if (g_denClose && drawn[0]) {  // photo mode, close: the one you care for fills the picture
        mid = at[0];
        radius = rad[0] * 0.8f;
    }
    // Feet land low on the screen (close up, a little higher: above the photo's name plate).
    const Vec3 want{mid.x, mid.y, radius * (g_denClose ? 0.4f : 0.62f)};
    const float k = g_camRadius > 0 ? std::fmin(1.0f, app.dt * 2.5f) : 1.0f;
    g_camTarget = lerp(g_camTarget, want, k);
    g_camRadius += (radius - g_camRadius) * k;

    Vec3 dir = normalize(Vec3{-0.35f, -0.9f, 0.32f});  // tools/blender/den_model.py CAM_DIR
    if (g_nudgeYaw != 0 || g_nudgePitch != 0) {  // swung round the dragons and tilted (D85)
        const float c = std::cos(g_nudgeYaw), s = std::sin(g_nudgeYaw);
        dir = normalize(Vec3{dir.x * c - dir.y * s, dir.x * s + dir.y * c, dir.z + g_nudgePitch});
    }
    const float dist = g_camRadius / std::tan(kFovY * 0.5f) * 0.95f;
    C3D_Mtx projection, view, model;
    topProjection(projection, 0.25f, std::fmax(dist * 4.0f, 70.0f), dist);
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
    for (int i = 0; i < kDenShown; ++i) g_headSet[i] = g_mouthSet[i] = g_backSet[i] = false;
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
        if (g_posed.form->headBone >= 0) {
            g_heads[i] = apply(model, g_posed.poseMat[g_posed.form->headBone].translation());
            g_headSet[i] = true;
        }
        if (g_posed.form->chestBone >= 0) {  // the top of the back: the spine's joints, lifted to the skin
            const float lift = 0.35f * g_posed.scale;
            g_backs[i][0] = apply(model, g_posed.poseMat[g_posed.form->chestBone].translation()) + Vec3{0, 0, lift};
            g_backs[i][1] = apply(model, g_posed.poseMat[0].translation()) + Vec3{0, 0, lift};
            g_backSet[i] = true;
        }
        Vec3 mouth;
        if (mouthLocal(g_posed, mouth)) {
            g_mouths[i] = apply(model, mouth);
            g_mouthSet[i] = true;
        }
    }
    if (g_burst && g_burst->active) {  // the hatching's shell pieces, lit where they fell
        float nest[3];
        localLight(g_burst->ground.nest, blend, nest);
        lightDragon(light, nest);
        submitShards(app, denEgg.shardsOk ? denEgg : g_egg, view);
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
        topProjection(projection, 0.05f, dist * 4.0f, dist);
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
    const int slot = lookOf(d);
    const int* clips = clipsForSlot(slot, form);
    const bool animsOk = animsOkFor(slot);
    static int showSlot = -1;
    if (animsOk && (showId != d.id || showForm != form || showClip != clip || showSlot != slot)) {
        show = DenActor{};
        int index = clips[static_cast<int>(clip)];
        if (index < 0) index = clips[static_cast<int>(ClipId::Idle)];
        show.anim.play(index, 0.0f, true);
        showId = d.id;
        showForm = form;
        showClip = clip;
        showSlot = slot;
    }
    if (animsOk) {
        show.anim.update(libFor(slot), app.dt, nullptr, 0);
        show.eyes.update(0.0f, app.dt);
    }
    if (!pose(app, d, animsOk ? &show : nullptr, now, 0, g_posed)) return;
    C3D_Mtx model;
    modelMatrix(g_posed, model);
    const Vec3 hips = apply(model, g_posed.poseMat[0].translation());
    const float radius = g_posed.cache->radius * g_posed.size;
    const Vec3 target{hips.x, hips.y, hips.z + radius * 0.25f};
    const float dist = radius * 1.05f / std::tan(kFovY * 0.5f);
    topProjection(projection, 0.05f, dist * 4.0f, dist);
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
                                     viewRadius(*c, kindSize(*ds[i])) * 0.8f);
    }
    C3D_Mtx projection, view, model;
    const Vec3 target{mid.x, mid.y, reach * 0.45f};
    const float dist = reach / std::tan(kFovY * 0.5f) * 0.95f;
    topProjection(projection, 0.05f, dist * 4.0f, dist);
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
    submitShards(app, g_egg, view);  // the hatching: its shell pieces round it
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

// A form's files: its model and its skin (form order: hatchling LOD0, LOD1, grown LOD0, LOD1).
void formPaths(int look, int k, char* ecm, char* skin, std::size_t cap) {
    const int form = k < 2 ? kFormHatchling : kFormGrown, lod = k % 2;
    char dir[48];
    const char* name = form == kFormHatchling ? "hatchling" : "grown";
    if (isKind(look))
        std::snprintf(dir, sizeof(dir), "romfs:/dragons/%s/", kindInfo(kindOfSlot(look)).name);
    else
        std::snprintf(dir, sizeof(dir), "%s", kLookDir[look]);
    std::snprintf(ecm, cap, "%s%s%s.ecm", dir, name, lod ? "_lod1" : "");
    std::snprintf(skin, cap, "%s%s%s_skin.t3x", dir, name, lod ? "_lod1" : "");
}

// One form of one look: its model and skin.
bool loadLookForm(int look, int k) {
    const int form = k < 2 ? kFormHatchling : kFormGrown, lod = k % 2;
    Form& f = g_forms[look][form][lod];
    if (f.ok) return true;
    char ecm[80], skin[80];
    formPaths(look, k, ecm, skin, sizeof(ecm));
    if (!loadForm(ecm, skin, f)) {
        releaseForm(f);
        return false;
    }
    if (isKind(look)) {  // its plan names the bones on the ground (a wyvern stands on its wings)
        const PlanInfo& plan = planInfo(planOfSlot(look));
        for (int i = 0; i < 4; ++i) f.model.contacts[i] = static_cast<s8>(f.model.skel.find(plan.contacts[i]));
    }
    return true;
}

// A whole look (its four forms and their animation bindings). False if a model is missing.
bool loadLook(int look) {
    if (look < 0 || look >= kLookCount + kindCount() || g_lookFailed[look]) return false;
    if (g_lookLoaded[look]) return true;
    if (isKind(look) && !loadPlan(planOfSlot(look))) {
        g_lookFailed[look] = true;
        return false;
    }
    for (int k = 0; k < 4; ++k)
        if (!loadLookForm(look, k)) {
            g_lookFailed[look] = true;
            return false;
        }
    if (animsOkFor(look))
        for (int f = 0; f < kFormCount; ++f) bindAnims(libFor(look), g_forms[look][f][0].model.skel, g_bind[look][f]);
    g_lookLoaded[look] = true;
    return true;
}

}  // namespace

bool loadNextLook(const SaveData& s) {
    if (!g_ready) return false;
    static bool asked[kLookSlots] = {};
    for (int i = 0; i < s.dragonCount; ++i) {
        const int look = kLookCount + (s.dragons[i].kind < kindCount() ? s.dragons[i].kind : 0);
        if (g_lookLoaded[look] || g_lookFailed[look]) continue;
        const int plan = planOfSlot(look);
        char a[80], b[80];
        if (!asked[look]) {  // every file it needs, read ahead off the main thread (Beta: the loader)
            asked[look] = true;
            if (!g_planTried[plan]) {
                planPath(plan, a, sizeof(a));
                prefetch::want(a);
            }
            for (int k = 0; k < 4; ++k) {
                formPaths(look, k, a, b, sizeof(a));
                prefetch::want(a);
                prefetch::want(b);
            }
        }
        if (!g_planTried[plan]) {  // its clips first, then a form a call, each once it's read
            planPath(plan, a, sizeof(a));
            if (!prefetch::ready(a)) return true;
            hitch::mark("clips");
            if (!loadPlan(plan)) g_lookFailed[look] = true;
            return true;
        }
        for (int k = 0; k < 4; ++k) {
            const int form = k < 2 ? kFormHatchling : kFormGrown;
            if (!g_forms[look][form][k % 2].ok) {
                formPaths(look, k, a, b, sizeof(a));
                if (!prefetch::ready(a) || !prefetch::ready(b)) return true;
                hitch::mark("form");
                if (!loadLookForm(look, k)) g_lookFailed[look] = true;
                return true;
            }
        }
        loadLook(look);  // all four in: bind its animations
        return true;
    }
    return false;
}

void setDenNudge(float yaw, float pitch) {
    g_nudgeYaw = yaw;
    g_nudgePitch = pitch;
}

void setForceLook(int look) {
    g_forceLook = look >= 0 && look < kLookCount ? look : -1;
    for (Cache& c : g_caches) c.valid = false;
}

void setDevKind(int kind, int variant) {
    g_forceLook = kind >= 0 && kind < kindCount() ? kLookCount + kind : -1;
    g_devVariant = variant >= 0 && variant < kKindVariants ? variant : 0;
    for (Cache& c : g_caches) c.valid = false;
}

int devKind() { return g_forceLook >= kLookCount ? g_forceLook - kLookCount : -1; }
int devVariant() { return g_devVariant; }

int forceLook() { return g_forceLook; }

void setDenClose(bool close) { g_denClose = close; }

void setEye(float eye) {
    g_eye = eye;
    g_shiftA = g_shiftB = 0;
    if (eye == 0) return;
    // The per-eye projection against the flat one, at a focus of 1 (the shift scales with it):
    // where a point straight ahead lands at two depths, in the screen's own x (the tilt turns
    // the axes, so which one is found by nudging a point sideways).
    C3D_Mtx flat, eyeM;
    Mtx_PerspTilt(&flat, kFovY, C3D_AspectRatioTop, 0.05f, 10.0f, false);
    Mtx_PerspStereoTilt(&eyeM, kFovY, C3D_AspectRatioTop, 0.05f, 10.0f, eye * kStereoDepth, 0.9f, false);
    auto ndc = [](const C3D_Mtx& m, float x, float z, float out[2]) {
        const C3D_FVec c = Mtx_MultiplyFVec4(&m, FVec4_New(x, 0, z, 1));
        out[0] = c.x / c.w;
        out[1] = c.y / c.w;
    };
    float at[2], side[2];
    ndc(flat, 0, -1, at);
    ndc(flat, 0.1f, -1, side);
    const int axis = std::fabs(side[0] - at[0]) > std::fabs(side[1] - at[1]) ? 0 : 1;
    const float pxPerNdc = 0.1f * (kScreenH * 0.5f / std::tan(kFovY * 0.5f)) / (side[axis] - at[axis]);
    float shift[2];
    for (int k = 0; k < 2; ++k) {
        float f[2], e[2];
        ndc(flat, 0, -(1.0f + k), f);
        ndc(eyeM, 0, -(1.0f + k), e);
        shift[k] = (e[axis] - f[axis]) * pxPerNdc;
    }
    g_shiftA = 2 * (shift[0] - shift[1]);
    g_shiftB = shift[0] - g_shiftA;
}

float eyeShift(float depthOverFocus) {
    return g_eye == 0 ? 0.0f : g_shiftA / std::fmax(0.05f, depthOverFocus) + g_shiftB;
}

void followInDen(Vec3 at, float weight) {
    g_follow = at;
    g_followWeight = weight < 0 ? 0 : (weight > 1 ? 1 : weight);
}

bool project(Vec3 p, float& x, float& y, float& pixelsPerUnit) {
    return g_denViewSet && projectWith(g_denView, p, x, y, pixelsPerUnit);
}

bool backOf(int i, Vec3& chest, Vec3& hips) {
    if (i < 0 || i >= kDenShown || !g_backSet[i]) return false;
    chest = g_backs[i][0];
    hips = g_backs[i][1];
    return true;
}

bool headOf(int i, Vec3& out) {
    if (i < 0 || i >= kDenShown || !g_headSet[i]) return false;
    out = g_heads[i];
    return true;
}

bool roomReady() { return g_room.ok; }
bool eggReady() { return g_egg.ok; }

void setBurst(const ShellBurst* burst, const Dragon* of) {
    g_burst = burst;
    g_burstOf = of;
}

const ShardShape* eggShards() { return g_egg.shardsOk ? g_egg.shapes : nullptr; }

u32 backdrop(s64 now) {
    if (!g_room.ok) return theme::kDenPlum;
    const DayBlend b = dayBlend(now);
    const u8* a = &g_room.scene.backdrop[b.a * 4];
    const u8* c = &g_room.scene.backdrop[b.b * 4];
    auto mixc = [&](int k) { return static_cast<u8>(a[k] + (c[k] - a[k]) * b.t); };
    return theme::rgba(mixc(0), mixc(1), mixc(2));
}

const AnimLibrary* anims() { return g_animsOk ? &g_anims : nullptr; }
const AnimLibrary* animsFor(const Dragon& d) {
    const int slot = g_ready ? lookOf(d) : kLookClassic;
    return animsOkFor(slot) ? &libFor(slot) : nullptr;
}
const int* clipIndexFor(const Dragon& d, int form) { return clipsForSlot(g_ready ? lookOf(d) : kLookClassic, form); }
const int* clipIndex(int form) { return g_clipIndex[form == kFormHatchling ? kFormHatchling : kFormGrown]; }
int lookFor(const Dragon& d) { return g_ready ? lookOf(d) : kLookClassic; }
const ModelData* model(int form, int look) {
    return g_ready && form >= 0 && form < kFormCount && look >= 0 && look < kLookSlots && g_lookLoaded[look]
               ? &g_forms[look][form][0].model
               : nullptr;
}
const AnimBinding* binding(int form, int look) {
    return form >= 0 && form < kFormCount && look >= 0 && look < kLookSlots && g_lookLoaded[look] && animsOkFor(look)
               ? &g_bind[look][form]
               : nullptr;
}


// ---------------------------------------------------------------------- the valley (Beta WP1)
namespace {

// A piece of the valley on the GPU: a tile at one detail level, the islands, or the water.
struct ValleyGpu {
    int tx = -1, ty = -1, lod = -1;
    Vec3* pos = nullptr;
    u8* col = nullptr;
    u16* idx = nullptr;
    int count = 0;  // indices
    u32 used = 0;   // the valley frame it was last drawn in
    void release() {
        retire(pos);
        retire(col);
        retire(idx);
        pos = nullptr;
        col = nullptr;
        idx = nullptr;
        count = 0;
        tx = ty = lod = -1;
    }
};
constexpr int kValleySlots = 64;       // tiles kept built
constexpr int kValleyBuilds = 2;       // tiles built a frame at most (the rest show coarser, or wait)
constexpr float kValleyNear = 0.5f, kValleyFar = 400.0f;  // Beta's valley is 2.3 km: see further
ValleyGpu g_vtiles[kValleySlots];
ValleyGpu g_vextras, g_vwater;
// The flown dragon's shadow (D81, a height tell): a soft disc laid on the ground under it,
// rebuilt each frame into one of two buffers (the GPU may still be drawing last frame's).
struct ShadowGpu {
    Vec3* pos = nullptr;
    u8* col = nullptr;
    u16* idx = nullptr;
};
constexpr int kShadowRim = 16;
ShadowGpu g_vshadow[2];
int g_vshadowFlip = 0;
const Valley* g_valleyOf = nullptr;
u32 g_valleyFrame = 0;
C3D_FogLut g_fogLut;
bool g_fogOk = false;
ValleyStats g_valleyStats;
C3D_Tex g_valleyMapTex;
Tex3DS_SubTexture g_valleyMapSub;
C2D_Image g_valleyMap;
bool g_valleyMapOk = false;

bool uploadValley(ValleyGpu& g, const ValleyMesh& m) {
    if (m.idx.empty()) return false;
    const std::size_t n = m.pos.size();
    g.pos = static_cast<Vec3*>(linearAlloc(n * sizeof(Vec3)));
    g.col = static_cast<u8*>(linearAlloc(n * 4));
    g.idx = static_cast<u16*>(linearAlloc(m.idx.size() * sizeof(u16)));
    if (!g.pos || !g.col || !g.idx) {
        g.release();
        return false;
    }
    std::memcpy(g.pos, m.pos.data(), n * sizeof(Vec3));
    std::memcpy(g.col, m.color.data(), n * 4);
    std::memcpy(g.idx, m.idx.data(), m.idx.size() * sizeof(u16));
    GSPGPU_FlushDataCache(g.pos, n * sizeof(Vec3));
    GSPGPU_FlushDataCache(g.col, n * 4);
    GSPGPU_FlushDataCache(g.idx, m.idx.size() * sizeof(u16));
    g.count = static_cast<int>(m.idx.size());
    return true;
}

void drawValleyGpu(App& app, const ValleyGpu& g) {
    if (!g.count) return;
    C3D_BufInfo* buf = C3D_GetBufInfo();
    BufInfo_Init(buf);
    BufInfo_Add(buf, g.pos, sizeof(Vec3), 1, 0x0);
    BufInfo_Add(buf, g.col, 4, 1, 0x1);
    BufInfo_Add(buf, g.col, 4, 1, 0x2);  // one colour set: no blend between two
    C3D_DrawElements(GPU_TRIANGLES, g.count, C3D_UNSIGNED_SHORT, g.idx);
    app.stats.tris += g.count / 3;
    app.stats.draws += 1;
}

void drawValleyShadow(App& app, const Valley& v, const ValleyView& view) {
    if (view.shadow <= 0.01f || view.shadowRadius <= 0.0f) return;
    g_vshadowFlip ^= 1;
    ShadowGpu& g = g_vshadow[g_vshadowFlip];
    constexpr int n = kShadowRim + 1;
    if (!g.pos) {
        g.pos = static_cast<Vec3*>(linearAlloc(n * sizeof(Vec3)));
        g.col = static_cast<u8*>(linearAlloc(n * 4));
        g.idx = static_cast<u16*>(linearAlloc(kShadowRim * 3 * sizeof(u16)));
        if (!g.pos || !g.col || !g.idx) {
            if (g.pos) linearFree(g.pos);
            if (g.col) linearFree(g.col);
            if (g.idx) linearFree(g.idx);
            g = ShadowGpu{};
            return;
        }
        for (int k = 0; k < kShadowRim; ++k) {  // a fan round the middle
            g.idx[k * 3] = 0;
            g.idx[k * 3 + 1] = static_cast<u16>(1 + k);
            g.idx[k * 3 + 2] = static_cast<u16>(1 + (k + 1) % kShadowRim);
        }
        GSPGPU_FlushDataCache(g.idx, kShadowRim * 3 * sizeof(u16));
    }
    // Each point sits just over the ground (or the water) where it falls: the disc follows slopes.
    auto put = [&](int i, float x, float y, u8 alpha) {
        g.pos[i] = {x, y, std::fmax(v.heightAt(x, y), v.water) + 0.12f};
        u8* c = g.col + i * 4;
        c[0] = 30, c[1] = 20, c[2] = 40, c[3] = alpha;
    };
    put(0, view.shadowAt.x, view.shadowAt.y, static_cast<u8>(255.0f * std::fmin(1.0f, view.shadow)));
    for (int k = 0; k < kShadowRim; ++k) {
        const float a = k * (6.2831853f / kShadowRim);
        put(1 + k, view.shadowAt.x + view.shadowRadius * std::cos(a), view.shadowAt.y + view.shadowRadius * 0.8f * std::sin(a), 0);
    }
    GSPGPU_FlushDataCache(g.pos, n * sizeof(Vec3));
    GSPGPU_FlushDataCache(g.col, n * 4);
    C3D_BufInfo* buf = C3D_GetBufInfo();
    BufInfo_Init(buf);
    BufInfo_Add(buf, g.pos, sizeof(Vec3), 1, 0x0);
    BufInfo_Add(buf, g.col, 4, 1, 0x1);
    BufInfo_Add(buf, g.col, 4, 1, 0x2);
    C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_COLOR);  // under the dragon, hiding nothing
    C3D_DrawElements(GPU_TRIANGLES, kShadowRim * 3, C3D_UNSIGNED_SHORT, g.idx);
    C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
    app.stats.tris += kShadowRim;
    app.stats.draws += 1;
}

// The static program for the valley's ground and things, fogged, lit by the day's tint.
void bindValleyStatic(const C3D_Mtx& projection, const C3D_Mtx& view, Rgb tint) {
    C3D_BindProgram(&g_staticProgram);
    C3D_SetAttrInfo(&g_staticAttr);
    C3D_LightEnvBind(nullptr);
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    for (int i = 1; i < 6; ++i) C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_locSProjection, &projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_locSModelView, &view);  // the valley is modelled in world space
    C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locSBlend, 0, 0, 0, 0);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locSTint, tint.r / 65025.0f, tint.g / 65025.0f, tint.b / 65025.0f, 1.0f / 255.0f);
}

// True if a box can't be seen: every corner is beyond the same side of the view.
bool outsideView(const C3D_Mtx& clip, Vec3 lo, Vec3 hi) {
    int out[5] = {};
    for (int k = 0; k < 8; ++k) {
        const C3D_FVec c =
            Mtx_MultiplyFVec4(&clip, FVec4_New(k & 1 ? hi.x : lo.x, k & 2 ? hi.y : lo.y, k & 4 ? hi.z : lo.z, 1.0f));
        out[0] += c.x < -c.w;
        out[1] += c.x > c.w;
        out[2] += c.y < -c.w;
        out[3] += c.y > c.w;
        out[4] += c.w <= 0.0f;
    }
    for (int p : out)
        if (p == 8) return true;
    return false;
}

// A tile at a level: kept from before, or built now if this frame's building isn't used up
// (else any level of it already built, or nothing yet).
const ValleyGpu* valleyTile(const Valley& v, int tx, int ty, int lod, int& budget) {
    for (ValleyGpu& g : g_vtiles)
        if (g.tx == tx && g.ty == ty && g.lod == lod) {
            g.used = g_valleyFrame;
            return &g;
        }
    if (budget <= 0) {
        for (ValleyGpu& g : g_vtiles)
            if (g.tx == tx && g.ty == ty && g.count) {
                g.used = g_valleyFrame;
                return &g;
            }
        return nullptr;
    }
    ValleyGpu* slot = nullptr;
    for (ValleyGpu& g : g_vtiles) {
        if (g.used == g_valleyFrame && g.count) continue;  // drawn this frame: keep
        if (!slot || !g.count || (slot->count && g.used < slot->used)) slot = &g;
        if (!g.count) break;
    }
    if (!slot) return nullptr;
    --budget;
    ++g_valleyStats.built;
    static ValleyMesh mesh;
    buildValleyTile(v, tx, ty, lod, mesh);
    slot->release();
    if (!uploadValley(*slot, mesh)) return nullptr;
    slot->tx = tx;
    slot->ty = ty;
    slot->lod = lod;
    slot->used = g_valleyFrame;
    return slot;
}

// The valley's places (D85): each a .esm static scene in its own frame (metres, +Y its front,
// three lighting sets baked), read ahead as you come near, built on the GPU closer in and let
// go far off. Its parts: the solid model, the festival lantern's post, the mill's sails (turned
// round their hub), the glows (windows at night) and the lantern's light, lit by the festival.
enum PlaceRole : u8 { kRoleSolid, kRoleLantern, kRoleSails, kRoleGlow, kRoleLight };
struct PlaceGpu {
    struct Part {
        u16* idx = nullptr;
        int count = 0;
        u8 flags = 0, role = kRoleSolid;
    };
    static constexpr int kMaxParts = 8;
    float* pos = nullptr;
    u8* color[kLightSets] = {};
    Part parts[kMaxParts];
    int partCount = 0;
    float reach = 0, low = 0, high = 0;  // how far it spreads round its anchor, and its height
    bool ok = false, failed = false, asked = false;
    void release() {
        retire(pos);
        pos = nullptr;
        for (u8*& c : color) {
            retire(c);
            c = nullptr;
        }
        for (Part& p : parts) {
            retire(p.idx);
            p = Part{};
        }
        partCount = 0;
        ok = asked = false;
    }
};
PlaceGpu g_places[kPlaceCount];
constexpr const char* kPlaceFiles[kPlaceCount] = {
    "romfs:/valley/places/den.esm",       "romfs:/valley/places/market.esm",  "romfs:/valley/places/stone.esm",
    "romfs:/valley/places/sanctuary.esm", "romfs:/valley/places/vault.esm",   "romfs:/valley/places/trailhead.esm",
    "romfs:/valley/places/arena.esm",     "romfs:/valley/places/lake.esm",    "romfs:/valley/places/keeper.esm",
    "romfs:/valley/places/isles.esm",     "romfs:/valley/places/orchard.esm", "romfs:/valley/places/mill.esm",
    "romfs:/valley/places/grotto.esm",    "romfs:/valley/places/ruins.esm"};
constexpr float kPlaceWant = 420.0f, kPlaceLoad = 330.0f, kPlaceDrop = 480.0f;

u8 placeRole(const char* name) {
    if (std::strcmp(name, "lantern_light") == 0) return kRoleLight;
    if (std::strcmp(name, "lantern") == 0) return kRoleLantern;
    if (std::strcmp(name, "sails") == 0) return kRoleSails;
    if (std::strcmp(name, "glow") == 0) return kRoleGlow;
    return kRoleSolid;
}

bool loadPlace(PlaceGpu& g, const char* path) {
    std::vector<u8> bytes;
    static StaticScene s;  // (only while it's built: the GPU keeps its own copy)
    s = StaticScene{};
    if (!readFile(path, bytes) || !loadStaticScene(bytes.data(), bytes.size(), s) || s.sets != kLightSets ||
        s.parts.size() > std::size_t(PlaceGpu::kMaxParts))
        return false;
    g.pos = static_cast<float*>(linearAlloc(sizeof(float) * 3 * s.vertexCount));
    bool ok = g.pos != nullptr;
    for (int k = 0; k < kLightSets && ok; ++k) {
        g.color[k] = static_cast<u8*>(linearAlloc(std::size_t(s.vertexCount) * 4));
        ok = g.color[k] != nullptr;
        if (ok) {
            std::memcpy(g.color[k], s.colors(k), std::size_t(s.vertexCount) * 4);
            GSPGPU_FlushDataCache(g.color[k], std::size_t(s.vertexCount) * 4);
        }
    }
    for (std::size_t i = 0; i < s.parts.size() && ok; ++i) {
        const StaticPart& sp = s.parts[i];
        PlaceGpu::Part& part = g.parts[g.partCount++];
        part.count = sp.indexCount;
        part.flags = sp.flags;
        part.role = placeRole(sp.name);
        part.idx = static_cast<u16*>(linearAlloc(sizeof(u16) * std::max(1, part.count)));
        ok = part.idx != nullptr;
        if (ok) {
            std::memcpy(part.idx, s.indices.data() + sp.firstIndex, sizeof(u16) * part.count);
            GSPGPU_FlushDataCache(part.idx, sizeof(u16) * part.count);
        }
    }
    if (!ok) {
        g.release();
        return false;
    }
    g.reach = g.low = g.high = 0;
    for (int v = 0; v < s.vertexCount; ++v) {
        const Vec3 p = s.pos[v];
        g.pos[v * 3] = p.x;
        g.pos[v * 3 + 1] = p.y;
        g.pos[v * 3 + 2] = p.z;
        g.reach = std::fmax(g.reach, std::fmax(std::fabs(p.x), std::fabs(p.y)));
        g.low = std::fmin(g.low, p.z);
        g.high = std::fmax(g.high, p.z);
    }
    GSPGPU_FlushDataCache(g.pos, sizeof(float) * 3 * s.vertexCount);
    g.ok = true;
    return true;
}

// Reads ahead the places coming into reach, builds one a frame, lets the far ones go.
void streamPlaces(const Valley& v, Vec3 eye) {
    bool built = false;
    for (const ValleyPlaceInfo& p : v.places) {
        if (p.id >= kPlaceCount) continue;
        PlaceGpu& g = g_places[p.id];
        const float d = std::hypot(eye.x - p.at.x, eye.y - p.at.y);
        if (g.ok) {
            if (d > kPlaceDrop) g.release();
            continue;
        }
        if (g.failed || d > kPlaceWant) continue;
        if (!g.asked) {
            prefetch::want(kPlaceFiles[p.id]);
            g.asked = true;
        }
        if (!built && (d < kPlaceLoad || prefetch::ready(kPlaceFiles[p.id]))) {
            built = true;
            if (!loadPlace(g, kPlaceFiles[p.id])) g.failed = true;
        }
    }
}

// A place's frame in the valley: its anchor, turned so its +Y is the way it faces.
C3D_Mtx placeFrame(const ValleyPlaceInfo& p) {
    C3D_Mtx m;
    Mtx_Identity(&m);
    Mtx_Translate(&m, p.at.x, p.at.y, p.at.z, true);
    Mtx_RotateZ(&m, p.heading + 3.14159265f, true);
    return m;
}

// The places' opaque parts (glows = false) or their glows and lit lanterns (after the dragons).
void drawPlaces(App& app, const Valley& v, const ValleyView& view, const C3D_Mtx& viewM, const C3D_Mtx& clip,
                const DayBlend& blend, bool glows) {
    if (glows) {
        C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_COLOR);
        C3D_CullFace(GPU_CULL_NONE);
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ONE, GPU_ZERO, GPU_ONE);
    } else {
        C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
        C3D_CullFace(GPU_CULL_BACK_CCW);
    }
    C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locSBlend, blend.t, 0, 0, 0);
    for (const ValleyPlaceInfo& p : v.places) {
        if (p.id >= kPlaceCount || !g_places[p.id].ok) continue;
        const PlaceGpu& g = g_places[p.id];
        const Vec3 lo{p.at.x - g.reach, p.at.y - g.reach, p.at.z + g.low};
        const Vec3 hi{p.at.x + g.reach, p.at.y + g.reach, p.at.z + g.high};
        if (outsideView(clip, lo, hi) || std::hypot(view.eye.x - p.at.x, view.eye.y - p.at.y) > kValleyFar + g.reach)
            continue;
        const bool lit = (view.lanternsLit >> p.id) & 1u;
        const C3D_Mtx frame = placeFrame(p);
        C3D_Mtx mv;
        Mtx_Multiply(&mv, &viewM, &frame);
        C3D_Mtx sails = mv;
        if (p.id == kPlaceMill) {  // the sails turn slowly round their hub
            const PlaceLayout& L = placeLayout(kPlaceMill);
            Mtx_Translate(&sails, L.hub.x, L.hub.y, L.hub.z, true);
            Mtx_Rotate(&sails, FVec3_New(L.hubAxis.x, L.hubAxis.y, L.hubAxis.z), app.t * 0.7f, true);
            Mtx_Translate(&sails, -L.hub.x, -L.hub.y, -L.hub.z, true);
        }
        C3D_BufInfo* buf = C3D_GetBufInfo();
        BufInfo_Init(buf);
        BufInfo_Add(buf, g.pos, sizeof(float) * 3, 1, 0x0);
        BufInfo_Add(buf, g.color[blend.a], 4, 1, 0x1);
        BufInfo_Add(buf, g.color[blend.b], 4, 1, 0x2);
        for (int i = 0; i < g.partCount; ++i) {
            const PlaceGpu::Part& part = g.parts[i];
            if (((part.flags & kStaticAdditive) != 0) != glows || !part.count) continue;
            if (part.role == kRoleLight && !lit) continue;  // the festival lantern, dark till lit
            const float k = (part.flags & kStaticFlicker) ? flicker(app.t + p.id) : 1.0f;
            C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locSTint, k / 255.0f, k / 255.0f, k / 255.0f, 1.0f / 255.0f);
            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_locSModelView, part.role == kRoleSails ? &sails : &mv);
            C3D_DrawElements(GPU_TRIANGLES, part.count, C3D_UNSIGNED_SHORT, part.idx);
            app.stats.tris += part.count / 3;
            app.stats.draws += 1;
            g_valleyStats.places += part.count / 3;
        }
    }
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_locSModelView, &viewM);
    if (glows)
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA,
                       GPU_ONE_MINUS_SRC_ALPHA);
    C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
}


// The valley's people (Beta WP13): your character and the villagers, loaded the first time
// they're wanted (eight small models), posed from the one clip library, drawn with the dragons'
// program on the skin's clean corner (vertex paint only).
struct PersonForm {
    ModelData model;
    GpuMesh body, eyes, hair[kHairStyles];
    AnimBinding bind;
    int eyesBone = -1;
    bool ok = false, tried = false;
    void release() {
        body.release();
        eyes.release();
        for (GpuMesh& h : hair) h.release();
        ok = tried = false;
    }
};
PersonForm g_personForms[kPeople];
AnimLibrary g_personLib;
bool g_personLibOk = false, g_personLibTried = false;

bool personLibReady() {
    if (!g_personLibTried) {
        g_personLibTried = true;
        std::vector<u8> bytes;
        g_personLibOk = readFile("romfs:/anims/person.eca", bytes) && loadAnims(bytes.data(), bytes.size(), g_personLib);
    }
    return g_personLibOk;
}

PersonForm* personForm(int who) {
    if (who < 0 || who >= kPeople || !personLibReady()) return nullptr;
    PersonForm& f = g_personForms[who];
    if (!f.tried) {
        f.tried = true;
        std::vector<u8> bytes;
        const MeshData* body = nullptr;
        const MeshData* eyes = nullptr;
        if (readFile(personFile(static_cast<Person>(who)), bytes) && loadModel(bytes.data(), bytes.size(), f.model) &&
            (body = f.model.findMesh(kMeshBody, kGroupBody, 0)) && (eyes = f.model.findMesh(kMeshPart, kGroupEyes, 0)) &&
            fillStatic(f.body, *body) && fillStatic(f.eyes, *eyes)) {
            for (int h = 0; h < kHairStyles; ++h)
                if (const MeshData* m = f.model.findMesh(kMeshPart, kGroupHair, static_cast<u8>(h))) fillStatic(f.hair[h], *m);
            bindAnims(g_personLib, f.model.skel, f.bind);
            f.eyesBone = f.model.skel.find("eyes");
            f.ok = true;
        }
    }
    return f.ok ? &f : nullptr;
}

// A person, posed and drawn (after bindDragons and a lightDragon). `frame` (if set) places
// them instead of their spot: the rider on its dragon's seat.
void drawPerson(App& app, const PersonView& p, const C3D_Mtx& viewM, const C3D_Mtx* frame = nullptr) {
    PersonForm* f = personForm(p.form);
    if (!f) return;
    perf::Scope timed(perf::Pose);
    BonePose bones[kMaxBones];
    idlePose(f->model, 0.0f, 0, bones);
    float root[2] = {0, 0};
    if (p.anim && p.anim->clip >= 0) {
        Quat delta[kMaxBones];
        p.anim->sample(g_personLib, f->bind, f->model.skel.count, delta, root);
        applyDeltas(bones, delta, f->model.skel.count);
    }
    if (f->eyesBone >= 0) bones[f->eyesBone].scale.z *= 1.0f - kBlinkSquash * p.blink;
    static Mat34 poseMat[kMaxBones], skin[kMaxBones];
    evaluatePose(f->model.skel, bones, poseMat, skin);
    C3D_Mtx model, mv;
    if (frame) {
        model = *frame;
    } else {
        Mtx_Identity(&model);
        Mtx_Translate(&model, p.at.x, p.at.y, p.at.z + root[1] * p.scale, true);
        Mtx_RotateZ(&model, p.heading, true);
        Mtx_Translate(&model, 0, -root[0] * p.scale, 0, true);  // forward is -Y
        Mtx_Scale(&model, p.scale, p.scale, p.scale);
    }
    Mtx_Multiply(&mv, &viewM, &model);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_locModelView, &mv);
    lookShading(kLookClassic);
    for (int i = 0; i < kPalCount; ++i)
        C3D_FVUnifSet(GPU_VERTEX_SHADER, g_locPalette + i, p.pal[i].r / 255.0f, p.pal[i].g / 255.0f, p.pal[i].b / 255.0f, 1.0f);
    bindSkin(nullptr);
    dragonPattern(kPatternSolid, {0, 0, 0});
    drawMesh(app, f->body, skin);
    drawMesh(app, f->eyes, skin);
    if (p.hair >= 0 && p.hair < kHairStyles) drawMesh(app, f->hair[p.hair], skin);
}

// Where the rider sits on the flown dragon (its plan's seat on its seat bone), as a frame for
// the rider: on the seat, turned and tilted with the dragon.
bool riderFrame(const Posed& d, const ValleyView& view, const C3D_Mtx& dragonModel, Person rider, C3D_Mtx& out) {
    if (!d.cache || !isKind(d.cache->look)) return false;
    const KindInfo& kind = kindInfo(kindOfSlot(d.cache->look));
    const PlanInfo& plan = planInfo(kind.plan);
    const int bone = plan.seatBone ? d.form->model.skel.find(plan.seatBone) : -1;
    if (bone < 0) return false;
    // The plan's seat: an offset (armature axes) from the bone's head at rest, carried by the
    // bone's pose as the skin is.
    const Vec3 head = inverseAffine(d.form->model.skel.invRest[bone]).translation();
    const Vec3 seat = transformPoint(d.skin[bone], head + plan.seat * kind.formScale[d.cache->form]);
    const Vec3 w = apply(dragonModel, seat);
    const Vec3 s = personSeat(rider);
    Mtx_Identity(&out);
    Mtx_Translate(&out, w.x, w.y, w.z, true);
    Mtx_RotateZ(&out, view.heading, true);
    Mtx_RotateX(&out, view.pitch, true);
    Mtx_RotateY(&out, -view.roll, true);
    Mtx_Translate(&out, -s.x, -s.y, -s.z + 0.07f, true);  // a little high: the legs clear a broad back
    return true;
}

// The Market's stall: the egg of the day on its stand (gone once bought) and the day's four
// goods on the stall's mats, each fitted to its spot; a sold-out spot shows a plain crate.
struct GoodsMesh {
    GpuMesh mesh;
    float scale = 1, lift = 0, cx = 0, cy = 0;
};
GoodsMesh g_goods[kItems + 1];
EggMotion g_standEgg;

bool goodsMesh(Item it, GoodsMesh*& out) {
    GoodsMesh& g = g_goods[it < Item::Count ? static_cast<int>(it) : kItems];
    out = &g;
    if (g.mesh.vbo) return true;
    const PropMesh m = stallMesh(it);
    if (m.pos.empty()) return false;
    Vec3 lo = m.pos[0], hi = m.pos[0];
    for (const Vec3& p : m.pos) {
        lo = {std::fmin(lo.x, p.x), std::fmin(lo.y, p.y), std::fmin(lo.z, p.z)};
        hi = {std::fmax(hi.x, p.x), std::fmax(hi.y, p.y), std::fmax(hi.z, p.z)};
    }
    const float wide = std::fmax(hi.x - lo.x, hi.y - lo.y), tall = hi.z - lo.z;
    g.scale = std::fmin(0.46f / std::fmax(wide, 1e-3f), 0.55f / std::fmax(tall, 1e-3f));
    g.lift = -lo.z;
    g.cx = (lo.x + hi.x) * 0.5f;
    g.cy = (lo.y + hi.y) * 0.5f;
    return uploadProp(g.mesh, m);
}

void drawStall(App& app, const Valley& v, const ValleyView& view, const C3D_Mtx& viewM) {
    const ValleyPlaceInfo* market = v.place(kPlaceMarket);
    if (!market || !g_places[kPlaceMarket].ok) return;
    const PlaceLayout& L = placeLayout(kPlaceMarket);
    const float yaw = market->heading + 3.14159265f;
    const Mat34 identity[1] = {Mat34::identity()};
    lookShading(kLookClassic);
    bindSkin(nullptr);
    dragonPattern(kPatternSolid, {0, 0, 0});
    for (int k = 0; k < 4; ++k) {
        GoodsMesh* g = nullptr;
        if (!goodsMesh(view.goods[k], g)) continue;
        const Vec2 at = placeToWorld(*market, {L.goods[k].x, L.goods[k].y});
        C3D_Mtx model = placeMatrix({at.x, at.y, market->at.z + L.goods[k].z}, yaw + 0.25f * (k - 1.5f), 1.0f, g->scale);
        Mtx_Translate(&model, -g->cx, -g->cy, g->lift, true);
        modelView(viewM, model);
        const PropLook look = stallLook(view.goods[k]);
        setLook(look, look.glow * 0.6f);
        drawMesh(app, g->mesh, identity);
    }
    if (view.marketEgg && g_egg.ok) {  // the egg of the day, rocking a little on its straw
        g_standEgg.update(app.dt, 0.0f, app.rng);
        if (g_standEgg.rock < 0.02f) g_standEgg.knock(0.03f, 0);
        const Vec2 at = placeToWorld(*market, {L.eggStand.x, L.eggStand.y});
        Mat34 skin[2];
        eggSkin(g_egg.model, g_standEgg, skin);
        constexpr float kStandEgg = 0.6f;  // the egg's height on the stand, metres
        C3D_Mtx model, mv;
        Mtx_Identity(&model);
        Mtx_Translate(&model, at.x, at.y, market->at.z + L.eggStand.z, true);
        Mtx_RotateZ(&model, yaw + 0.4f * std::sin(app.t * 0.3f), true);
        Mtx_Scale(&model, kStandEgg, kStandEgg, kStandEgg);
        Mtx_Translate(&model, 0, 0, -groundOffset(g_egg.model, skin), true);
        Mtx_Multiply(&mv, &viewM, &model);
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_locModelView, &mv);
        eggColours(app, *view.marketEgg, 1.0f);
        drawMesh(app, g_egg.shell, skin);
    }
}

}  // namespace

void releaseValley() {
    for (ValleyGpu& g : g_vtiles) g.release();
    g_vextras.release();
    g_vwater.release();
    for (PlaceGpu& p : g_places) {
        p.release();
        p.failed = false;
    }
    for (GoodsMesh& g : g_goods) g.mesh.release();
    for (PersonForm& f : g_personForms) f.release();
    g_valleyOf = nullptr;
    if (g_valleyMapOk) {
        retireTex(g_valleyMapTex);
        g_valleyMapOk = false;
    }
}

ValleyStats valleyStats() { return g_valleyStats; }
const AnimLibrary* personAnims() { return personLibReady() ? &g_personLib : nullptr; }

void drawValley(App& app, const ValleyView& view, s64 now) {
    if (!g_ready || !view.valley) return;
    const Valley& v = *view.valley;
    if (g_valleyOf != &v) {
        releaseValley();
        g_valleyOf = &v;
    }
    ++g_valleyFrame;
    g_valleyStats = {};
    perf::Scope timed(perf::Room);
    C3D_Mtx projection, viewM, clip;
    const float focus = std::fmax(4.0f, length(view.at - view.eye));
    topProjection(projection, kValleyNear, kValleyFar, focus);
    lookAt(viewM, view.eye, view.target);
    g_denView = viewM;  // project() works in the valley too
    g_denViewSet = true;
    Mtx_Multiply(&clip, &projection, &viewM);
    if (!g_fogOk) {  // fog thickens from about 130 m to the far plane (the LUT knows the projection)
        FogLut_Exp(&g_fogLut, 1.0f / 280.0f, 2.5f, kValleyNear, kValleyFar);
        g_fogOk = true;
    }
    C2D_Flush();
    C3D_FogGasMode(GPU_FOG, GPU_PLAIN_DENSITY, false);
    C3D_FogColor(u32(view.fog.r) | (u32(view.fog.g) << 8) | (u32(view.fog.b) << 16));
    C3D_FogLutBind(&g_fogLut);
    bindValleyStatic(projection, viewM, view.tint);
    C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
    C3D_CullFace(GPU_CULL_BACK_CCW);
    // The ground round the camera: in view, at a level by distance.
    const int t = v.tiles();
    const float ts = v.tileSize(), reachM = kValleyFar * 0.85f;
    const int cx = static_cast<int>((view.eye.x - v.x0) / ts), cy = static_cast<int>((view.eye.y - v.y0) / ts);
    const int reach = static_cast<int>(reachM / ts) + 1;
    int budget = kValleyBuilds;
    for (int ty = cy - reach; ty <= cy + reach; ++ty)
        for (int tx = cx - reach; tx <= cx + reach; ++tx) {
            if (tx < 0 || ty < 0 || tx >= t || ty >= t) continue;
            const Vec3 lo{v.x0 + tx * ts, v.y0 + ty * ts, v.tileLow[std::size_t(ty) * t + tx] - 12.0f};
            const Vec3 hi{lo.x + ts, lo.y + ts, v.tileHigh[std::size_t(ty) * t + tx]};
            const Vec3 near{std::fmax(lo.x, std::fmin(view.eye.x, hi.x)), std::fmax(lo.y, std::fmin(view.eye.y, hi.y)),
                            std::fmax(lo.z, std::fmin(view.eye.z, hi.z))};
            const float d = length(near - view.eye);
            if (d > reachM || outsideView(clip, lo, hi)) continue;
            if (const ValleyGpu* g = valleyTile(v, tx, ty, valleyLodFor(d), budget)) {
                drawValleyGpu(app, *g);
                ++g_valleyStats.tiles;
                g_valleyStats.ground += g->count / 3;
            }
        }
    // The places, near enough to have been built.
    const DayBlend blend = dayBlend(now);
    streamPlaces(v, view.eye);
    drawPlaces(app, v, view, viewM, clip, blend, false);
    bindValleyStatic(projection, viewM, view.tint);
    // The islands (built once), both faces drawn.
    if (!g_vextras.count) {
        ValleyMesh m;
        buildValleyExtras(v, m);
        uploadValley(g_vextras, m);
    }
    C3D_CullFace(GPU_CULL_NONE);
    drawValleyGpu(app, g_vextras);
    drawValleyShadow(app, v, view);
    // The dragon.
    if (view.dragon && pose(app, *view.dragon, view.actor, now, 0, g_posed)) {
        bindDragons(projection);
        const float plain[3] = {1, 1, 1};
        lightDragon(dragonLight(dayBlend(now)), plain);
        C3D_Mtx model;
        Mtx_Identity(&model);
        constexpr float kPivot = 1.6f;  // it tilts about its middle, not its feet
        Mtx_Translate(&model, view.at.x, view.at.y, view.at.z + kPivot * g_posed.size, true);
        Mtx_RotateZ(&model, view.heading, true);
        Mtx_RotateX(&model, view.pitch, true);
        Mtx_RotateY(&model, -view.roll, true);
        Mtx_Translate(&model, 0, 0, -kPivot * g_posed.size, true);
        Mtx_Scale(&model, g_posed.size, g_posed.size, g_posed.size);
        Mtx_Translate(&model, 0, 0, -g_posed.ground, true);
        submit(app, g_posed, viewM, model);
        if (g_posed.form->headBone >= 0) {
            g_heads[0] = apply(model, g_posed.poseMat[g_posed.form->headBone].translation());
            g_headSet[0] = true;
        }
        // You on its back.
        C3D_Mtx seat;
        const bool seated = view.riderOn && view.peopleCount > 0 && view.people[0].seated &&
                            riderFrame(g_posed, view, model, static_cast<Person>(view.people[0].form), seat);
        if (seated) drawPerson(app, view.people[0], viewM, &seat);
        if (autotest::shooting())
            autotest::log("rider on %d seated %d look %d kind %d seat (%.2f %.2f %.2f) at (%.2f %.2f %.2f)", view.riderOn,
                          seated, g_posed.cache ? g_posed.cache->look : -1,
                          g_posed.cache && isKind(g_posed.cache->look) ? kindOfSlot(g_posed.cache->look) : -1,
                          seat.r[0].w, seat.r[1].w, seat.r[2].w, view.at.x, view.at.y, view.at.z);
    }
    // The people about (you on foot, the villagers), near enough to see.
    if (view.peopleCount > 0) {
        bindDragons(projection);
        const float plain[3] = {1, 1, 1};
        lightDragon(dragonLight(dayBlend(now)), plain);
        for (int i = 0; i < view.peopleCount; ++i) {
            const PersonView& p = view.people[i];
            if (p.seated) continue;
            const float d = std::hypot(p.at.x - view.eye.x, p.at.y - view.eye.y);
            if (d > 70.0f || outsideView(clip, p.at - Vec3{0.8f, 0.8f, 0}, p.at + Vec3{0.8f, 0.8f, 1.8f})) continue;
            drawPerson(app, p, viewM);
        }
    }
    // The Market's stall (the dragons' program, their light).
    if (const ValleyPlaceInfo* market = v.place(kPlaceMarket);
        market && g_places[kPlaceMarket].ok && std::hypot(view.eye.x - market->at.x, view.eye.y - market->at.y) < 140.0f) {
        bindDragons(projection);
        const float plain[3] = {1, 1, 1};
        lightDragon(dragonLight(blend), plain);
        drawStall(app, v, view, viewM);
    }
    // The places' glows: windows and lamps at night, the festival's lit lanterns.
    bindValleyStatic(projection, viewM, view.tint);
    drawPlaces(app, v, view, viewM, clip, blend, true);
    // The water and the waterfall: see-through, over everything, writing no depth.
    if (!g_vwater.count) {
        ValleyMesh m;
        buildValleyWater(v, m);
        uploadValley(g_vwater, m);
    }
    bindValleyStatic(projection, viewM, view.tint);
    C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_COLOR);
    C3D_CullFace(GPU_CULL_NONE);
    drawValleyGpu(app, g_vwater);
    C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
    C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
    end3D();
}

const C2D_Image* valleyMap(const Valley& v) {
    if (g_valleyMapOk) return &g_valleyMap;
    constexpr int kSize = 128;
    if (!C3D_TexInit(&g_valleyMapTex, kSize, kSize, GPU_RGB565)) return nullptr;
    u16* px = static_cast<u16*>(g_valleyMapTex.data);
    for (int y = 0; y < kSize; ++y)
        for (int x = 0; x < kSize; ++x) {
            // The texture's first row is its top: north.
            const int i = x * (v.n - 1) / (kSize - 1), j = (kSize - 1 - y) * (v.n - 1) / (kSize - 1);
            const u8* c = &v.rgb[(std::size_t(j) * v.n + i) * 3];
            const bool wet = v.h[std::size_t(j) * v.n + i] < v.water;
            const int r = wet ? 70 : c[0], g = wet ? 140 : c[1], b = wet ? 178 : c[2];
            px[tiledIndex(x, y, kSize)] = static_cast<u16>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
        }
    C3D_TexFlush(&g_valleyMapTex);
    C3D_TexSetFilter(&g_valleyMapTex, GPU_LINEAR, GPU_LINEAR);
    g_valleyMapSub = {kSize, kSize, 0.0f, 1.0f, 1.0f, 0.0f};
    g_valleyMap = {&g_valleyMapTex, &g_valleyMapSub};
    g_valleyMapOk = true;
    return &g_valleyMap;
}

}  // namespace ec::r3d
