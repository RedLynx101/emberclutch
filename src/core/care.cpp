#include "core/care.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace ec {
namespace {

bool startsWith(const char* s, const char* prefix) { return std::strncmp(s, prefix, std::strlen(prefix)) == 0; }

bool isLimb(const char* bone) {
    for (const char* limb : {"arm_up", "arm_lo", "hand", "leg_up", "leg_lo", "foot"})
        if (startsWith(bone, limb)) return true;
    return false;
}

bool leftSide(const char* bone) {
    const std::size_t n = std::strlen(bone);
    return n > 2 && bone[n - 2] == '_' && bone[n - 1] == 'L';
}

u32 mix(u32 x) {  // a small integer hash
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

float percentile(std::vector<float>& v, float p) {
    std::sort(v.begin(), v.end());
    return v[static_cast<std::size_t>(p * (v.size() - 1) + 0.5f)];
}

}  // namespace

// ------------------------------------------------------------------------------ picking
int buildCapsules(const ModelData& m, BoneCapsule* out, int max) {
    const MeshData* body = m.findMesh(kMeshBody, kGroupBody, 0);
    if (!body) return 0;
    const Skeleton& s = m.skel;
    std::vector<std::vector<Vec3>> local(s.count);
    for (int v = 0; v < body->vertexCount; ++v) {
        const u8* sk = &body->skin[std::size_t(v) * 4];
        const int bone = body->palette[sk[2] >= sk[3] ? sk[0] : sk[1]];
        if ((sk[2] >= sk[3] ? sk[2] : sk[3]) < 150) continue;  // shared vertices blur the capsule
        local[bone].push_back(transformPoint(s.invRest[bone], body->pos[v]));
    }
    int n = 0;
    for (int b = 0; b < s.count && n < max; ++b) {
        std::vector<Vec3>& pts = local[b];
        if (pts.size() < 8) continue;
        BoneCapsule c;
        c.bone = static_cast<u8>(b);
        for (const Vec3& p : pts) c.cx += p.x, c.cz += p.z;
        c.cx /= pts.size();
        c.cz /= pts.size();
        std::vector<float> ys, rs;
        for (const Vec3& p : pts) {
            ys.push_back(p.y);
            rs.push_back(std::sqrt((p.x - c.cx) * (p.x - c.cx) + (p.z - c.cz) * (p.z - c.cz)));
        }
        c.t0 = percentile(ys, 0.05f);
        c.t1 = percentile(ys, 0.95f);
        c.radius = percentile(rs, 0.6f);
        out[n++] = c;
    }
    return n;
}

int pickCapsule(const ScreenCapsule* caps, int n, Vec2 touch, float& t, float& across) {
    int best = -1;
    for (int i = 0; i < n; ++i) {
        const ScreenCapsule& c = caps[i];
        const float ax = c.b.x - c.a.x, ay = c.b.y - c.a.y;
        const float len2 = ax * ax + ay * ay;
        float u = len2 > 1e-6f ? ((touch.x - c.a.x) * ax + (touch.y - c.a.y) * ay) / len2 : 0.0f;
        u = u < 0 ? 0 : (u > 1 ? 1 : u);
        const float px = c.a.x + ax * u - touch.x, py = c.a.y + ay * u - touch.y;
        const float d = std::sqrt(px * px + py * py);
        if (d > c.radius || (best >= 0 && c.depth >= caps[best].depth)) continue;
        best = i;
        t = u;
        const float side = ax * (touch.y - c.a.y) - ay * (touch.x - c.a.x);  // 2D cross: > 0 counter-clockwise
        across = (side >= 0 ? 1.0f : -1.0f) * (c.radius > 0 ? d / c.radius : 0.0f);
    }
    return best;
}

PetZone zoneOf(const char* bone, Vec3 outward, float t) {
    if (std::strcmp(bone, "jaw") == 0) return PetZone::Chin;
    if (startsWith(bone, "antenna") || startsWith(bone, "ear")) return PetZone::Head;  // a kind's own head bones (D77)
    if (std::strcmp(bone, "head") == 0 || std::strcmp(bone, "snout") == 0 || std::strcmp(bone, "eyes") == 0) {
        if (outward.z < -0.35f) return PetZone::Chin;
        if (std::fabs(outward.x) > 0.6f && outward.z < 0.5f) return PetZone::Cheek;
        return PetZone::Head;
    }
    if (startsWith(bone, "neck")) return outward.z < -0.3f ? PetZone::Chin : PetZone::Neck;
    if (startsWith(bone, "tail")) return PetZone::Tail;
    if (startsWith(bone, "wing")) return PetZone::Wing;
    if (isLimb(bone)) return PetZone::Paw;
    if (std::strcmp(bone, "chest") == 0 && outward.y < -0.55f && outward.z > -0.5f && t < 0.6f) return PetZone::Heart;
    if (outward.z < -0.4f) return PetZone::Belly;
    return PetZone::Back;
}

int regionOf(const char* bone, Vec3 outward) {
    if (std::strcmp(bone, "head") == 0 || std::strcmp(bone, "snout") == 0 || std::strcmp(bone, "jaw") == 0 ||
        std::strcmp(bone, "eyes") == 0 || startsWith(bone, "antenna") || startsWith(bone, "ear"))
        return kRegionHead;
    if (startsWith(bone, "neck")) return kRegionNeck;
    if (startsWith(bone, "tail")) return kRegionTail;
    if (startsWith(bone, "wing")) return kRegionWings;
    if (isLimb(bone)) return leftSide(bone) ? kRegionLeft : kRegionRight;
    if (outward.z > 0.35f) return kRegionBack;
    if (outward.z < -0.4f) return kRegionBelly;
    return outward.x < 0 ? kRegionLeft : kRegionRight;
}

// ------------------------------------------------------------------------------ strokes
namespace {
constexpr float kRoughSpeed = 900.0f;     // px/s: scrubbing this hard is too much
constexpr float kEndlessSeconds = 12.0f;  // one stroke this long is too much too
constexpr float kReversalMemory = 0.8f;   // s: back-and-forth only counts while it keeps going
}  // namespace

void StrokeTracker::begin(Vec2 p) {
    *this = StrokeTracker{};
    start = last = p;
    down = true;
}

Stroke StrokeTracker::update(Vec2 p, float dt) {
    if (!down) begin(p);
    time += dt;
    const float dx = p.x - last.x, dy = p.y - last.y;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len > 0.5f) {
        const Vec2 nd{dx / len, dy / len};
        const bool hadDir = dir.x != 0 || dir.y != 0;
        if (hadDir) {
            float c = dir.x * nd.x + dir.y * nd.y;
            c = c < -1 ? -1 : (c > 1 ? 1 : c);
            if (c < -0.3f) {
                ++reversals;
                lastReversal = time;
            }
            turning = turning * 0.85f + (std::acos(c) / len) * 0.15f;
        }
        const float mx = hadDir ? dir.x * 0.5f + nd.x * 0.5f : nd.x;
        const float my = hadDir ? dir.y * 0.5f + nd.y * 0.5f : nd.y;
        const float ml = std::sqrt(mx * mx + my * my);
        dir = ml > 1e-4f ? Vec2{mx / ml, my / ml} : nd;
        distance += len;
    }
    if (dt > 0) speed = speed * 0.8f + (len / dt) * 0.2f;
    if (reversals > 0 && time - lastReversal > kReversalMemory) reversals = 0;
    last = p;
    return kind();
}

Stroke StrokeTracker::end() {
    const bool poke = down && time < 0.3f && distance < 10.0f;
    down = false;
    return poke ? Stroke::Poke : Stroke::None;
}

Stroke StrokeTracker::kind() const {
    if (!down) return Stroke::None;
    if (speed > kRoughSpeed || time > kEndlessSeconds) return Stroke::Rough;
    if (reversals >= 3) return Stroke::Scrub;
    if (turning > 0.06f && distance > 30.0f) return Stroke::Scratch;
    if (distance > 12.0f) return Stroke::Gentle;
    return Stroke::None;
}

// ------------------------------------------------------------------------------ quirks
SweetSpot sweetSpotOf(const Dragon& d) {
    static constexpr PetZone kZones[] = {PetZone::Head, PetZone::Cheek, PetZone::Chin, PetZone::Neck,
                                         PetZone::Back, PetZone::Belly, PetZone::Tail};
    const u32 h = mix(d.id * 2654435761U + 0x5EEDu);
    SweetSpot s;
    s.zone = kZones[h % (sizeof(kZones) / sizeof(kZones[0]))];
    const bool sided = s.zone == PetZone::Cheek || s.zone == PetZone::Back || s.zone == PetZone::Neck;
    s.side = sided ? static_cast<s8>((h >> 8) % 2 ? 1 : -1) : 0;
    return s;
}

bool atSweetSpot(const Dragon& d, PetZone zone, Vec3 outward) {
    const SweetSpot s = sweetSpotOf(d);
    if (zone != s.zone) return false;
    if (s.side == 0) return true;
    return (outward.x > 0.15f && s.side > 0) || (outward.x < -0.15f && s.side < 0);
}

const FoodInfo& foodInfo(Food f) {
    static const FoodInfo kFoods[static_cast<int>(Food::Count)] = {
        {"Firepepper", 30, 2, false}, {"River fish", 32, 3, false},  {"Skyberry", 26, 2, false},
        {"Honeyroot", 30, 3, false},  {"Frostmelon", 28, 3, false},  {"Starfruit", 28, 2, false},
        {"Hearth bread", 24, 2, false}, {"Roast drumstick", 40, 3, false}, {"Ember candy", 8, 1, true},
        {"Glimmer cookie", 6, 1, true},
    };
    return kFoods[static_cast<int>(f) < static_cast<int>(Food::Count) ? static_cast<int>(f) : 0];
}

Taste tasteOf(const Dragon& d, Food f) {
    if (f == Food::EmberCandy) return Taste::Favorite;  // the make-up helper (content inventory §7)
    const int food = static_cast<int>(f);
    if (food >= kElementCount) return Taste::Liked;  // basic foods and the cookie: everyone likes them
    if (food == d.favoriteFood) return Taste::Favorite;
    // One or two dislikes among the other element foods, fixed by the dragon's id.
    const u32 h = mix(d.id * 0x9E3779B1U + 0xF00Du);
    auto nth = [&](u32 k) {  // the k-th element food that isn't the favourite
        int n = static_cast<int>(k % (kElementCount - 1));
        return n >= d.favoriteFood ? n + 1 : n;
    };
    if (food == nth(h)) return Taste::Disliked;
    if ((h >> 12) % 3 == 0 && food == nth((h >> 4) + 1) && nth((h >> 4) + 1) != nth(h)) return Taste::Disliked;
    return Taste::Liked;
}

BathMood bathMoodOf(const Dragon& d) {
    switch (static_cast<Element>(d.genome.elementA)) {
        case Element::Tide: return BathMood::Loves;
        case Element::Ember: return BathMood::Grudging;
        default: return BathMood::Fine;
    }
}

// ------------------------------------------------------------------------------ grooming
namespace {
constexpr float kShinePerRegion = 8.0f;   // a whole region brushed; polishing adds half as much
constexpr float kDustPerRegion = 110.0f;  // a whole region brushed takes all its dust off
float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
float clamp100(float v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }
}  // namespace

bool GroomSession::brush(Dragon& d, int region, float amount, bool withGrain) {
    if (region < 0 || region >= kRegionCount || amount <= 0) return false;
    const float a = withGrain ? amount : amount * 0.5f;
    const float before = brushed[region];
    brushed[region] = clamp01(before + a);
    const float gained = brushed[region] - before;
    cleanRegion(d, region, a * kDustPerRegion);
    d.needs.shine = clamp100(d.needs.shine + gained * kShinePerRegion);
    const bool done = before < 1.0f && brushed[region] >= 1.0f;
    if (done) addBond(d, 1);
    return done;
}

bool GroomSession::polish(Dragon& d, int region, float amount) {
    if (region < 0 || region >= kRegionCount || amount <= 0) return false;
    const float before = polished[region];
    polished[region] = clamp01(before + amount);
    d.needs.shine = clamp100(d.needs.shine + (polished[region] - before) * kShinePerRegion * 0.5f);
    return before < 1.0f && polished[region] >= 1.0f;
}

bool GroomSession::checkGleam() {
    if (gleamed) return false;
    for (int r = 0; r < kRegionCount; ++r)
        if (brushed[r] < 1.0f || polished[r] < 1.0f) return false;
    gleamed = true;
    return true;
}

float GroomSession::coverage() const {
    float sum = 0;
    for (float b : brushed) sum += b;
    return sum / kRegionCount;
}

}  // namespace ec
