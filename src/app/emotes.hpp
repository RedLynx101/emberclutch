// Feelings shown (D137, the Living Valley pass): the little icons that pop over someone's head and by
// their portrait when a line carries a feeling (Animal Crossing's way: a heart, a "!", sweat drops, a
// cloud of gloom, zzz ...), drawn with citro2d's shapes; their sound; and how a feeling tunes the
// voiced letters (higher and quicker when happy, lower and slower when sad, a shake when scared).
#pragma once

#include "app/app.hpp"
#include "app/render3d.hpp"
#include "core/story.hpp"

namespace ec::emote {

// Whether a feeling pops an icon at all (calm, cool and wistful don't).
bool hasIcon(story::Feel f);
// The icon at (x, y), `size` pixels across, `t` seconds since it popped (it pops in, bobs, then
// holds; alpha fades it). Top or bottom screen, inside a 2D pass.
void draw(App& app, story::Feel f, float x, float y, float size, float t, float alpha = 1.0f);
// Its sound as it pops (none for some).
void play(story::Feel f);

struct Voice {
    float pitch = 1.0f;   // times the speaker's pitch
    float speed = 1.0f;   // times the letters' pace
    float wobble = 0.0f;  // a shake in the pitch (scared, crying, dizzy)
    float volume = 0.8f;
};
Voice voiceOf(story::Feel f);

// The story person saying the line now, on their figure (D138): the face takes the line's feeling
// (the mouth moving while its letters come; the eyes popping, pulsing, squinting or blinking slow by
// the feeling) and it's marked speaking. False (nothing changed) if they aren't the one speaking.
bool speakingFigure(const App& app, int person, r3d::PersonView& p);
// The line's feeling in the body: a clip to play once as the line starts (null: none, just talking).
const char* clipFor(story::Feel f, int person);
// The feeling over the speaking figure's head (top screen, after the 3D; nothing if none was drawn).
void drawOverSpeaker(App& app);

// The portrait a feeling shows (romfs/portraits/<id>.t3x frames): 0 calm, 1 happy, 2 sad, 3 angry, 4 surprised.
int portraitFrame(story::Feel f);

}  // namespace ec::emote
