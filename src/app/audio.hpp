// Audio: streamed Ogg music with sample-accurate loops (LOOPSTART tag written by
// tools/audio/make_loop.py) plus preloaded sound effects. See architecture section 6.
#pragma once

#include <3ds/types.h>

namespace ec::audio {

// Sound effects: romfs:/sfx/<slug>.wav plus any takes <slug>-2.wav .. -4.wav, played in turn
// (tools/audio/process_sfx.py writes them; the slugs are docs/audio/suno-sfx-alpha1.md's).
enum class Sfx : u8 {
    Tap, Confirm, Back, Error, Toast, Save,                  // interface
    Chirp, Trill, Purr, Squeak, Whimper, Yawn, Sneeze, Rumble,  // dragon voice (pitched per dragon)
    Step, Thump, Flap,                                       // body
    EggKnock, EggCrack, EggHatch,                            // egg
    Munch, Gulp, Brush, Sparkle, Splash, Bounce,             // care
    // Sound brief 2 (docs/audio/sfx-batch-2.md): each has a stand-in, retuned, until its own
    // file arrives (D35); drop romfs/sfx/<slug>.wav in and it plays instead.
    BallRoll, BallPickup, Grumble, Sniff, Giggle, LegKick, TubSlide, Suds, WaterPour, ShakeSpray,
    EggHeartbeat, EggTurn, HatchCry,                         // hands-on care
    RopeTug, FeatherFlutter, OrbRattle, TreatDrop, BowlClink, NestSettle, EggLay, Coin, Register,
    MapOpen, TravelWhoosh, TrailDepart,                      // toys, den and places (Alpha 2)
    // Sound brief 3 (docs/audio/sfx-batch-3.md), the valley and flying: its other sounds are in
    // romfs/sfx already and get a slot as the work that needs them arrives.
    Wingbeat, Takeoff, Landing, DiveWhoosh, WaterSkim, SplashBig, DragonStep,
    // Beta 1: you on foot, the places, the festival, the challenges
    StepGrass, StepStone, StepWood, Mount, FindSparkle, LanternLight, LanternRelight, DoorWood, QuestPage,
    BreathFlame, BreathFrost, BreathGust, BreathLight, BreathMist, BreathSpores, VillageBell, CrowdCheer, CrowdAww,
    WhistleStart, RingPass, FruitToss, FruitCatch, StarShimmer,
    Count
};
// True once a sound's own file is loaded (not a stand-in).
bool has(Sfx s);

// Looping beds under the music (the den's hearth and night outside, an egg's hum). Each
// fades toward the level last set; set it every frame it's wanted, or it fades out.
// The valley's: the wind high up, the meadow by day, the night, the lake, and the wings
// fluttering in a glide.
enum class Bed : u8 { Hearth, Night, EggHum, Market, WindHigh, Meadow, ValleyNight, Lake, WingFlutter, Stream, Waterfall,
                      Village, Count };
void setBed(Bed b, float level);  // 0..1

// Returns false (and stays silent) if the DSP can't start, e.g. no sdmc:/3ds/dspfirm.cdc.
bool init();
void shutdown();
bool ok();

// Fade to `slug` (romfs:/music/<slug>.ogg). Same track again = no-op. nullptr = silence.
void playMusic(const char* slug);
const char* currentMusic();

// A one-shot music cue (e.g. the hatching stinger) that ducks the loop, then resumes it.
void playStinger(const char* slug);

void playSfx(Sfx s, float pitch = 1.0f, float gain = 1.0f);  // gain: 0..1 of the SFX volume
void setVolumes(u8 music, u8 sfx);  // 0..100

// Call once per frame from the main thread (fades, volume).
void update(float dt);

// Dev overlay: streaming-thread loop count, track switches, and stage
// (0 idle, 1 opening, 2 open, 3 open failed, 4 decoding).
struct DebugInfo {
    u32 loops, switches;
    int stage;
    float gain;
};
DebugInfo debugInfo();

}  // namespace ec::audio
