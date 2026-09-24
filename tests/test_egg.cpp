// Egg tests (WP2): the model (romfs/models/egg.ecm, tools/blender/egg_model.py) loads with
// its two bones; it rests, rocks on its round bottom and opens; cracks appear on schedule
// and stay invisible until then; the dragon inside knocks as hatching nears.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "check.hpp"
#include "core/dragon_mesh.hpp"
#include "core/egg.hpp"
#include "core/genetics.hpp"
#include "core/names.hpp"

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

// The den's egg (Alpha 2): the same egg in ~300 triangles, so a full den (the room, three
// dragons and two eggs) fits the 8k frame: 8,000 - 2,000 room - 3,000 - 2 x 1,200 = 2 x 300.
TEST(the_den_egg_is_light) {
    std::vector<u8> data;
    if (FILE* f = std::fopen("../romfs/models/egg_lod1.ecm", "rb")) {
        std::fseek(f, 0, SEEK_END);
        data.resize(static_cast<std::size_t>(std::ftell(f)));
        std::fseek(f, 0, SEEK_SET);
        if (std::fread(data.data(), 1, data.size(), f) != data.size()) data.clear();
        std::fclose(f);
    }
    ModelData m;
    CHECK(loadModel(data.data(), data.size(), m));
    CHECK(m.skel.count == 2 && m.skel.find("cap") == 1);
    const MeshData* shell = m.findMesh(kMeshBody, kGroupBody, 0);
    CHECK(shell != nullptr);
    if (!shell) return;
    const int tris = static_cast<int>(shell->indices.size() / 3);
    std::printf("  den egg: %d triangles\n", tris);
    CHECK(tris <= 312);
    bool cracks[3] = {};
    for (int v = 0; v < shell->vertexCount; ++v)
        for (int k = 0; k < 3; ++k) cracks[k] |= shell->paint[v * 4] == (k == 0 ? kPalPattern : k == 1 ? kPalHorn : kPalMembrane);
    CHECK(cracks[0] && cracks[1] && cracks[2]);
}

// Turning counts a few hours apart, up to four times; the turns become bond at hatching.
// The egg spins a quarter turn each time and settles.
TEST(turning_the_egg) {
    Dragon d = anEgg(0.4f, 60);
    const s64 t0 = 1'000'000;
    CHECK(turnEgg(d, t0));
    CHECK(!turnEgg(d, t0 + 3600));  // too soon
    CHECK(turnEgg(d, t0 + kEggTurnGap));
    CHECK(turnEgg(d, t0 + 2 * kEggTurnGap) && turnEgg(d, t0 + 3 * kEggTurnGap));
    CHECK(!turnEgg(d, t0 + 9 * kEggTurnGap));  // four is plenty
    CHECK(d.eggTurns == kMaxEggTurns);
    d.incubationSeconds = kIncubationSeconds;
    Rng rng(1);
    CHECK(tryHatch(d, t0 + 30 * 3600, rng));
    CHECK(d.bond == kBondPerEggTurn * kMaxEggTurns && d.bondHigh == d.bond);
    CHECK(!turnEgg(d, t0 + 40 * 3600));  // hatched

    EggMotion m;
    m.turn();
    Rng r2(2);
    for (int f = 0; f < 60; ++f) m.update(1.0f / 30, 0.2f, r2);
    CHECK(std::fabs(m.yaw - 3.14159265f * 0.5f) < 0.02f);
    Mat34 skin[2];
    eggSkin(eggModel(), m, skin);
    const Vec3 side = transformDir(skin[0], {1, 0, 0});
    CHECK(std::fabs(side.y - 1.0f) < 0.05f);  // a quarter turn about its upright axis
}

// Listening: the heartbeat grows stronger with the egg; its pace tells the temperament it
// hatches with (the same one every time for a given egg).
TEST(listening_to_the_egg) {
    Dragon young = anEgg(0.1f, 60), grown = anEgg(0.9f, 60);
    CHECK(heartbeatOf(young).strength < heartbeatOf(grown).strength);
    int seen[static_cast<int>(Personality::Count)] = {};
    for (u32 id = 1; id <= 120; ++id) {
        Dragon d = anEgg(0.9f, 60);
        d.id = id;
        const Personality p = temperamentOf(d);
        ++seen[static_cast<int>(p)];
        Dragon hatched = d;
        hatched.incubationSeconds = kIncubationSeconds;
        Rng rng(id);
        tryHatch(hatched, 1000, rng);
        CHECK(hatched.personality == p);
    }
    for (int n : seen) CHECK(n >= 8);
    Dragon sleepy = grown, playful = grown;
    for (u32 id = 1; id < 200 && temperamentOf(sleepy) != Personality::Sleepy; ++id) sleepy.id = id;
    for (u32 id = 1; id < 200 && temperamentOf(playful) != Personality::Playful; ++id) playful.id = id;
    CHECK(heartbeatOf(sleepy).bpm < heartbeatOf(playful).bpm - 40);
}

TEST(hatchlings_get_name_suggestions) {
    Dragon d = anEgg(1.0f, 60);
    char a[kNameMax], b[kNameMax];
    int differ = 0;
    for (u32 roll = 0; roll < 40; ++roll) {
        suggestName(d, roll, a, sizeof(a));
        suggestName(d, roll + 1, b, sizeof(b));
        CHECK(a[0] != '\0' && std::strlen(a) < kNameMax);
        differ += std::strcmp(a, b) != 0;
        char again[kNameMax];
        suggestName(d, roll, again, sizeof(again));
        CHECK(std::strcmp(a, again) == 0);
    }
    CHECK(differ > 30);
    char name[kNameMax] = "Old";
    CHECK(setName(name, sizeof(name), "  Cinder  ") && std::strcmp(name, "Cinder") == 0);
    CHECK(!setName(name, sizeof(name), "   ") && std::strcmp(name, "Cinder") == 0);
    CHECK(setName(name, sizeof(name), "Abcdefghijklmnopqrstu") && std::strlen(name) == kNameMax - 1);
    // A two-byte character straddling the end is left out whole.
    CHECK(setName(name, sizeof(name), "Abcdefghijklmn\xC3\xA9") && std::strcmp(name, "Abcdefghijklmn") == 0);
}

}  // namespace

void runEggTests() {
    RUN(egg_model_loads_with_its_cap);
    RUN(egg_rests_rocks_and_opens);
    RUN(egg_cracks_open_on_schedule);
    RUN(the_dragon_inside_knocks_near_hatching);
    RUN(the_den_egg_is_light);
    RUN(turning_the_egg);
    RUN(listening_to_the_egg);
    RUN(hatchlings_get_name_suggestions);
}
