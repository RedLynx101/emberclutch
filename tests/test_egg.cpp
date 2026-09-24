// Egg tests (WP2): the model (romfs/models/egg.ecm, tools/blender/egg_model.py) loads with
// its two bones; it rests, rocks on its round bottom and opens; cracks appear on schedule
// and stay invisible until then; the dragon inside knocks as hatching nears.
#include <cmath>
#include <cstdio>
#include <vector>

#include "check.hpp"
#include "core/dragon_mesh.hpp"
#include "core/egg.hpp"
#include "core/genetics.hpp"

using namespace ec;

namespace {

const ModelData& eggModel() {
    static ModelData m;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        std::vector<u8> data;
        if (FILE* f = std::fopen("../romfs/models/egg.ecm", "rb")) {
            std::fseek(f, 0, SEEK_END);
            data.resize(static_cast<std::size_t>(std::ftell(f)));
            std::fseek(f, 0, SEEK_SET);
            if (std::fread(data.data(), 1, data.size(), f) != data.size()) data.clear();
            std::fclose(f);
        }
        CHECK(loadModel(data.data(), data.size(), m));
    }
    return m;
}

Dragon anEgg(float progress, float warmth) {
    Rng rng(3);
    Dragon d = makeEgg(1, makePurebred(Element::Ember, rng), Sex::Female, 0);
    d.incubationSeconds = static_cast<s32>(progress * kIncubationSeconds);
    d.warmth = warmth;
    return d;
}

TEST(egg_model_loads_with_its_cap) {
    const ModelData& m = eggModel();
    CHECK(m.skel.count == 2 && m.skel.find("root") == 0 && m.skel.find("cap") == 1);
    const MeshData* shell = m.findMesh(kMeshBody, kGroupBody, 0);
    CHECK(shell != nullptr && m.meshes.size() == 1);
    if (!shell) return;
    std::printf("  egg: %d vertices, %d triangles\n", shell->vertexCount, static_cast<int>(shell->indices.size() / 3));
    CHECK(shell->indices.size() / 3 <= 1000);
    // The cap is the top of the egg; cracks glow (emissive paint) in their own slots.
    const float seam = m.skel.rest[1].translation().z;
    int crackVerts = 0;
    bool split = true;
    for (int v = 0; v < shell->vertexCount; ++v) {
        const bool cap = shell->palette[shell->skin[v * 4]] == 1;
        const float z = shell->pos[v].z;
        split = split && (cap ? z > seam - 0.06f : z < seam + 0.06f);
        const u8 slot = shell->paint[v * 4];
        if (slot == kPalPattern || slot == kPalHorn || slot == kPalMembrane) {
            ++crackVerts;
            CHECK(shell->paint[v * 4 + 3] == 255);
        }
    }
    CHECK(split);
    CHECK(crackVerts > 20);
}

TEST(egg_rests_rocks_and_opens) {
    const ModelData& m = eggModel();
    const MeshData* shell = m.findMesh(kMeshBody, kGroupBody, 0);
    if (!shell) return;
    Mat34 skin[2];
    EggMotion still;
    eggSkin(m, still, skin);
    const Vec3 p{0.1f, -0.2f, 0.7f};
    CHECK(length(transformPoint(skin[0], p) - p) < 1e-5f && length(transformPoint(skin[1], p) - p) < 1e-5f);
    const float restGround = groundOffset(m, skin);
    CHECK(std::fabs(restGround) < 0.01f);  // the egg's lowest point is its base

    // A rub sets it rocking on its round bottom (it rolls rather than sinking), then it settles.
    EggMotion rocking;
    rocking.rub(1.5f, 30, 2);
    Rng rng(1);
    float most = 0, lowest = 0;
    for (int f = 0; f < 30; ++f) {
        rocking.update(1.0f / 30, 0.2f, rng);
        eggSkin(m, rocking, skin);
        most = std::fmax(most, std::fabs(rocking.angle()));
        lowest = std::fmin(lowest, groundOffset(m, skin));
    }
    CHECK(most > 0.05f);
    CHECK(lowest > -0.08f);  // the base stays near the floor while it rocks
    for (int f = 0; f < 120; ++f) rocking.update(1.0f / 30, 0.2f, rng);
    CHECK(rocking.rock < 0.01f);

    // Opening lifts the cap and leaves the rest where it was.
    EggMotion open;
    open.capLift = 1;
    eggSkin(m, open, skin);
    float capRise = 0, rootMove = 0;
    int capVerts = 0;
    for (int v = 0; v < shell->vertexCount; ++v) {
        const int bone = shell->palette[shell->skin[v * 4]];
        const Vec3 moved = transformPoint(skin[bone], shell->pos[v]);
        if (bone == 1) {
            capRise += moved.z - shell->pos[v].z;
            ++capVerts;
        } else {
            rootMove = std::fmax(rootMove, length(moved - shell->pos[v]));
        }
    }
    CHECK(capVerts > 0 && capRise / capVerts > 0.2f);
    CHECK(rootMove < 1e-5f);
}

TEST(egg_cracks_open_on_schedule) {
    CHECK(eggCracks(anEgg(0.5f, 60)) == 0);
    CHECK(eggCracks(anEgg(0.86f, 60)) == 1);
    CHECK(eggCracks(anEgg(0.95f, 60)) == 2);
    CHECK(eggCracks(anEgg(1.0f, 60)) == 3);
    Rgb pal[kPalCount];
    float glow[kPalCount];
    eggPalette(anEgg(0.5f, 60), 1.0f, pal, glow);
    for (u8 slot : {kPalPattern, kPalHorn, kPalMembrane}) {  // closed cracks match the shell
        CHECK(pal[slot].r == pal[kPalBase].r && pal[slot].g == pal[kPalBase].g && pal[slot].b == pal[kPalBase].b);
        CHECK(glow[slot] == 0);
    }
    CHECK(glow[kPalBase] == 0 && glow[kPalGlow] > 0);
    const float coolGlow = glow[kPalGlow];
    eggPalette(anEgg(0.95f, 100), 1.0f, pal, glow);
    CHECK(glow[kPalPattern] > 0 && glow[kPalHorn] > 0 && glow[kPalMembrane] == 0);
    CHECK(pal[kPalPattern].r != pal[kPalBase].r || pal[kPalPattern].b != pal[kPalBase].b);
    CHECK(glow[kPalGlow] > coolGlow);  // a warm egg glows brighter
    eggPalette(anEgg(0.5f, 0), 1.0f, pal, glow);
    CHECK(glow[kPalGlow] < coolGlow);
}

TEST(the_dragon_inside_knocks_near_hatching) {
    Rng rng(9);
    for (float progress : {0.3f, 0.95f}) {
        EggMotion m;
        int knocks = 0;
        for (int f = 0; f < 60 * 30; ++f) knocks += m.update(1.0f / 30, progress, rng);
        std::printf("  progress %.2f: %d knocks a minute\n", progress, knocks);
        CHECK(progress < 0.6f ? knocks == 0 : knocks >= 6);
    }
}

}  // namespace

void runEggTests() {
    RUN(egg_model_loads_with_its_cap);
    RUN(egg_rests_rocks_and_opens);
    RUN(egg_cracks_open_on_schedule);
    RUN(the_dragon_inside_knocks_near_hatching);
}
