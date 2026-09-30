# Emberclutch — Game Design Document

Status: **v0.2 — reviewed** (2026-09-23; review answers in §17 and the decision log)
Owner: Noah Hicks · Platform: Nintendo 3DS homebrew (old 3DS is the performance floor)

Emberclutch is a dragon-raising life sim in the spirit of a pocket pet game: you hatch,
raise, train, breed and fly with dragons that live in real time on your 3DS. Babies are
irresistibly cute; adults are majestic. Nothing ever dies — neglected dragons get upset,
not hurt.

> Companion docs: [Breeds & Genetics](breeds-and-genetics.md) ·
> [Theme & Art Direction](theme-and-art-direction.md) ·
> [Technical Architecture](../tech/architecture.md) · [Roadmap](../plan/roadmap.md) ·
> [Decision Log](../plan/decisions.md) · [Future: Equine Line](../future/equine-line.md)

---

## 1. Pillars

1. **A bond you can feel.** Touch, voice and daily care are the core verbs. The dragon
   notices you, remembers you, and reacts.
2. **Watch it grow.** Every day the dragon is a little bigger. Egg → Adult takes about
   a week of real time (a day and a half in the egg, 5.5 days to grown with the best care: D136),
   driven by care, not grinding.
3. **Every clutch is a surprise.** Breeding produces visibly different dragons. You build
   a den full of dragons that are *yours*.
4. **Forgiving, never punishing.** No death, no loss, no running away. Neglect shows up as
   an upset dragon you win back.
5. **Built for the hardware.** Stylized, readable, and smooth on an old 3DS. Short
   sessions (2–10 min) are always worth it.

## 2. Core loops

| Loop | Length | What happens |
|---|---|---|
| **Check-in** | 1–3 min | Greet, feed, pet, groom. Heartglows brighten. Collect eggs. |
| **Session** | 5–20 min | Train a trick, enter a competition, ride out over the valley, tend eggs. |
| **Daily** | 1 real day | Each dragon earns 0–3 *care stars*. Stars + days drive growth. |
| **Long** | weeks | Raise to adult, breed new variants, fill the den and Sanctuary, win cups. |

## 3. The dragon

### 3.1 Life stages

Stages advance when **both** gates are met: minimum real time since hatching (from the
3DS clock) **and** total care stars earned. With good care, the day gate is the limiter;
with patchy care, growth slows down but never reverses.

| Stage | Min time | Care stars | Size | Unlocks |
|---|---|---|---|---|
| **Egg** | 1.5 days warm | — | — | Rub to warm, turn, listen. Hatches in the den nest. |
| **Hatchling** | 0 | — | 25% | Feeding, petting, naming by voice, first tricks (Sit, Roar). |
| **Juvenile** | 1.5 days | 3 | 45% | Ground tricks, Fruit Catch (hop version), Command Trial, walks. |
| **Adolescent** | 3.25 days | 7 | 70% | Gliding, short flights, Sky Rings, full Fruit Catch, Shine Show, breath puff. |
| **Adult** | 5.5 days | 12 | 100% | Full flight, **riding**, Lantern Trial, breeding, Wanderings finds improve. |

(D136, 2026-09-30: was 1 day in the egg and 4 / 8 / 14 days with 6 / 14 / 26 stars. A dragon earns at
most 3 stars a day and about five days close in 5.5, so 12 stars needs near-perfect care.)

**Continuous growth:** within a stage the dragon grows smoothly each day (bone scales and
proportions interpolate), so a player sees change on every check-in. Proportions follow a
"cute → majestic" curve: head and eye scale shrink relative to body; neck, tail, horns
and wingspan lengthen.

### 3.2 Needs

Four needs, each 0–100. Kept few on purpose so the bottom-screen HUD stays readable.

| Need | Refilled by | Drains ~ |
|---|---|---|
| **Belly** | Feeding (favorite food fills more) | 6 / hour (3 while asleep) |
| **Energy** | Sleeping at night; tired dragons also nap on their own in the den (below 25, until 70) | 3 / hour awake |
| **Shine** | Grooming (brush, polish, bath) | 2 / hour, faster after flying/digging |
| **Play** | Toys, tricks, competitions, riding, other dragons | 4 / hour |

Eggs have a single need, **Warmth**, refilled by rubbing the egg on the touch screen or
setting it in a nest near an adult's heartglow.

### 3.3 Mood and the heartglow

Mood is derived from the needs (weighted average, lowest need counts double) plus
recent interaction. It is shown diegetically by the **heartglow** — the ember light in
every dragon's chest.

| Mood | Heartglow | Behaviour |
|---|---|---|
| **Joyful** | Bright, quick pulse, sparks | Follows the stylus, learns tricks faster, bonus in competitions |
| **Content** | Steady glow | Normal |
| **Restless** | Flickering | Paces, nudges the screen, obeys less reliably |
| **Sulky** | Dim | Sighs, lies down, slow to respond |
| **Upset** | Dim, turned away | Ignores commands and won't compete or be ridden until you make up |

**Forgiving but upset.** A dragon becomes *Upset* after its mood stays Sulky for 24
hours, or after 3+ days without a visit. It never dies, leaves, or loses progress. An upset
dragon **retreats to the den's sulk nook** (a shadowy corner) and won't play with the other
dragons until you make up.
**Making up** is a short, sweet interaction: approach slowly (hold the stylus still near
it), offer its favorite food, then pet until the heartglow re-lights. Long absences
(7+ days) trigger a special "I missed you" greeting after making up.

### 3.4 Bond

Long-term affection toward the player, 0–1000. Rises with daily care, petting, voice
recognition successes, riding and competing; never decays below its high-water mark
minus 100 (forgiving). Gates: breeding (≥ 300), riding confidence, top-tier cups.

### 3.5 Personality

Each dragon rolls one personality at hatch. Personalities only color behavior and
small modifiers — never a "bad" roll.

| Personality | Flavor | Modifier |
|---|---|---|
| Brave | Stands tall, roars at thunder | +Sky Rings |
| Shy | Peeks from behind wings | Bond grows faster from gentle petting |
| Playful | Chases the stylus | Play drains slower from toys |
| Proud | Poses, preens | +Shine Show |
| Sleepy | Yawns, naps in sunbeams | Energy recovers faster |
| Curious | Sniffs everything | Better Wanderings finds |

Each dragon also has a **favorite food** (rolled at hatch, biased toward its breed).

### 3.6 Sex

Every dragon is **male or female**, rolled 50/50 when the egg is laid and revealed at
hatching. **Breeding needs one male and one female.** Sex has no effect on stats or care.
Visual differences are subtle (D23): males have bigger horns and crest; females a longer
tail fan and brighter accent sheen.

## 4. Care interactions (touch-first)

Hands-on like *Nintendogs*: the tool you hold is drawn where you touch and the dragon
reacts to exactly where and how. The full spec: [care interactions](care-interactions.md).

- **Pet:** stroke with the stylus. Head scratches, chin rubs, and belly rubs each have
  distinct reactions; the heartglow pulses under your stylus.
- **Feed:** drag food from the pouch to its mouth. Favorite food gets a happy wiggle.
- **Groom:** brush (Shine), polish scales (sparkles), splash bath (Tide dragons love it,
  Ember dragons hate it — personality moment).
- **Play:** toss a ball or fruit, dangle a feather, tug rope.
- **Call:** say its name into the mic; it looks at you and comes over.
- **Sleep:** at night (22:00–07:00 local) dragons sleep in the den. Pet a sleeping dragon
  and it purrs and curls tighter.
- **Egg care:** rub to warm (Warmth), gently turn it, tap to hear it wiggle. Blow into
  the mic to cool an overheated egg (eggs by an Ember dragon can overheat — small joke,
  no penalty).

## 5. Training and voice

- **Tricks** are learned by doing, then naming: coax the pose with a touch gesture, then
  record a voice command. Repetition raises the trick's skill (0–100).
- **Voice recognition** is on-device template matching of the player's own recordings
  (like classic pet games). It is always **optional**: every command also has a
  **cue button** on the bottom screen, so the game is fully playable without the mic.
- Trick list (unlock stage in brackets): Sit [H], Roar [H], Lie Down [J], Spin [J],
  Beg [J], Wing Spread [J], Tail Chase [J], Fly Up [Adol], Breath Puff [Adol],
  Loop-de-loop [Adult], Breath Flare [Adult], Bow [Adult].

### 5.1 Stats

Five stats (D78), grown by care, training and competing, from each breed's base values
and each dragon's own potential ([dragons, version 2](dragons-v2.md) §5).

- **Wing** — flight speed and agility. Grows from flying and Sky Rings.
- **Wit** — obedience and trick reliability. Grows from tricks and Command Trial.
- **Might** — strength: carrying, tugging, Fruit Catch.
- **Breath** — the power of its element's breath (was *Spark*). Grows from Lantern Trial,
  Shine Show, breath practice.
- **Stamina** — endurance: how long it flies, plays and works.

Competition performance = stat × mood multiplier × bond confidence, plus player skill.

## 6. Competitions (never ridden)

*Beta (D73) brings Sky Rings, Lantern Trial and Fruit Catch with the valley. **Sky Rings is
ridden** (D74): you fly the course yourself, the dragon's Wing setting speed and turning;
the others stay unmounted.*

The player is always on the ground or on a tower, cueing the dragon — **riding is
disabled in all training and competitions.** Each event has four cups:
**Ember → Flame → Blaze → Starfire**. Winning earns Gleam, ribbons (den decorations)
and bond.

| Event | Type | Stage | How it plays |
|---|---|---|---|
| **Sky Rings** | Flying | Adolescent+ | Dragon flies a ring course. You tap the next ring on the bottom-screen map to cue it; Wing + responsiveness decide the line. |
| **Fruit Catch** | Flying (hop version for juveniles) | Juvenile+ | Flick fruit with the stylus; the dragon leaps or dives to catch it. Distance + style scoring. |
| **Command Trial** | Grounded | Juvenile+ | A judge calls a sequence of tricks; cue each by voice or button. Wit + mood. |
| **Shine Show** | Grounded | Adolescent+ | Groom, then present: a short pose routine judged on Shine, mood and flair. |
| **Lantern Trial** | Grounded | Adult | Crystal lanterns light up in a pattern; cue the dragon's breath to light them in order. Every element works (fire, mist, gust, spores, frost, light). |

## 7. Riding and Skyreach Valley

*Moved to Beta (D73, [plan](../plan/beta.md)): about 1 km across, 6–8 places standing in it,
arcade flying. **You're seen on foot** (D74): a third-person player walking with the dragon
at their side, riding it once it's grown; villagers and a first campaign join you.*

- **Adult dragons can be ridden anywhere in free roam** — on the ground (walk/run) or in
  the air (take off, glide, bank, dive, land).
- A **world map** fast-travels between places from Alpha 2; in 1.0 it becomes the live
  map of free flight ([map & travel](world-map-and-travel.md)).
- **Skyreach Valley** is a small open map: the Den (home), the Market, the Arena, the
  Nesting Grove, cliffs, a lake, and a few floating islands. Short draw distance with
  atmospheric fog to keep it fast on old 3DS.
- Riding finds **Gleam**, **trinkets** (hoard items) and occasionally a **wild egg**.
- Controls: Circle Pad steer, A flap/accelerate, B dive, R burst, L brake (D112; they banked before), touch screen
  shows the map. Optional gyro look.
- The player appears as a small **rider** on the dragon's back (one model, 3 outfit
  colors) — the only time the player is seen.

## 8. The Den, Sanctuary and Cold Vault

- **Den** — the home scene. Up to **3 active dragons** at a time (performance cap) plus
  **2 egg nests**. Dragons in the den live in real time and interact with each other.
- **Sanctuary** — off-map storage for any number of dragons (save cap 200). Keepers tend
  them: needs drain at 10% speed and never drop below 50, growth pauses, mood holds.
  Swap dragons in and out of the den at any time.
- **Cold Vault** — egg storage (cap 50). Incubation pauses while vaulted.
- Players can own any mix of breeds, stages and eggs at once.
- Den customization: rugs, perches, lanterns, the hoard pile (grows with trinkets).

## 9. Breeding (summary)

A bonded, happy adult **male and female** placed together at the **Nesting Stone** lay a
single egg the next day. Offspring inherit element alleles, body parts, colors and rare traits from both
parents. Six base breeds and fifteen hybrids give 21 named breeds, with thousands of
visual variations. Full rules: [Breeds & Genetics](breeds-and-genetics.md).

## 10. Time

- All simulation runs on the 3DS real-time clock. When the game starts, elapsed time is
  simulated in one-hour steps (capped at 14 days of catch-up).
- **Clock safety:** if the clock goes backwards, no time is counted and nothing is
  penalized. Large forward jumps are capped by the same 14-day catch-up limit.
- Day/night lighting follows local time. Dragons sleep at night.
- "Day" for care stars = local calendar day.

## 11. Wanderings (pedometer)

Take a juvenile-or-older dragon "along" and close the 3DS. The system pedometer counts
your steps; when you return, the dragon has found things proportional to distance:
Gleam, food, trinkets, and rarely a **wild egg** (the main way to get breeds you did not
start with). One dragon wanders at a time; its needs drain normally.

**Must-have for v1.** Wanderings ships together with breeding (Alpha 2). Wild eggs are one
of the two ways, with the Market, to find a partner of the opposite sex.

## 12. Economy

- **Gleam** — the single currency (dragons love shiny things). From competitions,
  Wanderings, riding finds and selling trinkets.
- **Market** — food, grooming kits, toys, den decor, nest upgrades, and one egg per
  day on rotation. **Market eggs are labeled with their sex** so a partner can be bought
  on purpose (D24); bred and wild eggs stay a surprise until they hatch.
- Full item list: [content & assets §7](../plan/content-and-assets.md).
- No real-money anything, ever.

## 13. Getting started

1. Title → name yourself → choose one of three starter eggs: **Ember, Tide, or Gale**.
2. Tutorial is diegetic: warm the egg, watch it hatch, feed it, say its name.
   At hatching you **name the dragon** with the 3DS keyboard (a random suggestion is
   pre-filled) and can rename it any time in the den (D27).
3. Grove, Frost and Lumen eggs come from the Market rotation and Wanderings, which
   unlock after the first dragon reaches Juvenile. These are also how you find your first
   dragon a partner of the opposite sex.

## 14. Social (later phase)

**Sky Visits** over 3DS local wireless: visit a friend's den, dragons play together,
exchange gifts, and compete in local events. Stretch goal: a *cross-den clutch* where
two players' dragons each give one egg to both players.

## 15. Screens

- **Top screen:** the 3D world (den, arena, valley). Optional stereoscopic 3D.
- **Bottom screen:** touch interaction and HUD — needs as four small ember-gauge icons,
  heartglow mood indicator, the pouch (items), cue buttons, map.
- In petting mode the camera frames the dragon so touches on the bottom screen map to
  its body (head / chin / back / belly zones).
- Every screen and the flow between them: [screens & flow](screens-and-flow.md).

## 16. Out of scope (for now)

- Death, illness, aging past adult.
- Online play, StreetPass, amiibo, face tracking.
- Real-money purchases.
- Languages other than English in v1 (strings live in one table so translation can come
  later, D30).

## 17. Review answers (2026-09-23)

1. **Pace:** ~2 weeks with perfect care is right. Keep the 4 / 8 / 14 day gates.
2. **Starters:** just the three (Ember, Tide, Gale).
3. **Sexes:** dragons are male or female; breeding needs one of each (§3.6).
4. **Upset dragons** retreat to the sulk nook (§3.3).
5. **Wanderings** is a v1 must-have and ships with breeding (§11).

Still open: see the "Open" rows in the [decision log](../plan/decisions.md).
