"""Checks the player's guide against the game (docs/plan/guide.md: "every number checked against the code").

    py -3.12 tools/guide/check_guide.py

Each row reads a number from the source and says what the guide must then say. A change to the balance
(src/core/dragon.cpp and the rest) that the guide doesn't follow fails here, naming the line to fix.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
GUIDE = (ROOT / "docs" / "guide" / "guide.md").read_text(encoding="utf-8")
WORDS = {1: "one", 2: "two", 3: "three", 4: "four", 5: "five", 14: "fourteen", 40: "forty", 50: "fifty"}
DAYS = {36: "1½ days", 78: "3¼ days", 132: "5½ days"}


def read(rel: str, pattern: str) -> float:
    m = re.search(pattern, (ROOT / rel).read_text(encoding="utf-8"), re.S)
    if not m:
        sys.exit(f"[guide] couldn't find {pattern!r} in {rel}: the check needs updating")
    return eval(m.group(1), {})  # (a number, or a constant expression such as 36 * 3600)


# (what, the source file, the pattern holding the number, the phrases the guide must hold for it)
CHECKS = [
    ("the egg's time", "src/core/dragon.hpp", r"kIncubationSeconds = ([0-9 *]+);",
     lambda v: ["for **a day and a half**"] if v == 36 * 3600 else [f"{v / 3600:g} hours"]),
    ("juvenile: days", "src/core/dragon.cpp", r"Stage::Juvenile: return (\d+);", lambda v: [f"| **Juvenile** | {DAYS.get(v, v)} |"]),
    ("adolescent: days", "src/core/dragon.cpp", r"Stage::Adolescent: return (\d+);",
     lambda v: [f"| **Adolescent** | {DAYS.get(v, v)} |"]),
    ("grown: days", "src/core/dragon.cpp", r"Stage::Adult: return (\d+);",
     lambda v: [f"| **Grown** | {DAYS.get(v, v)} |", "grown in five and a half days" if v == 132 else "?"]),
    ("juvenile: stars", "src/core/dragon.cpp", r"stageMinStars.*?Stage::Juvenile: return (\d+);",
     lambda v: [f"| **Juvenile** | 1½ days | {v} |"]),
    ("adolescent: stars", "src/core/dragon.cpp", r"stageMinStars.*?Stage::Adolescent: return (\d+);",
     lambda v: [f"| **Adolescent** | 3¼ days | {v} |"]),
    ("grown: stars", "src/core/dragon.cpp", r"stageMinStars.*?Stage::Adult: return (\d+);", lambda v: [f"| **Grown** | 5½ days | {v} |"]),
    ("stars a day", "src/core/dragon.cpp", r"avg >= 65 \? (\d) :", lambda v: [f"up to **{WORDS[v]} care stars**"]),
    ("bedtime", "src/core/clock.hpp", r"return h >= (\d+) \|\|", lambda v: [f"from **{v - 12} pm"]),
    ("waking", "src/core/clock.hpp", r"\|\| h < (\d+);", lambda v: [f"to {v} am**"]),
    ("sulky to upset", "src/core/dragon.cpp", r"sulkyHours >= (\d+)", lambda v: ["sulky for a whole day"] if v == 24 else ["?"]),
    ("days away to upset", "src/core/dragon.cpp", r"lastVisitAt >= (\d) \* kDay", lambda v: [f"if {WORDS[v]} days go by without a visit"]),
    ("the favourite food", "src/core/dragon.cpp", r"favorite \? (1\.5)f", lambda v: ["half again as much"] if v == 1.5 else ["?"]),
    ("the Sanctuary's floor", "src/core/dragon.cpp", r"if \(n.belly < (\d+)\) n.belly", lambda v: ["never drop below half"] if v == 50 else ["?"]),
    ("dragons in the den", "src/core/den_roster.hpp", r"kDenDragons = (\d+);", lambda v: [f"Up to **{WORDS[v]} dragons**"]),
    ("egg nests", "src/core/den_roster.hpp", r"kDenEggs = (\d+);", lambda v: [f"**{WORDS[v]} nests**"]),
    ("the Cold Vault", "src/core/den_roster.hpp", r"kVaultEggs = (\d+);", lambda v: [f"up to {WORDS[v]}"]),
    ("the level cap", "src/core/trainer.hpp", r"kLevelCap = (\d+);", lambda v: [f"**level {v}**"]),
    ("moves a battle", "src/core/dragon.hpp", r"kMoveSlots = (\d+);", lambda v: [f"**{WORDS[v]} moves**"]),
    ("stat steps", "src/core/battle.hpp", r"kMaxStage = (\d+);", lambda v: [f"up to {WORDS[v]} steps either way"]),
    ("heals a battle", "src/core/battle.hpp", r"kHealUses = (\d+);", lambda v: ["heal (once a battle)"] if v == 1 else ["?"]),
    ("the turn limit", "src/core/battle.hpp", r"kMaxTurns = (\d+);", lambda v: [f"after {WORDS[v]} turns"]),
    ("a Wanderings find", "src/core/wanderings.hpp", r"kStepsPerFind = (\d+);", lambda v: [f"every {v} steps"]),
    ("breeding's bond", "src/core/breeding.hpp", r"kBreedingBond = (\d+);", lambda v: ["happy and close to you"]),
    ("nesting again", "src/core/breeding.hpp", r"kBreedingRest = (\d) \* 24", lambda v: [f"rests {WORDS[v]} days"]),
    ("the catch-up", "src/core/clock.hpp", r"kMaxCatchUp = (\d+) \* kDay;", lambda v: ["up to two weeks at most"] if v == 14 else ["?"]),
    ("the ring's penalty", "docs/tech/challenges.md", r"each missed ring\s+adds (\d) s", lambda v: [f"every ring missed adds {WORDS[v]} seconds"]),
    # ---- The almanac (D150): the needs, moods, stars, walks, the Wanderings, breeding, levels, traits.
    ("Belly awake", "src/core/dragon.cpp", r"n\.belly -= \(asleep \? [\d.]+f : ([\d.]+)f\)", lambda v: [f"| **Belly** | {v:g} |"]),
    ("Belly asleep", "src/core/dragon.cpp", r"n\.belly -= \(asleep \? ([\d.]+)f", lambda v: [f"| **Belly** | 6 | {v:g} |"]),
    ("Clean", "src/core/dragon.cpp", r"n\.clean -= ([\d.]+)f \* tidy", lambda v: [f"| **Clean** | {v:g} | {v:g} |"]),
    ("Play", "src/core/dragon.cpp", r"playDrain = \(asleep \? ([\d.]+)f : [\d.]+f\)", lambda v: [f"| **Play** | 4 | {v:g} |"]),
    ("Love", "src/core/dragon.cpp", r"\(asleep \? ([\d.]+)f : 3\.0f\) \* \(d\.personality == Personality::Shy", lambda v: [f"| **Love** | 3 | {v:g} |"]),
    ("sleep's Energy", "src/core/dragon.cpp", r"n\.energy \+= ([\d.]+)f \* hours \* sleepy;\n    \} else if", lambda v: [f"+{v:g} at night"]),
    ("a nap's Energy", "src/core/dragon.cpp", r"\} else if \(d\.napping\) \{\n\s+n\.energy \+= ([\d.]+)f", lambda v: [f"+{v:g} napping"]),
    ("a battle's Energy", "src/core/trainer.hpp", r"kEnergyBattle = ([\d.]+)f", lambda v: [f"a battle {v:g}"]),
    ("a show's Energy", "src/core/trainer.hpp", r"kEnergyShow = ([\d.]+)f", lambda v: [f"a beauty show {v:g}"]),
    ("a challenge's Energy", "src/core/trainer.hpp", r"kEnergyChallenge = ([\d.]+)f", lambda v: [f"a challenge {v:g}"]),
    ("joyful", "src/core/dragon.cpp", r"if \(score >= (\d+)\) return Mood::Joyful", lambda v: [f"**Joyful** at {v} and up"]),
    ("content", "src/core/dragon.cpp", r"if \(score >= (\d+)\) return Mood::Content", lambda v: [f"**Content** {v} and up"]),
    ("restless", "src/core/dragon.cpp", r"if \(score >= (\d+)\) return Mood::Restless", lambda v: [f"**Restless** {v} and up"]),
    ("three stars", "src/core/dragon.cpp", r"avg >= (\d+) \? 3", lambda v: [f"{v} and up earns\n  three stars"]),
    ("a walk's bond", "src/core/trainer.cpp", r"\? 75\.0f : ([\d.]+)f;", lambda v: [f"a bond point every {v:g} m"]),
    ("trained points", "src/core/trainer.hpp", r"kMaxTrained = (\d+);", lambda v: [f"plus up to {v} trained points"]),
    ("the rare colouring", "src/core/kinds.cpp", r"\(rareParent \? 10 : (\d+)\)", lambda v: [f"**1 time in {v}**"]),
    ("a wild egg", "src/core/wanderings.cpp", r"kTraitLucky\) \? (\d) : (?:\d), 100", lambda v: [f"(3%, or {v}% for a Lucky dragon)"]),
    ("grown wanderers", "src/core/wanderings.cpp", r"Stage::Adult\) chances \*= ([\d.]+)f", lambda v: ["30% more for grown dragons"] if v == 1.3 else ["?"]),
    ("level 42's experience", "src/core/trainer.cpp", r"return (\d+) \* n \* n \+ 38 \* n;", lambda v: [f"{v} × n² + 38 × n", "| 21,730 |" if v == 12 else "?"]),
    ("Swift", "src/core/flight.cpp", r"t\.glideSpeed \*= ([\d.]+)f", lambda v: [f"Flies {round((v - 1) * 100)}% faster"]),
    ("Tidy", "src/core/dragon.cpp", r"kTraitTidy\) \? ([\d.]+)f", lambda v: [f"dust settles, {round((1 - v) * 100)}% slower"]),
    ("Deep Sleeper", "src/core/dragon.cpp", r"kTraitDeepSleeper\) \? ([\d.]+)f", lambda v: [f"Energy back {round((v - 1) * 100)}% faster"]),
    ("Ironhide", "src/core/battle.cpp", r"kBtIronhide\) traits \*= ([\d.]+)f", lambda v: ["Takes a tenth less damage"] if v == 0.9 else ["?"]),
    ("Elemental", "src/core/battle.cpp", r"traits \*= ([\d.]+)f;\n\s+if \(foe\.traits & kBtIronhide", lambda v: [f"hit {round((v - 1) * 100)}% harder"]),
    ("Showoff", "src/core/pageant.cpp", r"kTraitShowoff\) \? ([\d.]+)f", lambda v: [f"+{v:g} Look in shows"]),
    ("Gentle Giant", "src/core/pageant.cpp", r"kTraitGentleGiant\) \? ([\d.]+)f", lambda v: [f"+{v:g} Poise in shows"]),
    ("Sure-Footed", "src/app/challenge_lanterns.cpp", r"kTraitSureFooted\) \? (\d) : 0", lambda v: [f"{WORDS[v].capitalize()} extra heart"]),
]


def array(rel: str, pattern: str) -> list:
    m = re.search(pattern, (ROOT / rel).read_text(encoding="utf-8"))
    if not m:
        sys.exit(f"[guide] couldn't find {pattern!r} in {rel}: the check needs updating")
    return [int(x) for x in re.findall(r"\d+", m.group(1))]


def cups(values) -> str:
    return " | ".join(f"{v:,}" for v in values) + " |"


# (what, the source, the pattern holding an array of four, the row's label in the guide)
ROWS = [
    ("a cup's first win", "src/core/challenges.cpp", r"k\[kCups\] = \{(80[\d, ]+)\}", "| **A cup's first win** | "),
    ("a cup won again", "src/core/challenges.cpp", r"k\[kCups\] = \{(30[\d, ]+)\}", "| **A win again (once a day)** | "),
    ("placing", "src/core/challenges.cpp", r"k\[kCups\] = \{(10[\d, ]+)\}", "| **Placing (while the day's prize is still to win)** | "),
    ("a show's first win", "src/core/pageant.cpp", r"kFirstWin\[kLeagues\] = \{([\d, ]+)\}", "| **A show's first win** | "),
    ("a league's bonus", "src/core/pageant.cpp", r"kLeagueBonus\[kLeagues\] = \{([\d, ]+)\}", "| **Winning the league** | "),
]


def main() -> None:
    bad = 0
    for what, rel, pattern, phrases in CHECKS:
        v = read(rel, pattern)
        for phrase in phrases(v):
            if phrase not in GUIDE:
                print(f"[guide] {what}: the code says {v} ({rel}), the guide should say \"{phrase}\"")
                bad += 1
    for what, rel, pattern, label in ROWS:
        row = label + cups(array(rel, pattern))
        if row not in GUIDE:
            print(f"[guide] {what}: the guide's row should read \"{row}\"")
            bad += 1
    print(f"[guide] {len(CHECKS) + len(ROWS)} numbers and rows checked, {bad} to fix")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
