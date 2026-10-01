// The valley's small life (1.0, workstream L): songbirds pecking in the meadows, rabbits at the
// woods' edges (snow hares up in the cold north), butterflies over the flowers, frogs at the
// shallows, ducks paddling the lake in a line, and a fox at dusk. They're spawned round you from
// the ground they suit (a seeded cell grid, so the same spot holds the same critters), live on
// their own (peck, nibble, hop, croak, paddle), scatter when you or your dragon come at them, and
// each has one A moment: a songbird hops over at a whistle, a rabbit leads your dragon a merry
// chase (it always gets away), a butterfly lands on your dragon's head, a frog croaks back, the
// ducks paddle over, the fox boops noses with your dragon. Befriending one is a Journal entry and
// a pinch of Play, Love or bond (paid a few times a day, no farming), a little Gleam for the
// day's first of each kind. Nobody is ever hurt; every chase ends with the critter escaping.
//
// Pure logic (PC-tested in tests/test_critters.cpp): the behaviour, the moments, the rewards and
// the meshes (a handful of vertex-coloured triangles each, the light baked in), built each frame
// as one triangle list for a single draw (app/wildlife, app/render_critters.inc).
#pragma once

#include "core/math3d.hpp"
#include "core/types.hpp"

namespace ec {

struct Valley;
struct Dragon;
struct SaveData;

namespace critters {

enum class Kind : u8 { Songbird, Rabbit, SnowHare, Butterfly, Frog, Duck, Fox, Count };
constexpr int kKinds = static_cast<int>(Kind::Count);

constexpr int kMaxCritters = 32;
constexpr int kMaxTris = 600;          // the whole pass in a frame (the old 3DS: the valley runs 4-10k)
constexpr float kLiveRadius = 46.0f;   // groups spawn within this of you
constexpr float kDropRadius = 62.0f;   // and go past this
constexpr float kCell = 20.0f;         // the spawn grid, metres

enum class State : u8 {
    Idle,    // pecking, nibbling, sitting, paddling about, fluttering over the flowers
    Move,    // a few hops (or a paddle, a trot) to somewhere near its home
    Flee,    // off in a hurry: birds take wing, rabbits zigzag for cover, the fox runs for the woods
    Leap,    // a frog's jump into the water
    Come,    // hopping or paddling up to you (a whistle, a call), or to your dragon (the fox, a butterfly)
    Visit,   // close by: a bird's head tilt, the ducks' bob, the fox sniffing your dragon's nose
    Perch,   // a butterfly on your dragon's head
    Chased,  // running from your dragon in its chase (always a little too quick for it)
    Soar,    // a songbird circling high over the meadows
    Gone,    // out of sight (in a bush, under the water, flown off): its splash fades, then it's freed
};

struct Critter {
    bool alive = false;
    Kind kind = Kind::Songbird;
    State state = State::Idle;
    u8 slot = 0;        // its place in its group (the mother duck is 0, her ducklings follow in line)
    u8 variant = 0;     // its colouring
    u32 cell = 0;       // the spawn cell it came from (its group)
    u32 rng = 1;        // its own random numbers
    Vec3 pos;           // its feet (a bird's or butterfly's body in the air), metres
    Vec3 home;          // where it keeps to
    Vec3 goal;          // where it's going
    Vec3 from;          // where the hop (or leap) began
    float heading = 0;  // radians about Z; 0 faces -Y (as the dragons)
    float speed = 0;    // m/s
    float air = 0;      // off the ground (a hop, a leap), metres
    float t = 0;        // seconds in this state
    float timer = 0;    // to its next little act
    float born = 0;     // seconds alive (it grows in over the first moment: no popping)
    float anim = 0;     // its own clock offset
    float puff = 0;     // a frog's throat, a bird's head tilt (0..1)
    float zig = 1;      // the zigzag's side (+1 / -1)
};

// What's about this frame (the scene fills it).
struct Around {
    Vec3 you;
    float youSpeed = 0;
    bool riding = false;    // on your grown partner (walking or flying): `pal` is where you both are
    bool hasPal = false;
    Vec3 pal;
    float palSpeed = 0;
    float palHeading = 0;
    Vec3 palHead;           // its head as last drawn (a butterfly's perch)
    bool palHeadSet = false;
    float day = 1, dusk = 0, night = 0;  // the time of day's weights (core/daylight), summing to 1
    bool quiet = false;     // no new ones (another feature has the valley: a battle, a show)
};

// A pressed near a critter: what happens.
enum class Act : u8 { None, Whistle, Chase, Still, CroakBack, Call, Quiet };
// The partner's part in a moment (the scene plays the clip): creeping up, the pounce, the run,
// sitting still, the sneeze a butterfly gives it, sniffing a fox's nose, happy after.
enum class PalMove : u8 { Free, Stalk, Pounce, Run, Sit, Sneeze, Sniff, Happy };

struct Moment {
    Act act = Act::None;
    int who = -1;          // the critter
    float t = 0;           // seconds in
    float mark = -1;       // when its turn came (a bird arrived, the butterfly landed, the fox reached it)
    bool takesPal = false; // your partner's in it: the scene hands it over (app/valley_ext)
    Vec3 pal;              // where the partner is (moved by the moment while it takes it)
    float palHeading = 0;
    PalMove move = PalMove::Free;
    bool befriended = false;
    bool active() const { return act != Act::None; }
};

// Things for the scene to hear and show, this frame.
enum class Ev : u8 {
    Chirp, Flutter, Hop, Croak, Splash, Quack, Rustle, Yip, Shimmer,  // the critters
    YouWhistle, YouCroak,  // you, starting a moment
    Seen,                  // a kind seen close for the first time this visit (the Journal marks it)
    Befriend,              // a moment's heart: the Journal and the rewards (befriend())
};
struct Event {
    Ev ev = Ev::Chirp;
    Kind kind = Kind::Songbird;
    Vec3 at;
    float pitch = 1;  // a duckling's peep is a quack pitched up
};
constexpr int kMaxEvents = 12;

struct Life {
    Critter c[kMaxCritters];
    u32 seed = 0xC0FFEE;
    float clock = 0;           // seconds (the animation)
    float lookFor = 0;         // to the next look round for cells to fill
    Vec3 last;                 // you, last frame (a long jump: a trip, everything round you anew)
    bool started = false;
    u32 emptied[16] = {};      // cells whose critters left (not refilled until you've gone away)
    int emptiedCount = 0;
    u8 seen = 0;               // kinds seen close this visit (a bit each)
    float soarFor = 0;         // to the soaring pair's return
    Moment moment;
    Event events[kMaxEvents];  // this frame's (begin()'s are kept for the next update's)
    int eventCount = 0;
    int reported = 0;          // (the ones the last update gave)
};

// ---- The ground: what's at a spot, for who lives there.
struct Ground {
    float height = 0;
    bool water = false;   // under the surface
    bool deep = false;    // deep enough to paddle (ducks)
    bool shore = false;   // dry, just above the water, water within a stride (frogs)
    bool snow = false;
    bool grass = false;
    bool earth = false;   // a path, sand, bare ground
    bool steep = false;
    bool flowers = false; // a flower patch within a few metres
    bool cover = false;   // a bush or a tree near (rabbits, the fox)
    bool nearPlace = false;  // a place's buildings (no land critters right by them)
};
Ground groundAt(const Valley& v, float x, float y);
// The kind (and group size) the cell holds at this time of day (Count: none), and its spot.
Kind cellKind(const Valley& v, u32 seed, int cx, int cy, float day, float dusk, float night, Vec3& spot, int& groupSize);

// ---- Living.
// Empties it (a new visit): the next update fills the cells round you.
void reset(Life& life, u32 seed);
// One frame (dt seconds, capped): spawns and drops, everyone's behaviour, scaring, the moment.
// Fills life.events.
void update(Life& life, const Valley& v, const Around& a, float dt);
// Places one of `kind` (and its group) a little ahead of you, on ground it suits within `within`
// metres (the autotest's `critters spawn`); returns its index or -1.
int spawnNear(Life& life, const Valley& v, Kind kind, Vec3 you, float heading, float within = 30.0f);

// ---- The A moment.
struct Offer {
    int who = -1;
    Act act = Act::None;
    float distance = 0;
};
// What A would do here, facing `forward` (the nearest critter in reach that's in front of you,
// calm, and willing: rabbits and the butterfly's landing want your dragon there).
Offer offer(const Life& life, Vec3 you, Vec3 forward, bool hasPal);
// A pressed: begins it (false if the critter's gone or it can't be done now).
bool begin(Life& life, const Valley& v, const Offer& o, const Around& a);
void endMoment(Life& life);  // (an interruption: the critters go back to their own business)

// ---- Rewards and the Journal (SaveData::progress: seen, friends, the day's rewards).
constexpr int kDailyPaid = 6;      // moments a day that pay your partner
constexpr int kGleamDaily = 3;     // the first of each kind befriended in a day
constexpr int kGleamFirst = 10;    // the first of each kind ever
struct Reward {
    bool firstEver = false;
    bool firstToday = false;
    bool paid = false;   // the partner's pinch (false once the day's are used up)
    int gleam = 0;
    float play = 0, love = 0;
    int bond = 0;
};
Reward befriend(SaveData& s, Dragon* partner, Kind kind, s32 today);
void markSeen(SaveData& s, Kind kind);
bool seen(const SaveData& s, Kind kind);
bool befriended(const SaveData& s, Kind kind);
int friendCount(const SaveData& s, Kind kind);   // times befriended (to 255)
int paidToday(const SaveData& s, s32 today);     // moments paid today
Act actFor(Kind kind);

// ---- The meshes: every critter in view (within its kind's reach of the eye and in front of it),
// nearest first, as a triangle list (three vertices each), at most kMaxTris.
struct Mesh {
    Vec3 pos[kMaxTris * 3];
    u8 col[kMaxTris * 3 * 4];
    int verts = 0;
    int shown = 0;  // critters drawn
    int tris() const { return verts / 3; }
};
void buildMesh(const Life& life, Vec3 eye, Vec3 target, Mesh& out);
// Custard (D138): Bram's fluffy sheepdog, added to a frame's mesh by the story (app/feature_story):
// standing with his tail going (wag 0..1 how hard), or flopped down (sit 1); about 130 triangles.
struct DogPose {
    Vec3 at;
    float heading = 0;  // (0 faces -Y, as the critters)
    float wag = 0.5f, sit = 0, clock = 0;
};
void addDog(Mesh& m, const DogPose& p);
// Triangles one of a kind takes (the budget), and a splash ring's.
int trianglesOf(Kind kind);
constexpr int kRingTris = 16;

}  // namespace critters
}  // namespace ec
