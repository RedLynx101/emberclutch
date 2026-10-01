#include "core/roamers.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

#include "core/clock.hpp"
#include "core/kinds.hpp"
#include "core/people.hpp"
#include "core/place_layout.hpp"
#include "core/rng.hpp"
#include "core/trainer.hpp"
#include "core/valley.hpp"
#include "core/walker.hpp"

namespace ec::roam {
namespace {

constexpr float kPi = 3.14159265f;

// Skins, hair and eyes as the people kit's (tools/people/looks.py), as the league's.
constexpr Rgb kPeach{255, 222, 196}, kHoney{240, 192, 152}, kSand{212, 158, 112}, kUmber{164, 108, 72},
    kCocoa{112, 70, 48};
constexpr Rgb kChestnut{132, 78, 44}, kDarkBrown{82, 54, 40}, kBlack{46, 40, 46}, kGinger{208, 104, 50},
    kGolden{236, 194, 104}, kSilver{220, 222, 232};
constexpr Rgb kBrownEyes{86, 56, 40}, kBlueEyes{70, 118, 176}, kGreenEyes{76, 128, 78}, kGreyEyes{118, 110, 124};

constexpr u8 kBodyA = static_cast<u8>(Person::PlayerA), kBodyB = static_cast<u8>(Person::PlayerB);
constexpr u8 kTraveller = static_cast<u8>(Person::Traveller), kChild = static_cast<u8>(Person::Child);

// Eight trainers out walking: each their own look, voice and dragon, and their own way of talking.
const Roamer kTable[kRoamers] = {
    {"Juno", "watches the sky", kBodyB,
     {1, 2, kPeach, kChestnut, {110, 170, 220}, {250, 238, 206}, {126, 84, 54}, kGreenEyes}, 1, 1.6f,
     {"Oh, hello there!", "Lovely day for flying!"},
     {"I'm Juno! Breeze and I walk the whole valley, watching the sky.",
      "You two look like you train a lot. A friendly duel, just for fun?"},
     "Breeze wants another go! Just a friendly one?",
     "Breeze won! Don't be sad, {D} was so close.",
     "You two fly as one! Breeze, let's keep practising.",
     "Breeze", "crestwing", 1, 0, 1, 1.25f},
    {"Dex", "a young hiker", kBodyA,
     {0, 4, kUmber, kBlack, {232, 142, 52}, {92, 72, 54}, {80, 60, 48}, kBrownEyes}, 1, 1.85f,
     {"Hey! Hi!", "Race you to the Market!"},
     {"I'm Dex! Nib and I are going to climb every hill in the valley.",
      "But first... a duel! Nib's been itching for one!"},
     "Again, again! Nib's not tired at all!",
     "We won! Nib, did you see that?!",
     "Aww! Okay, you're really good. Nib, snack break.",
     "Nib", "pouncer", 2, 0, 0, 1.4f},
    {"Marlo", "an old wanderer", kTraveller,
     {0, kNoHair, kSand, kSilver, {122, 92, 124}, {204, 172, 112}, {104, 82, 60}, kGreyEyes}, 0, 1.05f,
     {"Good day to you.", "Fine weather for walking."},
     {"Marlo. Ashby and I have walked these paths since before the Market was built.",
      "Humour an old traveller? A friendly duel, nothing more."},
     "Ashby's still warm from the last one. Once more?",
     "Old roads, old tricks. You'll get us next time, young one.",
     "Well, well. The valley's in good hands.",
     "Ashby", "kindlemoss", 0, 1, 2, 1.0f},
    {"Tilly", "Button's biggest fan", kChild,
     {0, kNoHair, kHoney, kGolden, {250, 170, 190}, {112, 92, 200}, {90, 70, 60}, kBlueEyes}, 1, 2.0f,
     {"Hiii!", "Is that your dragon? Wow!"},
     {"I'm Tilly and this is Button! Button is the bravest dragon ever.",
      "Wanna duel? Button REALLY wants to!"},
     "Button says one more! Pleeease?",
     "Button did it! Button, you're the best!",
     "Button tried really hard. Good job, {D}!",
     "Button", "puffback", 1, 0, 0, 1.3f},
    {"Ravi", "writes about dragons", kBodyA,
     {0, 0, kCocoa, kDarkBrown, {150, 62, 72}, {232, 192, 92}, {70, 50, 44}, kBrownEyes}, 0, 1.35f,
     {"Ah, a fellow keeper!", "What a fascinating dragon."},
     {"Ravi, at your service. I'm writing a book on how dragons duel.",
      "Would you and {D} help my research? A friendly bout?"},
     "Another chapter? Fathom is quite keen.",
     "Fathom wins! I shall note that down. Thank you, {P}.",
     "Remarkable! That goes in the book, {P}.",
     "Fathom", "lilyfin", 2, 0, 2, 1.15f},
    {"Sage", "plants wildflowers", kBodyB,
     {1, 5, kUmber, kChestnut, {112, 162, 92}, {242, 204, 122}, {110, 80, 56}, kGreenEyes}, 1, 1.45f,
     {"Mind the flowers!", "Hello, keeper!"},
     {"I'm Sage. Thornbud and I plant wildflowers along the paths.",
      "Something tells me you'd like a duel. Shall we?"},
     "Thornbud's blooming again. Another round?",
     "A good duel is like a good garden: patience. Well played.",
     "Beautifully done! Thornbud, let's go sniff the roses.",
     "Thornbud", "bloomstone", 0, 0, 1, 1.1f},
    {"Colm", "fishes the lake", kBodyA,
     {0, 5, kPeach, kGinger, {52, 72, 122}, {224, 224, 214}, {62, 52, 44}, kBlueEyes}, 0, 1.2f,
     {"Ahoy there!", "The fish are biting today."},
     {"Colm's the name. The fishing's slow, so Riptide and I go walking.",
      "Fancy a friendly duel while the fish think it over?"},
     "Riptide's still splashing about. Again?",
     "Hooked and landed! No hard feelings, {P}.",
     "The one that got away! Well duelled.",
     "Riptide", "ribbontail", 1, 1, 2, 1.2f},
    {"Elspeth", "knits scarves for dragons", kBodyB,
     {1, 3, kPeach, kSilver, {170, 142, 204}, {250, 240, 222}, {112, 82, 72}, kGreyEyes}, 1, 1.3f,
     {"Hello, dearie!", "Don't forget your scarf!"},
     {"Oh, what a fine dragon. I'm Elspeth, and this is Mittens.",
      "We may be old, but we still duel. Care to try us, dear?"},
     "Mittens is ready when you are, dear.",
     "Oh my! Mittens still has it. Have a biscuit, dear.",
     "Well done, dearie! Your {D} is a treasure.",
     "Mittens", "flurrytail", 2, 1, 3, 0.95f},
};

bool valid(int id) { return id >= 0 && id < kRoamers; }

Vec2 sub(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
float dist(Vec2 a, Vec2 b) { return std::hypot(a.x - b.x, a.y - b.y); }
float headingOf(Vec2 d) { return std::atan2(d.x, -d.y); }  // (0 faces -Y)

void lengths(const std::vector<Vec2>& pts, std::vector<float>& cum) {
    cum.assign(pts.size(), 0.0f);
    for (std::size_t k = 1; k < pts.size(); ++k) cum[k] = cum[k - 1] + dist(pts[k], pts[k - 1]);
}

// The part of a polyline from `d0` to `d1` metres along it.
std::vector<Vec2> cut(const std::vector<Vec2>& pts, float d0, float d1) {
    std::vector<float> cum;
    lengths(pts, cum);
    std::vector<Vec2> out;
    if (pts.empty()) return out;
    d1 = std::fmax(d1, d0);
    out.push_back(along(pts, cum, d0));
    for (std::size_t k = 0; k < pts.size(); ++k)
        if (cum[k] > d0 + 0.01f && cum[k] < d1 - 0.01f) out.push_back(pts[k]);
    out.push_back(along(pts, cum, d1));
    return out;
}

// The places' walls (and the villagers) by where they are: a grid of 16 m cells over them, each
// listing the walls that reach into it, so asking whether a spot is clear looks at a few walls near
// it rather than all of them (the network is built as the valley opens: on the 3DS it must be quick).
struct SolidIndex {
    static constexpr float kCell = 16.0f, kPad = 1.6f;  // (the margins asked for stay under kPad)
    const std::vector<Solid>* solids = nullptr;
    float x0 = 0, y0 = 0;
    int w = 0, h = 0;
    std::vector<u32> start;  // per cell: where its walls start in `ids` (and, last, the end)
    std::vector<u16> ids;

    void span(const Solid& s, int& cx0, int& cy0, int& cx1, int& cy1) const {
        const float r = s.radius + kPad;
        cx0 = static_cast<int>((s.at.x - r - x0) / kCell);
        cy0 = static_cast<int>((s.at.y - r - y0) / kCell);
        cx1 = std::min(w - 1, static_cast<int>((s.at.x + r - x0) / kCell));
        cy1 = std::min(h - 1, static_cast<int>((s.at.y + r - y0) / kCell));
    }
    void build(const std::vector<Solid>& list) {
        solids = &list;
        if (list.empty()) return;
        float x1 = -1e9f, y1 = -1e9f;
        x0 = y0 = 1e9f;
        for (const Solid& s : list) {
            x0 = std::fmin(x0, s.at.x - s.radius - kPad);
            y0 = std::fmin(y0, s.at.y - s.radius - kPad);
            x1 = std::fmax(x1, s.at.x + s.radius + kPad);
            y1 = std::fmax(y1, s.at.y + s.radius + kPad);
        }
        w = static_cast<int>((x1 - x0) / kCell) + 1;
        h = static_cast<int>((y1 - y0) / kCell) + 1;
        start.assign(static_cast<std::size_t>(w * h + 1), 0);
        int cx0, cy0, cx1, cy1;
        for (const Solid& s : list) {  // (how many in each cell, then where each cell's run starts)
            span(s, cx0, cy0, cx1, cy1);
            for (int cy = cy0; cy <= cy1; ++cy)
                for (int cx = cx0; cx <= cx1; ++cx) ++start[static_cast<std::size_t>(cy * w + cx + 1)];
        }
        for (std::size_t c = 1; c < start.size(); ++c) start[c] += start[c - 1];
        ids.assign(start.back(), 0);
        std::vector<u32> at(start.begin(), start.end() - 1);
        for (std::size_t k = 0; k < list.size(); ++k) {
            span(list[k], cx0, cy0, cx1, cy1);
            for (int cy = cy0; cy <= cy1; ++cy)
                for (int cx = cx0; cx <= cx1; ++cx) ids[at[static_cast<std::size_t>(cy * w + cx)]++] = static_cast<u16>(k);
        }
    }
    // Clear of every wall by `margin` (at most kPad).
    bool clear(Vec2 p, float margin) const {
        if (!solids || w == 0) return true;
        const int cx = static_cast<int>(std::floor((p.x - x0) / kCell)), cy = static_cast<int>(std::floor((p.y - y0) / kCell));
        if (cx < 0 || cy < 0 || cx >= w || cy >= h) return true;
        const std::size_t c = static_cast<std::size_t>(cy * w + cx);
        for (u32 k = start[c]; k < start[c + 1]; ++k) {
            const Solid& s = (*solids)[ids[k]];
            const float dx = p.x - s.at.x, dy = p.y - s.at.y, r = s.radius + margin;
            if (dx * dx + dy * dy < r * r) return false;
        }
        return true;
    }
};

bool lineClear(const SolidIndex& walls, Vec2 a, Vec2 b, float margin) {
    const int n = static_cast<int>(dist(a, b) / 0.5f) + 1;
    for (int k = 0; k <= n; ++k) {
        const float u = static_cast<float>(k) / n;
        if (!walls.clear({a.x + (b.x - a.x) * u, a.y + (b.y - a.y) * u}, margin)) return false;
    }
    return true;
}

// A way nudged round the walls it brushes (a bridge's post, a gatepost): points every metre, any
// too near a wall pushed out from it, eased along, pushed again. Its ends stay put.
void keepClear(std::vector<Vec2>& pts, const SolidIndex& near) {
    constexpr float kMargin = 0.5f;
    if (pts.size() < 2) return;
    std::vector<float> cum;
    lengths(pts, cum);
    bool brushes = false;  // (nothing near it: left as it is)
    for (float d = 0; d <= cum.back() && !brushes; d += 0.5f) brushes = !near.clear(along(pts, cum, d), kMargin);
    if (!brushes) return;
    const int n = static_cast<int>(cum.back() / 0.5f) + 1;  // (every half metre: no corner cut past a post)
    std::vector<Vec2> even;
    for (int k = 0; k <= n; ++k) even.push_back(along(pts, cum, cum.back() * k / n));
    // (to the nearest spot clear of every wall, on the side the way before it went: walls that
    // touch leave no gap to squeeze through)
    auto push = [&]() {
        for (std::size_t k = 1; k + 1 < even.size(); ++k) {
            if (near.clear(even[k], kMargin)) continue;
            for (float r = 0.25f; r <= 8.0f; r += 0.25f) {
                float best = 1e9f;
                Vec2 pick = even[k];
                for (int a = 0; a < 32; ++a) {
                    const float ang = a * (2.0f * kPi / 32.0f);
                    const Vec2 q{even[k].x + r * std::cos(ang), even[k].y + r * std::sin(ang)};
                    const float d = dist(q, even[k - 1]);
                    if (d < best && near.clear(q, kMargin)) {
                        best = d;
                        pick = q;
                    }
                }
                if (best < 1e9f) {
                    even[k] = pick;
                    break;
                }
            }
        }
    };
    for (int pass = 0; pass < 3; ++pass) {
        push();
        std::vector<Vec2> eased = even;
        for (std::size_t k = 1; k + 1 < even.size(); ++k)
            eased[k] = {(even[k - 1].x + 2 * even[k].x + even[k + 1].x) / 4, (even[k - 1].y + 2 * even[k].y + even[k + 1].y) / 4};
        even.swap(eased);
    }
    for (int pass = 0; pass < 4; ++pass) {  // pushed, then any long step between two split and pushed again
        push();
        std::vector<Vec2> finer{even[0]};
        for (std::size_t k = 1; k < even.size(); ++k) {
            if (dist(even[k], even[k - 1]) > 0.6f) finer.push_back({(even[k].x + even[k - 1].x) / 2, (even[k].y + even[k - 1].y) / 2});
            finer.push_back(even[k]);
        }
        even.swap(finer);
    }
    push();
    pts.swap(even);
}

// A way from p to q round the walls between them (a junction's crossing through a busy square):
// straight if nothing's in the way, else the shortest way on a half-metre grid, pulled straight
// wherever it can see ahead.
std::vector<Vec2> routeAround(Vec2 p, Vec2 q, const SolidIndex& near) {
    constexpr float kMargin = 0.5f, kCell = 0.5f, kPad = 12.0f;
    if (lineClear(near, p, q, kMargin)) return {p, q};
    const float x0 = std::fmin(p.x, q.x) - kPad, y0 = std::fmin(p.y, q.y) - kPad;
    const int w = static_cast<int>((std::fabs(p.x - q.x) + 2 * kPad) / kCell) + 1;
    const int h = static_cast<int>((std::fabs(p.y - q.y) + 2 * kPad) / kCell) + 1;
    if (w > 240 || h > 240) return {p, q};
    auto centre = [&](int c) { return Vec2{x0 + (c % w + 0.5f) * kCell, y0 + (c / w + 0.5f) * kCell}; };
    auto cellOf = [&](Vec2 v) {
        const int cx = std::min(w - 1, std::max(0, static_cast<int>((v.x - x0) / kCell)));
        const int cy = std::min(h - 1, std::max(0, static_cast<int>((v.y - y0) / kCell)));
        return cy * w + cx;
    };
    std::vector<u8> blocked(static_cast<std::size_t>(w * h));
    for (int c = 0; c < w * h; ++c) blocked[static_cast<std::size_t>(c)] = !near.clear(centre(c), kMargin);
    const int start = cellOf(p), goal = cellOf(q);
    blocked[static_cast<std::size_t>(start)] = blocked[static_cast<std::size_t>(goal)] = 0;
    std::vector<float> cost(static_cast<std::size_t>(w * h), 1e30f);
    std::vector<int> came(static_cast<std::size_t>(w * h), -1);
    std::vector<std::pair<float, int>> open;  // (a heap: the least estimate first)
    auto later = [](const std::pair<float, int>& a, const std::pair<float, int>& b) { return a.first > b.first; };
    cost[static_cast<std::size_t>(start)] = 0;
    open.push_back({dist(p, q), start});
    while (!open.empty()) {
        std::pop_heap(open.begin(), open.end(), later);
        const int c = open.back().second;
        open.pop_back();
        if (c == goal) break;
        const int cx = c % w, cy = c / w;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                const int nx = cx + dx, ny = cy + dy;
                if ((!dx && !dy) || nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
                const int nc = ny * w + nx;
                if (blocked[static_cast<std::size_t>(nc)]) continue;
                const float g = cost[static_cast<std::size_t>(c)] + kCell * (dx && dy ? 1.4142f : 1.0f);
                if (g >= cost[static_cast<std::size_t>(nc)]) continue;
                cost[static_cast<std::size_t>(nc)] = g;
                came[static_cast<std::size_t>(nc)] = c;
                open.push_back({g + dist(centre(nc), q), nc});
                std::push_heap(open.begin(), open.end(), later);
            }
    }
    if (came[static_cast<std::size_t>(goal)] < 0) return {p, q};  // (walled off: straight across)
    std::vector<Vec2> cells;
    for (int c = goal; c >= 0; c = came[static_cast<std::size_t>(c)]) cells.push_back(centre(c));
    cells.back() = p;
    cells.front() = q;
    std::vector<Vec2> way{p};
    for (std::size_t at = cells.size() - 1; at > 0;) {  // pulled straight: on to the furthest point in sight
        std::size_t next = at - 1;
        for (std::size_t k = 0; k + 1 < at; ++k)
            if (lineClear(near, cells[at], cells[k], kMargin - 0.05f)) {
                next = k;
                break;
            }
        way.push_back(cells[next]);
        at = next;
    }
    return way;
}

// The stretch's way from `node` (reversed if it runs the other way).
std::vector<Vec2> fromNode(const NetEdge& e, int node) {
    if (e.a == node) return e.pts;
    return std::vector<Vec2>(e.pts.rbegin(), e.pts.rend());
}

u64 seedOf(s32 day, int id) { return 0x51A7E00Dull ^ (static_cast<u64>(static_cast<u32>(day)) * 2654435761ull) ^ (static_cast<u64>(id + 1) << 40); }

// Arriving at a junction: how long they stop there.
float pauseAt(const NetNode& n, Rng& rng) {
    if (n.market) return 0.0f;                               // (the Market: walked through)
    if (n.end) return 40.0f + static_cast<float>(rng.below(41));  // a viewpoint: sit and look a while
    if (n.place >= 0 && rng.chance(1, 2)) return 8.0f + static_cast<float>(rng.below(9));  // a look about
    return 0.0f;
}

void nextLeg(const PathNet& net, Walk& w, bool first) {
    Rng rng(w.rng);
    const std::vector<int>& links = net.links[static_cast<std::size_t>(w.from)];
    int pick = links[0];
    if (links.size() > 1) {  // not straight back the way they came (unless it's a dead end)
        int options[16], n = 0;
        for (int e : links)
            if (e != w.prevEdge && n < 16) options[n++] = e;
        pick = n > 0 ? options[rng.below(static_cast<u32>(n))] : links[0];
    }
    const NetEdge& e = net.edges[static_cast<std::size_t>(pick)];
    const std::vector<Vec2> way = fromNode(e, w.from);
    const bool crossing = !first && !w.pts.empty() && w.prevEdge >= 0;
    const Vec2 here = crossing ? w.pts.back() : way.front();
    w.pts.clear();
    w.pts.push_back(here);
    if (crossing)  // (from where they stand, round the junction's walls, to this stretch's start)
        for (const Vec2& p : roam::crossing(net, w.from, w.prevEdge, pick))
            if (dist(p, w.pts.back()) > 0.05f) w.pts.push_back(p);
    for (const Vec2& p : way)
        if (dist(p, w.pts.back()) > 0.05f) w.pts.push_back(p);
    if (w.pts.size() < 2) w.pts.push_back(here);
    lengths(w.pts, w.cum);
    w.to = e.a == w.from ? e.b : e.a;
    w.edge = pick;
    w.prevEdge = pick;
    const Roamer& r = roamer(w.id);
    w.walkFor = w.cum.back() / r.pace;
    w.pauseFor = pauseAt(net.nodes[static_cast<std::size_t>(w.to)], rng);
    w.rng = rng.state();
    ++w.legs;
}

}  // namespace

const Roamer& roamer(int id) { return kTable[valid(id) ? id : 0]; }

void palette(int id, Rgb out[kPalCount]) { league::palette(roamer(id).look, out); }

// ---------------------------------------------------------------------------- who, when
int roster(s32 day, u8 out[kMaxOut]) {
    Rng rng(0xD0E1ull ^ (static_cast<u64>(static_cast<u32>(day)) * 0x9E3779B97F4A7C15ull));
    u8 order[kRoamers];
    for (int k = 0; k < kRoamers; ++k) order[k] = static_cast<u8>(k);
    for (int k = kRoamers - 1; k > 0; --k) {
        const int j = static_cast<int>(rng.below(static_cast<u32>(k + 1)));
        const u8 t = order[k];
        order[k] = order[j];
        order[j] = t;
    }
    const int n = kMinOut + static_cast<int>(rng.below(kMaxOut - kMinOut + 1));
    for (int k = 0; k < n; ++k) out[k] = order[k];
    return n;
}

bool isOut(s32 day, int id) {
    u8 ids[kMaxOut];
    const int n = roster(day, ids);
    for (int k = 0; k < n; ++k)
        if (ids[k] == id) return true;
    return false;
}

bool walkingHour(int hour) { return hour >= kOutFrom && hour < kOutUntil; }

float dayClock(s64 localUnix) {
    return static_cast<float>(localUnix - static_cast<s64>(dayIndex(localUnix)) * kDay - kOutFrom * kHour);
}

// ---------------------------------------------------------------------------- the network
Vec2 along(const std::vector<Vec2>& pts, const std::vector<float>& cum, float d, float* heading) {
    if (pts.empty()) return {};
    if (pts.size() == 1) return pts[0];
    std::size_t k = 1;
    while (k + 1 < pts.size() && cum[k] < d) ++k;
    const Vec2 a = pts[k - 1], b = pts[k];
    const float span = cum[k] - cum[k - 1];
    const float u = span > 1e-4f ? std::fmin(1.0f, std::fmax(0.0f, (d - cum[k - 1]) / span)) : 1.0f;
    if (heading && span > 1e-4f) *heading = headingOf(sub(b, a));
    return {a.x + (b.x - a.x) * u, a.y + (b.y - a.y) * u};
}

void buildNet(const Valley& v, const std::vector<Solid>& solids, PathNet& out) {
    out = PathNet{};
    SolidIndex walls;
    walls.build(solids);
    // Junctions: every path's ends, and every point another path shares.
    auto nodeAt = [&](Vec2 p) {
        for (std::size_t k = 0; k < out.nodes.size(); ++k)
            if (dist(out.nodes[k].at, p) < 1.0f) return static_cast<int>(k);
        return -1;
    };
    auto shared = [&](std::size_t path, Vec2 p) {
        for (std::size_t q = 0; q < v.paths.size(); ++q) {
            if (q == path) continue;
            for (const Vec2& o : v.paths[q])
                if (dist(o, p) < 1.0f) return true;
        }
        return false;
    };
    for (std::size_t k = 0; k < v.paths.size(); ++k) {
        const std::vector<Vec2>& path = v.paths[k];
        if (path.size() < 2) continue;
        for (std::size_t i = 0; i < path.size(); ++i) {
            if ((i == 0 || i + 1 == path.size() || shared(k, path[i])) && nodeAt(path[i]) < 0) {
                NetNode n;
                n.at = n.view = path[i];
                out.nodes.push_back(n);
            }
        }
    }
    // Stretches: each path cut at its junctions.
    for (const std::vector<Vec2>& path : v.paths) {
        if (path.size() < 2) continue;
        NetEdge e;
        e.a = nodeAt(path[0]);
        e.pts.push_back(path[0]);
        for (std::size_t i = 1; i < path.size(); ++i) {
            e.pts.push_back(path[i]);
            const int n = nodeAt(path[i]);
            if (n >= 0) {
                e.b = n;
                if (e.b != e.a) out.edges.push_back(e);
                e = NetEdge{};
                e.a = n;
                e.pts.push_back(path[i]);
            }
        }
    }
    out.links.assign(out.nodes.size(), {});
    for (std::size_t k = 0; k < out.edges.size(); ++k) {
        out.links[static_cast<std::size_t>(out.edges[k].a)].push_back(static_cast<int>(k));
        out.links[static_cast<std::size_t>(out.edges[k].b)].push_back(static_cast<int>(k));
    }
    // What each junction is: a path's end, near a place, in the Market.
    for (std::size_t k = 0; k < out.nodes.size(); ++k) {
        NetNode& n = out.nodes[k];
        n.end = out.links[k].size() == 1;
        float best = 45.0f;
        for (const ValleyPlaceInfo& p : v.places) {
            const float d = dist(n.at, {p.at.x, p.at.y});
            if (d < best) {
                best = d;
                n.place = p.id;
                n.view = {p.at.x, p.at.y};
            }
        }
        n.market = n.place == kPlaceMarket && best < 40.0f;
    }
    // A path's end at a place stops short of it: back along the way until it's well out from the
    // place's middle and clear of its walls, looking at it.
    for (std::size_t k = 0; k < out.nodes.size(); ++k) {
        NetNode& n = out.nodes[k];
        if (!n.end || n.place < 0) continue;
        NetEdge& e = out.edges[static_cast<std::size_t>(out.links[k][0])];
        std::vector<Vec2> way = fromNode(e, static_cast<int>(k));  // from the end back along it
        std::vector<float> cum;
        lengths(way, cum);
        const float want = std::fmax(10.0f, 0.3f * placeLayout(n.place).flat);
        float d = 0;
        while (d < cum.back() * 0.6f) {
            const Vec2 p = along(way, cum, d);
            if (dist(p, n.view) >= want && walls.clear(p, 1.5f)) break;
            d += 1.0f;
        }
        n.round = d;
    }
    // A junction in a place is walked round: the smallest reach from it at which the crossings
    // between every pair of its stretches, and the stretches for a way on from there, keep clear
    // of the walls (else the reach with the fewest in the way).
    for (std::size_t k = 0; k < out.nodes.size(); ++k) {
        NetNode& n = out.nodes[k];
        if (n.end || n.place < 0) continue;
        static constexpr float kReaches[] = {0.0f, 4.0f, 8.0f, 12.0f, 16.0f, 20.0f, 25.0f, 30.0f};
        int fewest = 1 << 30;
        for (float r : kReaches) {
            int bad = 0;
            std::vector<Vec2> ends;
            for (int ei : out.links[k]) {
                const std::vector<Vec2> way = fromNode(out.edges[static_cast<std::size_t>(ei)], static_cast<int>(k));
                std::vector<float> cum;
                lengths(way, cum);
                const float from = std::fmin(r, cum.back() * 0.45f);
                ends.push_back(along(way, cum, from));
                for (float d = from; d < std::fmin(from + 25.0f, cum.back() * 0.5f); d += 1.0f) bad += !walls.clear(along(way, cum, d), 0.4f);
            }
            for (std::size_t a = 0; a < ends.size(); ++a)
                for (std::size_t b = a + 1; b < ends.size(); ++b) bad += lineClear(walls, ends[a], ends[b], 0.6f) ? 0 : 4;
            if (bad < fewest) {
                fewest = bad;
                n.round = r;
            }
            if (bad == 0) break;
        }
    }
    // Each stretch trimmed by its ends' reach (a dead end's node moves to where it stops).
    for (NetEdge& e : out.edges) {
        std::vector<float> cum;
        lengths(e.pts, cum);
        const float len = cum.back();
        float ra = out.nodes[static_cast<std::size_t>(e.a)].round, rb = out.nodes[static_cast<std::size_t>(e.b)].round;
        if (ra + rb > len - 2.0f) {  // (a short stretch between two rounded junctions: keep its middle)
            const float s = (len - 2.0f) / std::fmax(0.01f, ra + rb);
            ra *= std::fmax(0.0f, s);
            rb *= std::fmax(0.0f, s);
        }
        e.pts = cut(e.pts, ra, len - rb);
        keepClear(e.pts, walls);
        lengths(e.pts, e.cum);
        e.length = e.cum.back();
    }
    for (std::size_t k = 0; k < out.nodes.size(); ++k) {
        NetNode& n = out.nodes[k];
        if (!n.end || out.links[k].empty()) continue;
        const NetEdge& e = out.edges[static_cast<std::size_t>(out.links[k][0])];
        n.at = e.a == static_cast<int>(k) ? e.pts.front() : e.pts.back();
    }
    // The crossings round each junction walked round: straight across, nudged round the walls.
    for (std::size_t k = 0; k < out.nodes.size(); ++k) {
        NetNode& n = out.nodes[k];
        const std::vector<int>& links = out.links[k];
        for (std::size_t a = 0; a < links.size(); ++a)
            for (std::size_t b = a + 1; b < links.size(); ++b) {
                auto endAt = [&](int ei) {
                    const NetEdge& e = out.edges[static_cast<std::size_t>(ei)];
                    return e.a == static_cast<int>(k) ? e.pts.front() : e.pts.back();
                };
                const Vec2 p = endAt(links[a]), q = endAt(links[b]);
                n.cross.push_back(dist(p, q) > 0.05f ? routeAround(p, q, walls) : std::vector<Vec2>{p, q});
            }
    }
}

std::vector<Vec2> crossing(const PathNet& net, int node, int from, int to) {
    if (node < 0 || node >= static_cast<int>(net.nodes.size())) return {};
    const std::vector<int>& links = net.links[static_cast<std::size_t>(node)];
    int ia = -1, ib = -1;
    for (int k = 0; k < static_cast<int>(links.size()); ++k) {
        if (links[static_cast<std::size_t>(k)] == from) ia = k;
        if (links[static_cast<std::size_t>(k)] == to) ib = k;
    }
    if (ia < 0 || ib < 0 || ia == ib) return {};
    const int a = ia < ib ? ia : ib, b = ia < ib ? ib : ia, count = static_cast<int>(links.size());
    const int pair = a * (2 * count - a - 1) / 2 + (b - a - 1);  // (pairs in order: (0,1), (0,2) .. (1,2) ..)
    const std::vector<std::vector<Vec2>>& cross = net.nodes[static_cast<std::size_t>(node)].cross;
    if (pair < 0 || pair >= static_cast<int>(cross.size())) return {};
    const std::vector<Vec2>& c = cross[static_cast<std::size_t>(pair)];
    if (ia < ib) return c;
    return std::vector<Vec2>(c.rbegin(), c.rend());
}

// ---------------------------------------------------------------------------- a day's walk
void startWalk(const PathNet& net, int id, s32 day, Walk& w) {
    w = Walk{};
    w.id = valid(id) ? id : 0;
    w.day = day;
    if (!net.ok()) return;
    Rng rng(seedOf(day, w.id));
    // Somewhere out on the paths (not in the Market), setting off.
    int start = 0;
    for (int tries = 0; tries < 64; ++tries) {
        start = static_cast<int>(rng.below(static_cast<u32>(net.nodes.size())));
        if (!net.nodes[static_cast<std::size_t>(start)].market && !net.links[static_cast<std::size_t>(start)].empty()) break;
    }
    w.rng = rng.state();
    w.from = start;
    w.legStart = 0;
    nextLeg(net, w, true);
}

Pose walkAt(const PathNet& net, Walk& w, float t) {
    Pose p;
    if (!net.ok() || w.pts.empty()) return p;
    if (t < w.legStart) startWalk(net, w.id, w.day, w);  // (the clock went back: from the morning again)
    for (int guard = 0; guard < 200000 && t >= w.legStart + w.walkFor + w.pauseFor; ++guard) {
        w.legStart += w.walkFor + w.pauseFor;
        w.from = w.to;
        nextLeg(net, w, false);
    }
    const Roamer& r = roamer(w.id);
    const float into = t - w.legStart;
    if (into < w.walkFor) {
        p.walking = true;
        p.speed = r.pace;
        p.at = along(w.pts, w.cum, into * r.pace, &p.heading);
        return p;
    }
    // Paused at the far end: looking at the place there (or on the way they were going).
    const NetNode& n = net.nodes[static_cast<std::size_t>(w.to)];
    p.at = w.pts.back();
    along(w.pts, w.cum, w.cum.back(), &p.heading);
    if (n.place >= 0 && dist(n.view, p.at) > 1.0f) p.heading = headingOf(sub(n.view, p.at));
    p.node = w.to;
    const float held = into - w.walkFor, left = w.pauseFor - held;
    p.sitting = n.end && w.pauseFor > 30.0f && held > 5.0f && left > 5.0f;
    return p;
}

// ---------------------------------------------------------------------------- a duel
int duelLevel(int partnerLevel, int id) {
    const int l = partnerLevel + roamer(id).edge;
    return l < 1 ? 1 : (l > kLevelCap ? kLevelCap : l);
}

Dragon dragonOf(int id, int level) {
    const Roamer& r = roamer(id);
    int kind = findKind(r.kind);
    if (kind < 0) kind = 0;
    Rng rng(0x60A3E500u + static_cast<u32>(id) * 7919u);
    Dragon d;
    d.id = 0xB1000000u + static_cast<u32>(valid(id) ? id : 0);  // apart from the save's and the league's
    d.stage = Stage::Adult;
    rollKind(d, kind, r.variant, rng);
    d.genome.build = static_cast<u8>(rng.below(3));
    d.genome.size = static_cast<u8>(rng.below(256));
    d.xp = trainer::xpForLevel(level < 1 ? 1 : level);
    const int trained = level / 4 < kMaxTrained ? level / 4 : kMaxTrained;  // (a keeper's dragon trains about as much)
    for (u8& t : d.trained) t = static_cast<u8>(trained);
    std::snprintf(d.name, sizeof(d.name), "%s", r.dragonName);
    d.needs = Needs{};
    d.bond = 600;
    return d;
}

u32 duelGleam(int level) {
    const u32 g = 8u + static_cast<u32>(level > 0 ? level : 0) / 2u;
    return g < kGleamCap ? g : kGleamCap;
}

bool paidToday(const SaveData& s, int id, s32 today) {
    return valid(id) && s.progress.roamDay == today && ((s.progress.roamPaid >> id) & 1u);
}

Reward record(SaveData& s, int dragonIndex, int id, int foeLevel, battle::Outcome o, s32 today) {
    Reward r;
    if (!valid(id) || dragonIndex < 0 || dragonIndex >= s.dragonCount) return r;
    Dragon& d = s.dragons[dragonIndex];
    // A friendly duel teaches a little less than a league battle.
    r.growth = battle::grow(d, battle::battleXp(trainer::levelOf(d), foeLevel, o, 0.8f));
    if (o == battle::Outcome::GaveUp) return r;
    trainer::count(s, kCountBattles);
    const bool won = o == battle::Outcome::Won;
    trainer::recordBattle(d, won);
    if (!won) return r;
    if (s.progress.duelsWon < 0xFFFF) ++s.progress.duelsWon;
    if (s.progress.roamDay != today) {  // a new day: every trainer's first win pays again
        s.progress.roamDay = today;
        s.progress.roamPaid = 0;
    }
    const u8 bit = static_cast<u8>(1u << id);
    if (s.progress.roamPaid & bit) {
        r.paidBefore = true;
    } else {
        s.progress.roamPaid = static_cast<u8>(s.progress.roamPaid | bit);
        r.gleam = duelGleam(foeLevel);
        s.gleam += r.gleam;
    }
    return r;
}

}  // namespace ec::roam
