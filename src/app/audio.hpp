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
    Count
};

// Looping beds under the music (the den's hearth and night outside, an egg's hum). Each
// fades toward the level last set; set it every frame it's wanted, or it fades out.
enum class Bed : u8 { Hearth, Night, EggHum, Count };
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

void playSfx(Sfx s, float pitch = 1.0f);
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
