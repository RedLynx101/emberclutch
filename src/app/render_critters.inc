// The valley's critters (workstream L, core/critters; included into render3d.cpp's valley code):
// every critter in view, as the one triangle list app/wildlife built this frame, copied into one
// of two buffers (the GPU may still be drawing the other: last frame's, or the other eye's) and
// drawn in a single call with the static program, lit by the day's tint and fogged like the rest
// of the valley. The buffers are made once and kept (58 KB), never freed while a frame may use them.
struct CritterGpu {
    Vec3* pos = nullptr;
    u8* col = nullptr;
};
constexpr int kCritterCap = 600 * 3;  // core/critters kMaxTris, three vertices each
CritterGpu g_critterGpu[2];
int g_critterFlip = 0;

void drawCritters(App& app, const ValleyView& view, const C3D_Mtx& projection, const C3D_Mtx& viewM) {
    if (!view.critterPos || !view.critterCol || view.critterVerts < 3) return;
    const int n = std::min(view.critterVerts, kCritterCap) / 3 * 3;
    g_critterFlip ^= 1;
    CritterGpu& g = g_critterGpu[g_critterFlip];
    if (!g.pos) {
        g.pos = static_cast<Vec3*>(linearAlloc(kCritterCap * sizeof(Vec3)));
        g.col = static_cast<u8*>(linearAlloc(kCritterCap * 4));
        if (!g.pos || !g.col) {
            if (g.pos) linearFree(g.pos);
            if (g.col) linearFree(g.col);
            g = CritterGpu{};
            return;
        }
    }
    std::memcpy(g.pos, view.critterPos, n * sizeof(Vec3));
    std::memcpy(g.col, view.critterCol, n * 4);
    GSPGPU_FlushDataCache(g.pos, n * sizeof(Vec3));
    GSPGPU_FlushDataCache(g.col, n * 4);
    bindValleyStatic(projection, viewM, view.tint);
    C3D_BufInfo* buf = C3D_GetBufInfo();
    BufInfo_Init(buf);
    BufInfo_Add(buf, g.pos, sizeof(Vec3), 1, 0x0);
    BufInfo_Add(buf, g.col, 4, 1, 0x1);
    BufInfo_Add(buf, g.col, 4, 1, 0x2);
    C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
    C3D_CullFace(GPU_CULL_NONE);  // (wings and ears are single triangles, seen from both sides)
    C3D_DrawArrays(GPU_TRIANGLES, 0, n);
    app.stats.tris += n / 3;
    app.stats.draws += 1;
    if (autotest::shooting()) autotest::log("critters: %d triangles in one draw", n / 3);
}
