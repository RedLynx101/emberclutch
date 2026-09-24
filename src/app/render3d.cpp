#include "app/render3d.hpp"

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "app/ui_draw.hpp"
#include "core/dragon_mesh.hpp"
#include "core/rig.hpp"
#include "dragon_shbin.h"

namespace ec::r3d {
namespace {

constexpr float kDegToRad = 3.14159265f / 180.0f;
constexpr float kFovY = 38.0f * kDegToRad;

// Matches the attribute loaders below and the shader's inputs v0..v3.
struct GpuVertex {
    float pos[3];
    float nrm[3];
    u8 skin[4];   // bone0, bone1 (palette-local), w0, w1
    u8 paint[4];  // palette A, palette B, mix, emissive
};
static_assert(sizeof(GpuVertex) == 32, "shared with the shader's attribute layout");

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
bool fill(GpuMesh& g, int n, const Vec3* pos, const Vec3* nrm, const u8* skin, const u8* paint, const u16* idx,
          int idxCount, const u8* palette, u8 paletteCount) {
    if (!g.reserve(n, idxCount)) return false;
    for (int v = 0; v < n; ++v) {
        GpuVertex& o = g.vbo[v];
        o.pos[0] = pos[v].x, o.pos[1] = pos[v].y, o.pos[2] = pos[v].z;
        o.nrm[0] = nrm[v].x, o.nrm[1] = nrm[v].y, o.nrm[2] = nrm[v].z;
        std::memcpy(o.skin, skin + std::size_t(v) * 4, 4);
        std::memcpy(o.paint, paint + std::size_t(v) * 4, 4);
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
    return fill(g, m.vertexCount, m.pos.data(), m.nrm.data(), m.skin.data(), m.paint.data(), m.indices.data(),
                static_cast<int>(m.indices.size()), m.palette, m.paletteCount);
}

// A little life until the animation system (WP5): breathing, a slow tail sway.
struct Wiggle {
    const char* bone;
    float x, z, speed, phase;  // degrees, degrees, rad/s, rad
};
constexpr Wiggle kWiggles[] = {
    {"chest", 0.8f, 0, 1.4f, 0}, {"neck1", 1.6f, 0, 1.4f, 0.4f}, {"head", -1.4f, 0, 1.4f, 0.9f},
    {"tail2", 0, 4, 0.8f, 0}, {"tail3", 0, 5, 0.8f, 0.6f}, {"tail4", 0, 7, 0.8f, 1.2f},
    {"wing_arm_L", 0, 2, 1.4f, 0.2f}, {"wing_arm_R", 0, -2, 1.4f, 0.2f},
};
constexpr int kWiggleCount = sizeof(kWiggles) / sizeof(kWiggles[0]);

struct Form {
    ModelData model;
    GpuMesh body;
    GpuMesh wings[kWingsCount];
    int wiggleBone[kWiggleCount];
    int headBone = -1, chestBone = -1;
    bool ok = false;
};

// What was last built for a dragon on screen: one slot per den dragon plus a spare.
struct Cache {
    bool valid = false;
    u32 lastUsed = 0;  // frame counter, for least-recently-used replacement
    u32 id = 0;
    int form = -1;
    float t = -1;
    Genome genome{};
    Sex sex = Sex::Female;
    GpuMesh parts;
    float ground = 0;          // lowest body vertex in the idle pose (armature space)
    Vec3 center{0, 0, 0};      // camera framing, armature space
    float radius = 1;
};

DVLB_s* g_dvlb = nullptr;
shaderProgram_s g_program;
int g_locProjection = -1, g_locModelView = -1, g_locBones = -1, g_locPalette = -1;
C3D_AttrInfo g_attr;
C3D_LightEnv g_lightEnv;
C3D_Light g_light;
C3D_LightLut g_lutToon, g_lutRim;
constexpr int kCacheSlots = 4;

// One dragon posed for this frame (static storage: the matrices are ~3.5 KB).
struct Posed {
    const Form* form = nullptr;
    const Cache* cache = nullptr;
    const Dragon* dragon = nullptr;
    float size = 1;  // genome size scale
    Mat34 poseMat[kMaxBones], skin[kMaxBones];
};

Form g_forms[kFormCount];
Cache g_caches[kCacheSlots];
u32 g_frame = 0;
Posed g_posed;
float g_adultRadius = 1;  // framing radius of a neutral adult: the camera's reference size
PartsMesh g_parts;
bool g_ready = false;

// Toon ramp on L.N (signed): plum shadow, a mid band, full light.
float toonRamp(float x, float) { return x < 0.12f ? 0.0f : (x < 0.45f ? 0.62f : 1.0f); }
// Rim on N.V: a thin bright band on the silhouette.
float rimBand(float x, float) { return x < 0.28f ? 1.0f : (x < 0.38f ? 0.35f : 0.0f); }

bool loadForm(const char* path, Form& f) {
    FILE* file = std::fopen(path, "rb");
    if (!file) return false;
    std::fseek(file, 0, SEEK_END);
    std::vector<u8> bytes(static_cast<std::size_t>(std::ftell(file)));
    std::fseek(file, 0, SEEK_SET);
    const bool read = std::fread(bytes.data(), 1, bytes.size(), file) == bytes.size();
    std::fclose(file);
    if (!read || !loadModel(bytes.data(), bytes.size(), f.model)) return false;
    const MeshData* body = f.model.findMesh(kMeshBody, kGroupBody, 0);
    if (!body || !fillStatic(f.body, *body)) return false;
    for (const MeshData& m : f.model.meshes)
        if (m.kind == kMeshWings && m.variant < kWingsCount && !fillStatic(f.wings[m.variant], m)) return false;
    for (int i = 0; i < kWiggleCount; ++i) f.wiggleBone[i] = f.model.skel.find(kWiggles[i].bone);
    f.headBone = f.model.skel.find("head");
    f.chestBone = f.model.skel.find("chest");
    f.ok = true;
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

void idleMotion(const Form& f, float time, BonePose* pose) {
    for (int i = 0; i < kWiggleCount; ++i) {
        const int b = f.wiggleBone[i];
        if (b < 0) continue;
        const Wiggle& w = kWiggles[i];
        const float s = std::sin(time * w.speed + w.phase);
        pose[b].rot = mul(pose[b].rot, quatFromEulerXYZ(w.x * s * kDegToRad, 0, w.z * s * kDegToRad));
    }
}

// Rebuilds the merged part mesh, ground offset and framing when the dragon, its growth or
// its genome changes (growth is slow: days, not frames).
void refreshCache(Cache& c, const Dragon& d, const Growth& gr, int build) {
    const bool same = c.valid && c.id == d.id && c.form == gr.form && std::fabs(c.t - gr.t) < 0.002f &&
                      c.sex == d.sex && std::memcmp(&c.genome, &d.genome, sizeof(Genome)) == 0;
    if (same) return;
    const Form& f = g_forms[gr.form];
    c.valid = false;
    if (!buildParts(f.model, d.genome, d.sex, gr.t, g_parts)) return;
    if (!fill(c.parts, static_cast<int>(g_parts.pos.size()), g_parts.pos.data(), g_parts.nrm.data(),
              g_parts.skin.data(), g_parts.paint.data(), g_parts.indices.data(),
              static_cast<int>(g_parts.indices.size()), g_parts.palette, g_parts.paletteCount))
        return;

    c.radius = framingRadius(f.model, gr.t, build, &c.center, &c.ground);

    c.id = d.id;
    c.form = gr.form;
    c.t = gr.t;
    c.genome = d.genome;
    c.sex = d.sex;
    c.valid = true;
}

int buildOf(const Dragon& d) { return d.genome.build < kModelBuilds ? d.genome.build : kBuildNeutral; }

// The dragon's cache slot, refreshed if its growth or genome changed. nullptr if the
// dragon has no model yet (an egg) or its parts could not be built.
Cache* cacheFor(const Dragon& d, s64 now) {
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
    refreshCache(*slot, d, growthFor(d.stage, stageProgress(d, now)), buildOf(d));
    slot->lastUsed = g_frame;
    return slot->valid ? slot : nullptr;
}

// Blends a dragon's own framing radius with the adult's, so babies read as small and adults
// as big while a hatchling still fills a good part of the screen.
float viewRadius(const Cache& c, float size) { return (0.8f * c.radius + 0.2f * g_adultRadius) * size; }

bool pose(App& app, const Dragon& d, s64 now, Posed& out) {
    Cache* c = cacheFor(d, now);
    if (!c) return false;
    const Form& f = g_forms[c->form];
    BonePose bones[kMaxBones];
    idlePose(f.model, c->t, buildOf(d), bones);
    idleMotion(f, app.t + (d.id % 7) * 0.9f, bones);  // dragons breathe out of step
    evaluatePose(f.model.skel, bones, out.poseMat, out.skin);
    out.form = &f;
    out.cache = c;
    out.dragon = &d;
    out.size = sizeScale(d.genome);
    return true;
}

// Model matrix: stand at pos, turned by yaw, genome size, feet on the floor.
void modelMatrix(const Posed& p, Vec3 pos, float yaw, C3D_Mtx& out) {
    Mtx_Identity(&out);
    Mtx_Translate(&out, pos.x, pos.y, pos.z, true);
    Mtx_RotateZ(&out, yaw, true);
    Mtx_Scale(&out, p.size, p.size, p.size);
    Mtx_Translate(&out, 0, 0, -p.cache->ground, true);
}

Vec3 apply(const C3D_Mtx& m, Vec3 v) {
    return {m.r[0].x * v.x + m.r[0].y * v.y + m.r[0].z * v.z + m.r[0].w,
            m.r[1].x * v.x + m.r[1].y * v.y + m.r[1].z * v.z + m.r[1].w,
            m.r[2].x * v.x + m.r[2].y * v.y + m.r[2].z * v.z + m.r[2].w};
}

void setupTexEnv() {
    // 0: light = ambient + toon (primary + secondary); alpha = rim (Fresnel)
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_FRAGMENT_PRIMARY_COLOR, GPU_FRAGMENT_SECONDARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_ADD);
    C3D_TexEnvSrc(env, C3D_Alpha, GPU_FRAGMENT_SECONDARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
    // 1: x vertex colour (the dragon's palette)
    env = C3D_GetTexEnv(1);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_PREVIOUS, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_MODULATE);
    // 2: + vertex colour x emissive (vertex alpha): heartglow, eye glints
    env = C3D_GetTexEnv(2);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PREVIOUS);
    C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_ALPHA, GPU_TEVOP_RGB_SRC_COLOR);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_MULTIPLY_ADD);
    // 3: + warm rim colour x rim; opaque
    env = C3D_GetTexEnv(3);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, GPU_PREVIOUS, GPU_PREVIOUS);
    C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_ALPHA, GPU_TEVOP_RGB_SRC_COLOR);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_MULTIPLY_ADD);
    C3D_TexEnvSrc(env, C3D_Alpha, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT);
    C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
    C3D_TexEnvColor(env, 0xFF28405A);  // ABGR: warm rim (90, 64, 40), alpha 255
    C3D_TexEnvInit(C3D_GetTexEnv(4));
    C3D_TexEnvInit(C3D_GetTexEnv(5));
}

void drawMesh(App& app, const GpuMesh& g, const Mat34* skin) {
    if (!g.vbo || g.indexCount == 0) return;
    C3D_FVec* rows = C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, g_locBones, g.paletteCount * 3);
    for (int i = 0; i < g.paletteCount; ++i) {
        const Mat34& m = skin[g.palette[i]];
        for (int r = 0; r < 3; ++r) rows[i * 3 + r] = FVec4_New(m.m[r][0], m.m[r][1], m.m[r][2], m.m[r][3]);
    }
    C3D_BufInfo* buf = C3D_GetBufInfo();
    BufInfo_Init(buf);
    BufInfo_Add(buf, g.vbo, sizeof(GpuVertex), 4, 0x3210);
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

    AttrInfo_Init(&g_attr);
    AttrInfo_AddLoader(&g_attr, 0, GPU_FLOAT, 3);          // position
    AttrInfo_AddLoader(&g_attr, 1, GPU_FLOAT, 3);          // normal
    AttrInfo_AddLoader(&g_attr, 2, GPU_UNSIGNED_BYTE, 4);  // skin
    AttrInfo_AddLoader(&g_attr, 3, GPU_UNSIGNED_BYTE, 4);  // paint

    // Lighting (architecture section 4): primary = plum-tinted ambient, secondary = toon
    // ramp on L.N (specular 0 through LUT D0), secondary alpha = rim (Fresnel LUT on N.V).
    static const C3D_Material kMaterial = {
        {0.42f, 0.36f, 0.44f},  // ambient
        {0.0f, 0.0f, 0.0f},     // diffuse (the toon LUT replaces it)
        {1.0f, 1.0f, 1.0f},     // specular0
        {0.0f, 0.0f, 0.0f},     // specular1
        {0.0f, 0.0f, 0.0f},     // emission
    };
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

    g_ready = loadForm("romfs:/models/hatchling.ecm", g_forms[kFormHatchling]) &&
              loadForm("romfs:/models/grown.ecm", g_forms[kFormGrown]);
    if (g_ready) g_adultRadius = framingRadius(g_forms[kFormGrown].model, 1.0f, kBuildNeutral, nullptr, nullptr);
    return g_ready;
}

void shutdown() {
    for (Form& f : g_forms) {
        f.body.release();
        for (GpuMesh& w : f.wings) w.release();
    }
    for (Cache& c : g_caches) {
        c.parts.release();
        c.valid = false;
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
void begin3D(const C3D_Mtx& projection) {
    C2D_Flush();
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
    for (int i = 0; i < 6; ++i) C3D_TexEnvInit(C3D_GetTexEnv(i));
    C2D_Prepare();
    prepare2D();
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
    drawMesh(app, p.form->body, p.skin);
    drawMesh(app, p.cache->parts, p.skin);
    if (const MeshData* wings = selectWings(p.form->model, d.genome))
        drawMesh(app, p.form->wings[wings->variant], p.skin);
}

void lookAt(C3D_Mtx& view, Vec3 eye, Vec3 target) {
    Mtx_LookAt(&view, FVec3_New(eye.x, eye.y, eye.z), FVec3_New(target.x, target.y, target.z), FVec3_New(0, 0, 1),
               false);
}

}  // namespace

void drawDen(App& app, const Dragon* const* dragons, int count, s64 now) {
    if (!g_ready) return;
    ++g_frame;
    if (count > 3) count = 3;
    // Den layout: one dragon in the middle; two side by side; three with the middle one in front.
    static constexpr float kSlotX[3][3] = {{0, 0, 0}, {-0.55f, 0.55f, 0}, {-1.0f, 0.0f, 1.0f}};
    static constexpr float kSlotY[3][3] = {{0, 0, 0}, {0.2f, 0.0f, 0}, {0.45f, 0.0f, 0.6f}};
    float maxRadius = 0;
    const Cache* first = nullptr;
    int shown = 0;
    for (int i = 0; i < count; ++i) {
        const Cache* c = cacheFor(*dragons[i], now);
        if (!c) continue;
        if (!first) first = c;
        const float r = viewRadius(*c, sizeScale(dragons[i]->genome));
        if (r > maxRadius) maxRadius = r;
        ++shown;
    }
    if (!first) return;

    // Camera: in front and a little to the left, looking slightly down, aimed 0.62 radii
    // above the floor so the feet always land on the rug.
    const float yaw = 20.0f * kDegToRad;  // dragons turn their heads a touch toward us
    const float cy = std::cos(yaw), sy = std::sin(yaw);
    const float radius = maxRadius * (1.0f + 0.7f * (shown - 1));
    const float spacing = maxRadius * 1.5f;
    Vec3 target{0, 0, radius * 0.62f};
    if (shown == 1) {  // centre the single dragon's body, not its origin
        const Vec3 c = first->center;
        const float size = sizeScale(dragons[0]->genome);
        target.x = (c.x * cy - c.y * sy) * size;
        target.y = (c.x * sy + c.y * cy) * size;
    }
    const Vec3 dir = normalize(Vec3{-0.35f, -0.9f, 0.32f});
    const float dist = radius / std::tan(kFovY * 0.5f) * 0.95f;
    C3D_Mtx projection, view, model;
    Mtx_PerspTilt(&projection, kFovY, C3D_AspectRatioTop, 0.05f, dist * 4.0f, false);
    lookAt(view, target + dir * dist, target);

    begin3D(projection);
    for (int i = 0, slot = 0; i < count; ++i) {
        if (!pose(app, *dragons[i], now, g_posed)) continue;
        const Vec3 pos{kSlotX[shown - 1][slot] * spacing, kSlotY[shown - 1][slot] * spacing, 0};
        modelMatrix(g_posed, pos, yaw, model);
        submit(app, g_posed, view, model);
        ++slot;
    }
    end3D();
}

void drawCloseUp(App& app, const Dragon& d, s64 now) {
    if (!g_ready) return;
    ++g_frame;
    if (!pose(app, d, now, g_posed) || g_posed.form->headBone < 0 || g_posed.form->chestBone < 0) return;
    C3D_Mtx projection, view, model;
    const float yaw = 30.0f * kDegToRad;
    modelMatrix(g_posed, Vec3{0, 0, 0}, yaw, model);
    // Head and chest: the parts you pet. The heartglow stays in frame.
    const Vec3 head = apply(model, g_posed.poseMat[g_posed.form->headBone].translation());
    const Vec3 chest = apply(model, g_posed.poseMat[g_posed.form->chestBone].translation());
    const Vec3 target = lerp(head, chest, 0.3f);
    const float radius = length(head - chest) * 0.75f;
    const Vec3 dir = normalize(Vec3{-0.3f, -0.95f, 0.18f});
    const float dist = radius / std::tan(kFovY * 0.5f);
    Mtx_PerspTilt(&projection, kFovY, C3D_AspectRatioBot, 0.05f, dist * 4.0f, false);
    lookAt(view, target + dir * dist, target);
    begin3D(projection);
    submit(app, g_posed, view, model);
    end3D();
}

}  // namespace ec::r3d
