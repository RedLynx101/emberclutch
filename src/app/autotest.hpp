// Dev builds: scripted runs, for checking the screens with nobody at the controls. If
// sdmc:/3ds/emberclutch/autotest.txt exists, its commands stand in for the buttons and the
// stylus, and "shot <name>" saves both screens as BMPs in sdmc:/3ds/emberclutch/shots/.
// tools/autotest.ps1 runs a script in Azahar and collects the pictures as PNGs.
//
// Commands, one per line (# starts a comment):
//   wait <s>                    nothing for s seconds
//   tap <x> <y>                 a quick tap on the bottom screen
//   hold <x> <y> <s>            the stylus held still
//   drag <x0> <y0> <x1> <y1> <s> [<hold>]  a stroke (then held still at its end), then lift
//   key <A|B|X|Y|L|R|START|SELECT|UP|DOWN|LEFT|RIGHT>   one press
//   keyhold <key> <s>           held down (keys together: UP+B)
//   shot <name>                 both screens, as drawn this frame
//   shotin <s> <name>           a shot s seconds into the next command (mid-stroke, mid-hold)
//   name <text>                 what the keyboard returns next (no keyboard is shown)
//   skip <hours>                the dev clock skips ahead
//   overlay <on|off>            the dev budget overlay
//   travel <place>              in the valley: go to a place (core/valley ValleyPlace)
//   light                       every festival lantern lit
//   view <place> <ex> <ey> <ez> <tx> <ty> <tz>   the valley's free camera at a place (its frame; z
//                               above its anchor), looking at a point
//   creator                     your look (the creator), back to this scene after
//   wander <steps>              the dragon cared for sets off on the Wanderings; that many steps walked
//   festival                    the Lantern Festival's eve (every other quest done, the lanterns lit)
//   goto <x> <y>                in the valley: stand there (metres), your partner called
//   challenge <c> <cup>         (in the valley) straight into a challenge's cup (core/world Challenge:
//                               0 Fruit Catch, 1 Sky Rings, 2 Lantern Trial; cup 1 Ember .. 4 Starfire)
//   autoplay <on|off>           the challenges play themselves (the pilot flies, the lanterns, throws)
//   cups <fruit> <rings> <lantern>   the highest cup won in each (their ribbons too)
//   battle <what> ...           1.0 battles (app/battle_feature.hpp battleCommand): start <league>
//                               <slot>, talk <league> <slot>, hollow <floor>, board, level <n>, league <won> [beaten
//                               bits], energy <n>, auto on|off (autoplay on also plays battles)
//   quit                        leave (writes shots/done.txt)
#pragma once

#include <cstddef>

#include "app/app.hpp"

namespace ec::autotest {

bool start(App& app);  // true if a script was found and loaded (dev builds only)
bool active();
Input next(App& app);  // this frame's input, from the script
// Around each frame: which framebuffers this frame's picture lands in, then (after the next
// C3D_FrameBegin has waited for it) saving any picture asked for.
void beforeFrameEnd();
void afterFrameBegin();
// While a script runs, the keyboard isn't shown: this is the name it "types" (false: none
// queued, take the suggestion).
bool typedName(char* out, std::size_t cap);
void finish();  // writes shots/done.txt
// True on a frame whose picture will be saved; log() then adds a line to shots/log.txt
// (for numbers behind a picture: camera framing, positions).
bool shooting();
void log(const char* fmt, ...);

}  // namespace ec::autotest
