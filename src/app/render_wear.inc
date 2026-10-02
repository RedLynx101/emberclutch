// 1.0's accessories on the dragons (the pageant, D90): included into render3d.cpp's own namespace
// (it draws with that file's meshes, the dragons' program and their light), called at the end of
// submit(), so every dragon wears its things wherever it's drawn: the den, the close-ups, the
// showcases, the valley and the others there. Each accessory's mesh (core/wear_mesh) is built once
// and carried by the bones core/wear_fit measured for the dragon's model: the frame placed by the
// bones' skinning matrices, as the body's own vertices are.

GpuMesh g_wearMeshes[kWearShapes];
WearFit g_wearFits[kLookSlots][kFormCount][2];  // per look, form and detail (the models never change)
bool g_wearFitTried[kLookSlots][kFormCount][2] = {};

// An accessory's mesh on the GPU: every vertex half on each of its two bones (palette 0 and 1).
bool uploadWear(GpuMesh& g, const PropMesh& m) {
    const std::size_t n = m.pos.size();
    std::vector<u8> skin;
    std::vector<float> uv;
    for (std::size_t v = 0; v < n; ++v) {
        skin.insert(skin.end(), {0, 1, 128, 127});
        uv.insert(uv.end(), {kCleanUv, kCleanUv});
    }
    const u8 palette[2] = {0, 1};
    return fill(g, static_cast<int>(n), m.pos.data(), m.nrm.data(), skin.data(), m.paint.data(), uv.data(), m.idx.data(),
                static_cast<int>(m.idx.size()), palette, 2);
}

const WearFit* wearFitFor(const Posed& p) {
    const int look = p.cache->look, form = p.cache->form;
    if (look < 0 || look >= kLookSlots || form < 0 || form >= kFormCount) return nullptr;
    const int lod = p.form == &g_forms[look][form][1] ? 1 : 0;
    WearFit& fit = g_wearFits[look][form][lod];
    if (!g_wearFitTried[look][form][lod]) {
        g_wearFitTried[look][form][lod] = true;
        // (the far model wears what the near one measured when it's loaded: the same bones, and no
        // hat jumping as a dragon walks off into the distance, run 23)
        const Form& near = g_forms[look][form][0];
        const ModelData& m = lod == 1 && !near.model.meshes.empty() ? near.model : p.form->model;
        fitWear(m, isKind(look) ? planOfSlot(look) : -1, form == kFormGrown, fit);
    }
    return &fit;
}

void drawWorn(App& app, const Posed& p) {
    const Dragon& d = *p.dragon;
    if (acc::wornCount(d) == 0 || !p.cache || !p.form) return;
    const WearFit* fit = wearFitFor(p);
    if (!fit) return;
    // Plain vertex colours on the skin's clean corner, and the rim back to the light's (a rare
    // dragon's glowing marks had stage 5).
    bindSkin(nullptr);
    kindPattern(-1, -1, {0, 0, 0}, {0, 0, 0});
    C3D_TexEnvColor(C3D_GetTexEnv(5), g_lastRim);
    for (int s = 0; s < kWearSlots; ++s) {
        const int a = acc::worn(d, static_cast<WearSlot>(s));
        if (a < 0 || !fit->ok[s]) continue;
        const Accessory& x = accessoryInfo(a);
        GpuMesh& g = g_wearMeshes[static_cast<int>(x.shape)];
        if (!g.vbo && !uploadWear(g, wearMesh(x.shape))) continue;
        PropLook look;
        look.colour[0] = x.colour[0];
        look.colour[1] = x.colour[1];
        look.colour[2] = x.trim;
        look.colour[3] = x.gem;
        setLook(look, 0.7f);
        const bool head = s == static_cast<int>(WearSlot::Head);
        const Mat34& frame = head && narrowOnHead(x.shape) ? fit->headNarrow : head && x.shape == WearShape::SunHat ? fit->headWide : fit->frame[s];
        const Mat34 bones[2] = {mul(p.skin[fit->boneA[s]], frame), mul(p.skin[fit->boneB[s]], frame)};
        drawMesh(app, g, bones);
    }
}

}  // namespace (closed for the pageant's public drawing below; opened again after it)

// The wardrobe's dragon (app/scene_wardrobe.cpp): as drawShowcase, standing at ease (or playing
// `clip`) and turned `spin`, framed on the slot being dressed as `zoom` goes 0 -> 1 (the whole
// dragon at 0).
void drawDressing(App& app, const Dragon& d, s64 now, float spin, int slot, float zoom, ClipId clip) {
    if (!g_ready || d.stage == Stage::Egg) return;
    ++g_frame;
    static DenActor show;
    static u32 showId = 0;
    static int showForm = -1, showLook = -1;
    static ClipId showClip = ClipId::Idle;
    const int form = growthFor(d.stage, stageProgress(d, now)).form;
    const int look = lookOf(d);
    const int* clips = clipsForSlot(look, form);
    const bool animsOk = animsOkFor(look);
    if (animsOk && (showId != d.id || showForm != form || showClip != clip || showLook != look)) {
        show = DenActor{};
        int index = clips[static_cast<int>(clip)];
        if (index < 0) index = clips[static_cast<int>(ClipId::Idle)];
        show.anim.play(index, 0.0f, true);
        showId = d.id;
        showForm = form;
        showClip = clip;
        showLook = look;
    }
    if (animsOk) {
        show.anim.update(libFor(look), app.dt, nullptr, 0);
        show.eyes.update(0.0f, app.dt);
    }
    if (!pose(app, d, animsOk ? &show : nullptr, now, 0, g_posed)) return;
    C3D_Mtx projection, view, model;
    modelMatrix(g_posed, model);
    const Vec3 hips = apply(model, g_posed.poseMat[0].translation());
    const float whole = g_posed.cache->radius * g_posed.size;
    Vec3 target{hips.x, hips.y, hips.z + whole * 0.25f};
    float radius = whole * 1.05f;
    if (slot >= 0 && slot < kWearSlots && zoom > 0.001f) {
        if (const WearFit* fit = wearFitFor(g_posed); fit && fit->ok[slot]) {
            const Mat34 m = mul(g_posed.skin[fit->boneA[slot]], fit->frame[slot]);
            const Vec3 at = apply(model, m.translation());
            const float unit = length(Vec3{m.m[0][0], m.m[1][0], m.m[2][0]}) * g_posed.size;
            static constexpr float kAcross[kWearSlots] = {2.4f, 3.0f, 2.2f, 4.0f};  // the view's radius, in the slot's units
            const float close = std::fmax(unit * kAcross[slot], whole * 0.2f);
            target = lerp(target, at, zoom);
            radius += (std::fmin(close, radius) - radius) * zoom;
        }
    }
    Vec3 dir = normalize(Vec3{-0.35f * std::cos(spin) - 0.9f * std::sin(spin), 0.35f * std::sin(spin) - 0.9f * std::cos(spin), 0.3f});
    if (slot == static_cast<int>(WearSlot::Tail) && zoom > 0.001f) {  // (D126: the tail from its own side, not through the body)
        Vec3 out{target.x - hips.x, target.y - hips.y, 0};
        if (length(out) > 1e-3f) {
            out = normalize(out);
            dir = normalize(lerp(dir, normalize(out * 0.8f + dir * 0.35f + Vec3{0, 0, 0.35f}), zoom));
        }
    }
    const float dist = radius / std::tan(kFovY * 0.5f);
    topProjection(projection, 0.05f, dist * 4.0f, dist);
    lookAt(view, target + dir * dist, target);
    C2D_Flush();
    bindDragons(projection);
    const float plain[3] = {1, 1, 1};
    lightDragon(dragonLight(dayBlend(now)), plain);
    submit(app, g_posed, view, model);
    end3D();
}

namespace {
