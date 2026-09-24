// Audio: streamed Ogg music with sample-accurate loops (LOOPSTART tag written by
// tools/audio/make_loop.py) plus preloaded sound effects. See architecture section 6.
#pragma once

#include <3ds/types.h>

namespace ec::audio {

enum class Sfx : u8 { Tap, Confirm, Back, Munch, Brush, Purr, Chirp, Crack, HatchPop, Step, Thump, Flap, Yawn, Count };

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
