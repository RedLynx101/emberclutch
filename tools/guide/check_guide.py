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
]


def main() -> None:
    bad = 0
    for what, rel, pattern, phrases in CHECKS:
        v = read(rel, pattern)
        for phrase in phrases(v):
            if phrase not in GUIDE:
                print(f"[guide] {what}: the code says {v} ({rel}), the guide should say \"{phrase}\"")
                bad += 1
    print(f"[guide] {len(CHECKS)} numbers checked, {bad} to fix")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
