"""Compiles the story scripts (story/*.story) into the game's tables (D137: the story engine).

  python tools/story/build_story.py            writes src/core/story_data.inc and src/core/story_ids.hpp
  python tools/story/build_story.py --check    only checks (exit 1 on a problem)
  python tools/story/build_story.py --summary  also writes docs/design/story-index.md (every quest, letter, person)

The scripts are plain text, one declaration per line, blocks by indentation (see story/README.md):

  person <id> "<Name>" "<title>" voice <0|1> pitch <f> [villager <v>] [body <form>] [portrait <sheet>]
  quest <id> "<Title>"            (then: line, giver, gleam, after, when, rumour, start auto, step, reward)
  talk <person>                   (then rules: rule [once] [important] [if <cond>], with do / lines / vary)
  letter <id> from <person> "<subject>"   (then: when, do, paragraphs; "--" breaks a page)
  spot <person> <place> <x> <y> [facing <rad>] [clip <name>] [when <cond>]
  spots <person> daily <place> <x> <y> [facing f] [clip c] | <place> <x> <y> ... [when <cond>]
  pickup <id> <place> <x> <y> "<prompt>" [group <g>] [glint] [when <cond>]   (then: do, lines)
  sign <id> <place> <x> <y> "<prompt>" [when <cond>]                            (then: lines)

A line of dialogue: `[feel] text`; `@person [feel] text` for someone else in the same talk; `* text` for
narration; `? <cond> | [feel] text` for a line said only when the condition holds.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
STORY = os.path.join(ROOT, "story")
OUT_INC = os.path.join(ROOT, "src", "core", "story_data.inc")
OUT_IDS = os.path.join(ROOT, "src", "core", "story_ids.hpp")
OUT_INDEX = os.path.join(ROOT, "docs", "design", "story-index.md")

FEELS = ["calm", "happy", "laugh", "excited", "surprised", "shock", "sad", "crying", "angry", "huff", "worried",
         "scared", "sleepy", "love", "proud", "cool", "shy", "thinking", "wistful", "dizzy"]
LINES = ["main", "pageant", "league", "hollow", "cove", "errand"]
PLACES = {"den": "kPlaceDen", "market": "kPlaceMarket", "stone": "kPlaceStone", "sanctuary": "kPlaceSanctuary",
          "vault": "kPlaceVault", "trailhead": "kPlaceTrailhead", "arena": "kPlaceArena", "lake": "kPlaceLake",
          "lodge": "kPlaceKeeper", "keeper": "kPlaceKeeper", "isles": "kPlaceIsles", "orchard": "kPlaceOrchard",
          "mill": "kPlaceMill", "bridge": "kPlaceMill", "grotto": "kPlaceGrotto", "ruins": "kPlaceRuins",
          "caldera": "kPlaceCaldera", "glade": "kPlaceGlade", "cove": "kPlaceCove", "hollow": "kPlaceHollow"}
CHALLENGES = {"fruit": 0, "rings": 1, "trial": 2}
WORLD_FLAGS = {"entered_valley": "kFlagEnteredValley", "met_keeper": "kFlagMetKeeper", "heard_story": "kFlagHeardStory",
               "found_stray": "kFlagFoundStray", "glided": "kFlagGlided", "rode": "kFlagRode",
               "met_traveller": "kFlagMetTraveller", "wandered": "kFlagWandered", "festival": "kFlagFestival",
               "met_market": "kFlagMetMarket", "met_sanctuary": "kFlagMetSanctuary", "met_steward": "kFlagMetSteward",
               "star_egg": "kFlagStarEgg", "love_letter": "kFlagLoveLetter"}
FOODS = ["firepepper", "riverfish", "skyberry", "honeyroot", "frostmelon", "starfruit", "bread", "drumstick", "candy",
         "cookie"]
RECORDS = ["fish", "shells", "battles", "shows", "wild", "photos", "walks", "cups"]
VILLAGERS = ["keeper", "market", "sanctuary", "steward", "child", "traveller"]

# Condition and effect opcodes (keep in step with core/story.cpp's).
COND_OPS = ["done", "begun", "active", "new", "step", "past", "flag", "world", "lantern", "place", "cup", "lanterns",
            "grown", "adults", "hatched", "juvenile", "days", "event", "eventdays", "hour", "daymod", "var", "vareq",
            "bit", "league", "beaten", "shows", "hollow", "count", "mail", "read", "gleam", "pouch", "partner",
            "wearing", "true", "showwon"]
EFFECT_OPS = ["start", "advance", "finish", "flag", "unflag", "world", "var", "add", "bit", "gleam", "food", "wear",
              "letter", "event", "staregg", "take"]
WHERE_KINDS = ["none", "place", "lantern", "cup", "person", "area", "spot", "unlit", "group"]
MAX_STEPS = 6
MAX_LINE_CHARS = 140   # a dialogue line (the box holds about 150 with names filled in)
MAX_TALK_LINES = 12


class Problem(Exception):
    pass


def camel(s):
    return "".join(p[:1].upper() + p[1:] for p in re.split(r"[_\-]", s) if p)


def accessories():
    """The accessory table's names as script slugs, in order (core/accessories.cpp)."""
    src = open(os.path.join(ROOT, "src", "core", "accessories.cpp"), encoding="utf-8").read()
    block = src[src.index("kAccessories[kAccessoryCount] = {"):]
    block = block[:block.index("};")]
    names = re.findall(r'^\s*\{"([^"]+)", S::', block, re.M)
    return [re.sub(r"[^a-z0-9]+", "_", n.lower()).strip("_") for n in names]


def tokens(line):
    out, i, n = [], 0, len(line)
    while i < n:
        c = line[i]
        if c.isspace():
            i += 1
        elif c == '"':
            j = line.index('"', i + 1)
            out.append(("str", line[i + 1:j]))
            i = j + 1
        else:
            j = i
            while j < n and not line[j].isspace():
                j += 1
            out.append(("word", line[i:j]))
            i = j
    return out


class Story:
    def __init__(self):
        self.persons, self.quests, self.letters, self.flags, self.vars, self.events = [], [], [], [], [], []
        self.groups = []
        self.rules, self.spots, self.pickups = [], [], []
        self.conds, self.effects, self.lines, self.steps = [], [], [], []
        self.acc = accessories()
        self.where = ""
        self.pending_refs = []  # (kind, name, where) checked once everything is read
        self.hidden = 0

    # ---------------------------------------------------------------- names
    def fail(self, msg):
        raise Problem(f"{self.where}: {msg}")

    def ref(self, kind, name):
        self.pending_refs.append((kind, name, self.where))
        return (kind, name)

    def flag(self, name):
        if name not in self.flags:
            self.flags.append(name)
        return ("flag", name)

    def var(self, name):
        if name not in self.vars:
            self.vars.append(name)
        return ("var", name)

    def event(self, name):
        if name not in self.events:
            self.events.append(name)
        return ("event", name)

    def group(self, name):
        if name not in self.groups:
            self.groups.append(name)
        return self.groups.index(name)

    def num(self, word, lo=None, hi=None):
        try:
            v = float(word) if "." in word else int(word)
        except ValueError:
            self.fail(f"expected a number, got '{word}'")
        if lo is not None and v < lo or hi is not None and v > hi:
            self.fail(f"{word} is out of range ({lo}..{hi})")
        return v

    def place(self, word):
        if word not in PLACES:
            self.fail(f"unknown place '{word}' (one of {', '.join(sorted(PLACES))})")
        return PLACES[word]

    # ---------------------------------------------------------------- conditions and effects
    def cond(self, words):
        """A condition: terms joined by `and`, each maybe `not ...`. Returns (start, count) in self.conds."""
        start = len(self.conds)
        terms, cur = [], []
        for w in words:
            if w == "and":
                terms.append(cur)
                cur = []
            else:
                cur.append(w)
        terms.append(cur)
        for t in terms:
            if not t:
                self.fail("an empty condition term")
            neg = 0
            if t[0] == "not":
                neg, t = 1, t[1:]
            op, args = t[0], t[1:]
            if op == "met":  # (a person met: its flag, set by any talk with them read to its end)
                self.need(args, 1, op)
                self.ref("person", args[0])
                op, args = "flag", ["met_" + args[0]]
            if op not in COND_OPS:
                self.fail(f"unknown condition '{op}'")
            a, b = self.cond_args(op, args)
            self.conds.append((COND_OPS.index(op), neg, a, b))
        return start, len(self.conds) - start

    def need(self, args, n, what):
        if len(args) != n:
            self.fail(f"'{what}' takes {n} argument(s), got {len(args)}: {' '.join(args)}")

    def cond_args(self, op, args):
        if op in ("done", "begun", "active", "new"):
            self.need(args, 1, op)
            return self.ref("quest", args[0]), 0
        if op in ("step", "past"):
            self.need(args, 2, op)
            return self.ref("quest", args[0]), self.num(args[1], 1, MAX_STEPS + 1)
        if op == "flag":
            self.need(args, 1, op)
            return self.flag(args[0]), 0
        if op == "world":
            self.need(args, 1, op)
            if args[0] not in WORLD_FLAGS:
                self.fail(f"unknown world flag '{args[0]}'")
            return ("raw", WORLD_FLAGS[args[0]]), 0
        if op in ("lantern", "place"):
            self.need(args, 1, op)
            return ("raw", self.place(args[0])), 0
        if op == "cup":
            if len(args) not in (1, 2) or args[0] not in CHALLENGES:
                self.fail("'cup' takes fruit|rings|trial [cups]")
            return ("int", CHALLENGES[args[0]]), self.num(args[1], 1, 4) if len(args) > 1 else 1
        if op in ("lanterns", "grown", "hatched", "juvenile", "partner", "true"):
            self.need(args, 0, op)
            return ("int", 0), 0
        if op in ("adults", "league", "shows", "hollow", "gleam"):
            self.need(args, 1, op)
            return ("int", self.num(args[0], 0, 65535)), 0
        if op == "days":
            self.need(args, 2, op)
            return self.ref("quest", args[0]), self.num(args[1], 0, 3650)
        if op == "event":
            self.need(args, 1, op)
            return self.event(args[0]), 0
        if op == "eventdays":
            self.need(args, 2, op)
            return self.event(args[0]), self.num(args[1], 0, 3650)
        if op == "hour":
            self.need(args, 2, op)
            return ("int", self.num(args[0], 0, 24)), self.num(args[1], 0, 24)
        if op == "daymod":
            self.need(args, 2, op)
            return ("int", self.num(args[0], 1, 30)), self.num(args[1], 0, 29)
        if op in ("var", "vareq"):
            self.need(args, 2, op)
            return self.var(args[0]), self.num(args[1], 0, 255)
        if op == "bit":
            self.need(args, 2, op)
            return self.var(args[0]), self.num(args[1], 0, 7)
        if op == "beaten":
            self.need(args, 2, op)
            return ("int", self.num(args[0], 0, 3)), self.num(args[1], 0, 4)
        if op == "count":
            self.need(args, 2, op)
            if args[0] not in RECORDS:
                self.fail(f"unknown record '{args[0]}'")
            return ("int", RECORDS.index(args[0])), self.num(args[1], 0, 65535)
        if op in ("mail", "read"):
            self.need(args, 1, op)
            return self.ref("letter", args[0]), 0
        if op == "pouch":
            self.need(args, 2, op)
            if args[0] not in FOODS:
                self.fail(f"unknown food '{args[0]}'")
            return ("int", FOODS.index(args[0])), self.num(args[1], 0, 999)
        if op == "showwon":
            self.need(args, 1, op)
            return ("int", self.num(args[0], 0, 3)), 0
        if op == "wearing":
            self.need(args, 1, op)
            if args[0] not in self.acc:
                self.fail(f"unknown accessory '{args[0]}'")
            return ("int", self.acc.index(args[0])), 0
        self.fail(f"condition '{op}' has no argument rule")

    def effect_list(self, words):
        """Effects, comma separated. Returns (start, count) in self.effects."""
        start = len(self.effects)
        parts, cur = [], []
        for w in words:
            if w.endswith(","):
                cur.append(w[:-1])
                parts.append(cur)
                cur = []
            elif w == ",":
                parts.append(cur)
                cur = []
            else:
                cur.append(w)
        if cur:
            parts.append(cur)
        for p in parts:
            if not p:
                continue
            op, args = p[0], p[1:]
            if op == "met":  # (a person met: its flag)
                self.need(args, 1, op)
                self.ref("person", args[0])
                op, args = "flag", ["met_" + args[0]]
            if op not in EFFECT_OPS:
                self.fail(f"unknown effect '{op}'")
            self.effects.append((EFFECT_OPS.index(op),) + self.effect_args(op, args))
        return start, len(self.effects) - start

    def effect_args(self, op, args):
        if op in ("start", "advance", "finish"):
            self.need(args, 1, op)
            return self.ref("quest", args[0]), 0
        if op in ("flag", "unflag"):
            self.need(args, 1, op)
            return self.flag(args[0]), 0
        if op == "world":
            self.need(args, 1, op)
            if args[0] not in WORLD_FLAGS:
                self.fail(f"unknown world flag '{args[0]}'")
            return ("raw", WORLD_FLAGS[args[0]]), 0
        if op in ("var", "add"):
            self.need(args, 2, op)
            return self.var(args[0]), self.num(args[1], 0, 255)
        if op == "bit":
            self.need(args, 2, op)
            return self.var(args[0]), self.num(args[1], 0, 7)
        if op == "gleam":
            self.need(args, 1, op)
            return ("int", self.num(args[0], 0, 65535)), 0
        if op == "food":
            self.need(args, 2, op)
            if args[0] not in FOODS:
                self.fail(f"unknown food '{args[0]}'")
            return ("int", FOODS.index(args[0])), self.num(args[1], 1, 99)
        if op == "take":
            self.need(args, 2, op)
            if args[0] not in FOODS:
                self.fail(f"unknown food '{args[0]}'")
            return ("int", FOODS.index(args[0])), self.num(args[1], 1, 99)
        if op == "wear":
            self.need(args, 1, op)
            if args[0] not in self.acc:
                self.fail(f"unknown accessory '{args[0]}' (one of {', '.join(self.acc)})")
            return ("int", self.acc.index(args[0])), 0
        if op == "letter":
            self.need(args, 1, op)
            return self.ref("letter", args[0]), 0
        if op == "event":
            self.need(args, 1, op)
            return self.event(args[0]), 0
        if op == "staregg":
            self.need(args, 0, op)
            return ("int", 0), 0
        self.fail(f"effect '{op}' has no argument rule")

    # ---------------------------------------------------------------- lines
    def line(self, text, group=0):
        """A dialogue line: `[feel] text`, `@who [feel] text`, `* text`, `? cond | line`."""
        cond = (0, 0)
        if text.startswith("?"):
            if "|" not in text:
                self.fail("a conditional line needs '? <condition> | <line>'")
            c, text = text[1:].split("|", 1)
            cond = self.cond(c.split())
            text = text.strip()
        speaker = None
        feel = "calm"
        if text.startswith("*"):
            speaker, text = "narrator", text[1:].strip()
            self.ref("person", "narrator")
        elif text.startswith("@"):
            who, text = text[1:].split(None, 1)
            speaker = who
            self.ref("person", who)
        m = re.match(r"\[([a-z]+)\]\s*(.*)$", text)
        if m:
            feel, text = m.group(1), m.group(2)
            if feel not in FEELS:
                self.fail(f"unknown feeling [{feel}] (one of {', '.join(FEELS)})")
        if not text:
            self.fail("an empty line")
        size = len(text.replace("{D}", "X" * 15).replace("{P}", "X" * 15))
        if size > MAX_LINE_CHARS + 30 or len(text) > MAX_LINE_CHARS:
            self.fail(f"a line is too long for the box ({len(text)} chars, at most {MAX_LINE_CHARS}): {text[:50]}...")
        self.lines.append((text, FEELS.index(feel), speaker, group, cond))
        return len(self.lines) - 1

    def paragraph(self, text):
        self.lines.append((text, 0, None, 0, (0, 0)))
        return len(self.lines) - 1

    # ---------------------------------------------------------------- parsing
    def parse_file(self, path):
        rows = []
        for i, raw in enumerate(open(path, encoding="utf-8").read().split("\n"), 1):
            if not raw.strip() or raw.lstrip().startswith("#"):
                continue
            indent = len(raw) - len(raw.lstrip(" "))
            if "\t" in raw[:indent + 1]:
                raise Problem(f"{os.path.basename(path)}:{i}: use spaces, not tabs")
            rows.append((i, indent, raw.strip()))
        k = 0
        name = os.path.basename(path)
        while k < len(rows):
            i, indent, text = rows[k]
            self.where = f"{name}:{i}"
            if indent != 0:
                self.fail("a declaration must start at the line's beginning")
            body = []
            k += 1
            while k < len(rows) and rows[k][1] > 0:
                body.append(rows[k])
                k += 1
            self.declaration(name, text, body)

    def declaration(self, name, text, body):
        toks = tokens(text)
        kw = toks[0][1]
        if kw == "person":
            self.person(toks, body)
        elif kw == "quest":
            self.quest(name, toks, body)
        elif kw == "talk":
            self.talk(name, toks, body)
        elif kw == "letter":
            self.letter(name, toks, body)
        elif kw == "spot":
            self.spot(toks, body)
        elif kw == "spots":
            self.spots_daily(toks, body)
        elif kw in ("pickup", "sign"):
            self.pickup(name, kw, toks, body)
        elif kw in ("flag", "var", "event"):
            for t in toks[1:]:
                getattr(self, kw)(t[1])
        else:
            self.fail(f"unknown declaration '{kw}'")

    def words(self, toks):
        return [t[1] for t in toks]

    def person(self, toks, body):
        if len(toks) < 3 or toks[2][0] != "str" or toks[3][0] != "str":
            self.fail('person <id> "<Name>" "<title>" ...')
        pid, pname, title = toks[1][1], toks[2][1], toks[3][1]
        if any(p["id"] == pid for p in self.persons):
            self.fail(f"person '{pid}' twice")
        p = {"id": pid, "name": pname, "title": title, "voice": 0, "pitch": 1.0, "villager": -1, "body": "", "portrait": "",
             "tint": (250, 226, 196)}
        w = self.words(toks[4:])
        j = 0
        while j < len(w):
            key = w[j]
            if key == "voice":
                p["voice"] = self.num(w[j + 1], 0, 1)
            elif key == "pitch":
                p["pitch"] = float(w[j + 1])
            elif key == "villager":
                if w[j + 1] not in VILLAGERS:
                    self.fail(f"unknown villager '{w[j + 1]}'")
                p["villager"] = VILLAGERS.index(w[j + 1])
            elif key == "body":
                p["body"] = w[j + 1]
            elif key == "portrait":
                p["portrait"] = w[j + 1]
            elif key == "tint":
                p["tint"] = tuple(int(x) for x in w[j + 1].split(","))
            else:
                self.fail(f"unknown person field '{key}'")
            j += 2
        self.flag("met_" + pid)
        self.persons.append(p)

    def quest(self, fname, toks, body):
        if len(toks) < 3 or toks[2][0] != "str":
            self.fail('quest <id> "<Title>"')
        qid, title = toks[1][1], toks[2][1]
        if any(q["id"] == qid for q in self.quests):
            self.fail(f"quest '{qid}' twice")
        q = {"id": qid, "title": title, "line": "main", "giver": None, "gleam": 0, "after": [], "cond": (0, 0),
             "rumour": "", "auto": 0, "steps": [], "reward": (0, 0), "file": fname}
        conds = []
        for i, indent, text in body:
            self.where = f"{fname}:{i}"
            t = tokens(text)
            w = self.words(t)
            kw = w[0]
            if kw == "line":
                if w[1] not in LINES:
                    self.fail(f"unknown line '{w[1]}' (one of {', '.join(LINES)})")
                q["line"] = w[1]
            elif kw == "giver":
                q["giver"] = w[1]
                self.ref("person", w[1])
            elif kw == "gleam":
                q["gleam"] = self.num(w[1], 0, 65535)
            elif kw == "after":
                for a in w[1:]:
                    q["after"].append(a)
                    self.ref("quest", a)
            elif kw == "when":
                conds.append(w[1:])
            elif kw == "rumour":
                q["rumour"] = t[1][1]
            elif kw == "start":
                if w[1:] != ["auto"]:
                    self.fail("'start auto' is the only start")
                q["auto"] = 1
            elif kw == "step":
                if t[1][0] != "str":
                    self.fail('step "<text>" [until <cond>] [talk <person>] [where ...]')
                q["steps"].append(self.step(t[1][1], w[2:]))
            elif kw == "reward":
                q["reward"] = self.effect_list(w[1:])
            else:
                self.fail(f"unknown quest field '{kw}'")
        # availability: every `after` done, and each `when`
        words = []
        for a in q["after"]:
            words += (["and"] if words else []) + ["done", a]
        for c in conds:
            words += (["and"] if words else []) + c
        if words:
            q["cond"] = self.cond(words)
        if not q["steps"]:
            self.fail(f"quest '{qid}' has no steps")
        if len(q["steps"]) > MAX_STEPS:
            self.fail(f"quest '{qid}' has {len(q['steps'])} steps (at most {MAX_STEPS})")
        self.quests.append(q)

    def step(self, text, w):
        s = {"text": text, "cond": (0, 0), "talk": None, "where": ("none", [])}
        if len(text) > 70:
            self.fail(f"a step's text is too long for the Journal ({len(text)} chars, at most 70)")
        # split into clauses: until ..., talk X, where ...
        clauses, cur = [], []
        for x in w:
            if x in ("until", "talk", "where") and cur:
                clauses.append(cur)
                cur = []
            cur.append(x)
        if cur:
            clauses.append(cur)
        for c in clauses:
            if c[0] == "until":
                s["cond"] = self.cond(c[1:])
            elif c[0] == "talk":
                s["talk"] = c[1]
                self.ref("person", c[1])
            elif c[0] == "where":
                kind, args = c[1], c[2:]
                if kind not in WHERE_KINDS:
                    self.fail(f"unknown where '{kind}' (one of {', '.join(WHERE_KINDS)})")
                if kind in ("place", "lantern"):
                    self.need(args, 1, "where " + kind)
                    self.place(args[0])
                elif kind == "cup":
                    if args[0] not in CHALLENGES:
                        self.fail("where cup fruit|rings|trial")
                elif kind == "person":
                    self.ref("person", args[0])
                elif kind == "area":
                    self.need(args, 4, "where area <place> <x> <y> <radius>")
                    self.place(args[0])
                elif kind == "spot":
                    self.need(args, 3, "where spot <place> <x> <y>")
                    self.place(args[0])
                elif kind == "group":
                    self.need(args, 1, "where group <group>")
                    self.group(args[0])
                s["where"] = (kind, args)
            else:
                self.fail(f"unknown step clause '{c[0]}'")
        if s["cond"] == (0, 0) and s["talk"] is None:
            self.fail(f"step '{text}' needs 'until <condition>' or 'talk <person>'")
        return s

    def rule_body(self, body, start_indent):
        """do / lines / vary / -- in a rule's (or letter's, pickup's) body."""
        eff = (0, 0)
        first = len(self.lines)
        group, groups, varying = 0, 1, False
        for i, indent, text in body:
            self.where = f"{self.fname}:{i}"
            if text.startswith("do ") or text == "do":
                if eff != (0, 0):
                    self.fail("one 'do' a rule (list the effects with commas)")
                eff = self.effect_list(text.split()[1:])
            elif text == "vary":
                varying = True
            elif text == "--":
                if not varying:
                    self.fail("'--' only inside 'vary'")
                group += 1
                groups = group + 1
            else:
                self.line(text, group)
        return eff, first, len(self.lines) - first, groups

    def talk(self, fname, toks, body):
        who = toks[1][1]
        self.ref("person", who)
        self.fname = fname
        k = 0
        while k < len(body):
            i, indent, text = body[k]
            self.where = f"{fname}:{i}"
            w = text.split()
            if w[0] != "rule":
                self.fail("a talk holds rules: 'rule [once] [important] [if <condition>]'")
            rbody = []
            k += 1
            while k < len(body) and body[k][1] > indent:
                rbody.append(body[k])
                k += 1
            once = "once" in w[1:4]
            chat = "chat" in w[1:4]
            important = not chat and ("important" in w[1:4] or "if" in w or once)
            # the order a person's rules are tried in, across every file: `first` rules (the festival
            # night), then the rest in the files' order, then `chat` rules (hints and idle talk that
            # hold for a while), then plain chatter (no condition) last
            tier = 1 if "first" in w[1:4] else (3 if chat else (2 if ("if" in w or once) else 4))
            cw = w[w.index("if") + 1:] if "if" in w else []
            eff, lf, ln, groups = self.rule_body(rbody, indent)
            if ln == 0:
                self.fail("a rule with no lines")
            if once:
                # its flag named from what it says (stable when other rules come and go; the save keeps it)
                import hashlib
                key = who + "|" + " ".join(cw) + "|" + self.lines[lf][0]
                hidden = f"once_{who}_{hashlib.sha1(key.encode('utf-8')).hexdigest()[:8]}"
                self.flag(hidden)
                cw = (cw + ["and"] if cw else []) + ["not", "flag", hidden]
                extra = self.effect_list(["flag", hidden])
                if eff == (0, 0):
                    eff = extra
                elif extra[0] == eff[0] + eff[1]:
                    eff = (eff[0], eff[1] + 1)
                else:
                    self.fail("internal: once-effects not adjacent")
            cond = self.cond(cw) if cw else (0, 0)
            per_group = [sum(1 for x in range(lf, lf + ln) if self.lines[x][3] == g) for g in range(groups)]
            if max(per_group) > MAX_TALK_LINES:
                self.fail(f"a rule says {max(per_group)} lines (at most {MAX_TALK_LINES})")
            self.rules.append({"person": who, "cond": cond, "eff": eff, "line": (lf, ln), "groups": groups,
                               "important": 1 if important else 0, "where": self.where, "tier": tier})

    def letter(self, fname, toks, body):
        if len(toks) < 5 or toks[2][1] != "from" or toks[4][0] != "str":
            self.fail('letter <id> from <person> "<subject>"')
        lid, who, subject = toks[1][1], toks[3][1], toks[4][1]
        self.ref("person", who)
        self.fname = fname
        cond, eff = (0, 0), (0, 0)
        first = len(self.lines)
        for i, indent, text in body:
            self.where = f"{fname}:{i}"
            if text.startswith("when "):
                cond = self.cond(text.split()[1:])
            elif text.startswith("do "):
                eff = self.effect_list(text.split()[1:])
            else:
                if len(text) > 400:
                    self.fail("a letter's paragraph is too long (at most 400 chars)")
                self.paragraph(text)
        if len(self.lines) == first:
            self.fail(f"letter '{lid}' has no words")
        if any(l["id"] == lid for l in self.letters):
            self.fail(f"letter '{lid}' twice")
        self.letters.append({"id": lid, "from": who, "subject": subject, "cond": cond, "eff": eff,
                             "line": (first, len(self.lines) - first)})

    def spot_args(self, w):
        """<place> <x> <y> [facing f] [clip c] -> (place, x, y, facing, clip)."""
        place = self.place(w[0])
        x, y = self.num(w[1]), self.num(w[2])
        facing, clip, j = 0.0, "", 3
        while j < len(w):
            if w[j] == "facing":
                facing = float(w[j + 1])
            elif w[j] == "clip":
                clip = w[j + 1]
            else:
                self.fail(f"unknown spot field '{w[j]}'")
            j += 2
        return place, x, y, facing, clip

    def spot(self, toks, body):
        w = self.words(toks)
        who = w[1]
        self.ref("person", who)
        cond = (0, 0)
        if "when" in w:
            cond = self.cond(w[w.index("when") + 1:])
            w = w[:w.index("when")]
        self.spots.append({"person": who, "at": self.spot_args(w[2:]), "cond": cond})

    def spots_daily(self, toks, body):
        w = self.words(toks)
        who = w[1]
        self.ref("person", who)
        if w[2] != "daily":
            self.fail("spots <person> daily <spot> | <spot> ... [when <cond>]")
        extra = []
        if "when" in w:
            extra = w[w.index("when") + 1:]
            w = w[:w.index("when")]
        options = " ".join(w[3:]).split("|")
        for k, o in enumerate(options):
            c = ["daymod", str(len(options)), str(k)] + (["and"] + extra if extra else [])
            self.spots.append({"person": who, "at": self.spot_args(o.split()), "cond": self.cond(c)})

    def pickup(self, fname, kw, toks, body):
        w = self.words(toks)
        if len(toks) < 6 or toks[5][0] != "str":
            self.fail(f'{kw} <id> <place> <x> <y> "<prompt>" ...')
        pid = w[1]
        place, x, y = self.place(w[2]), self.num(w[3]), self.num(w[4])
        prompt = toks[5][1]
        rest = w[6:]
        cond, group, glint = (0, 0), -1, 0
        if "when" in rest:
            cond = self.cond(rest[rest.index("when") + 1:])
            rest = rest[:rest.index("when")]
        j = 0
        while j < len(rest):
            if rest[j] == "group":
                group = self.group(rest[j + 1])
                j += 2
            elif rest[j] == "glint":
                glint = 1
                j += 1
            else:
                self.fail(f"unknown {kw} field '{rest[j]}'")
        self.fname = fname
        eff, lf, ln, groups = self.rule_body(body, 0)
        if ln == 0:
            self.fail(f"{kw} '{pid}' says nothing")
        if any(p["id"] == pid for p in self.pickups):
            self.fail(f"pickup '{pid}' twice")
        self.pickups.append({"id": pid, "place": place, "x": x, "y": y, "prompt": prompt, "sign": 1 if kw == "sign" else 0,
                             "group": group, "glint": glint, "cond": cond, "eff": eff, "line": (lf, ln)})

    # ---------------------------------------------------------------- checks
    def check(self):
        names = {"quest": [q["id"] for q in self.quests], "person": [p["id"] for p in self.persons],
                 "letter": [l["id"] for l in self.letters]}
        for kind, name, where in self.pending_refs:
            if name not in names[kind]:
                raise Problem(f"{where}: unknown {kind} '{name}'")
        for q in self.quests:
            for k, s in enumerate(q["steps"]):
                if s["talk"] is None:
                    continue
                # a talk step needs a rule that moves it on: 'advance' or 'finish' of this quest
                moves = False
                for r in self.rules:
                    for e in self.effects[r["eff"][0]:r["eff"][0] + r["eff"][1]]:
                        if EFFECT_OPS[e[0]] in ("advance", "finish") and e[1] == ("quest", q["id"]):
                            moves = True
                for l in self.letters + self.pickups:
                    for e in self.effects[l["eff"][0]:l["eff"][0] + l["eff"][1]]:
                        if EFFECT_OPS[e[0]] in ("advance", "finish") and e[1] == ("quest", q["id"]):
                            moves = True
                if not moves:
                    raise Problem(f"{q['file']}: quest '{q['id']}' step {k + 1} waits on a talk, but nothing advances it")
        if len(self.quests) > 64 or len(self.letters) > 64 or len(self.flags) > 256 or len(self.vars) > 32 or len(self.events) > 16:
            raise Problem("the save's story block is too small for this many quests/letters/flags/vars/events")

    # ---------------------------------------------------------------- output
    def cpp_str(self, s):
        for bad in ("“", "”", "‘", "’", "—", "…"):
            if bad in s:
                raise Problem(f"a curly quote, dash or ellipsis character in: {s[:60]} (the game's font has plain ones only)")
        return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'

    def arg(self, a):
        kind, v = a
        if kind == "quest":
            return f"kQ{camel(v)}"
        if kind == "person":
            return f"kP{camel(v)}"
        if kind == "flag":
            return f"kF{camel(v)}"
        if kind == "var":
            return f"kV{camel(v)}"
        if kind == "event":
            return f"kE{camel(v)}"
        if kind == "letter":
            return f"kL{camel(v)}"
        if kind == "raw":
            return v
        return str(int(v))

    # ---------------------------------------------------------------- save slots (story/ids.lock)
    LOCKED = ("quest", "flag", "var", "event", "letter")

    def assign_slots(self, lock_path):
        """Each saved name keeps its slot for good: the lock lists them in the order they were first
        built; a new name takes the next slot, a name gone keeps its slot empty."""
        lock = {k: [] for k in self.LOCKED}
        if os.path.exists(lock_path):
            for raw in open(lock_path, encoding="utf-8").read().split("\n"):
                raw = raw.strip()
                if raw and not raw.startswith("#"):
                    kind, name = raw.split()
                    lock[kind].append(name)
        now = {"quest": [q["id"] for q in self.quests], "flag": self.flags, "var": self.vars, "event": self.events,
               "letter": [l["id"] for l in self.letters]}
        for kind in self.LOCKED:
            for name in now[kind]:
                if name not in lock[kind]:
                    lock[kind].append(name)
        self.slot = {kind: {n: i for i, n in enumerate(lock[kind])} for kind in self.LOCKED}
        caps = {"quest": 64, "flag": 256, "var": 32, "event": 16, "letter": 64}
        for kind in self.LOCKED:
            if len(lock[kind]) > caps[kind]:
                raise Problem(f"story/ids.lock: {len(lock[kind])} {kind} slots, the save holds {caps[kind]}")
        self.lock_text = "# Save slots for the story's names (tools/story/build_story.py): never reorder or delete a line.\n" + \
            "".join(f"{kind} {n}\n" for kind in self.LOCKED for n in lock[kind])

    def ids(self):
        out = ["// Generated by tools/story/build_story.py from story/*.story: do not edit by hand.",
               "// The story's names for the game's code (D137).", "#pragma once", "", '#include "core/types.hpp"', "",
               "namespace ec::story {", ""]
        for name, items, prefix, typ, kind in (("Quest", [q["id"] for q in self.quests], "kQ", "u8", None),
                                                ("PersonId", [p["id"] for p in self.persons], "kP", "u8", None),
                                                ("FlagId", self.flags, "kF", "u16", "flag"), ("VarId", self.vars, "kV", "u8", "var"),
                                                ("EventId", self.events, "kE", "u8", "event"),
                                                ("LetterId", [l["id"] for l in self.letters], "kL", "u8", None)):
            out.append(f"enum {name} : {typ} {{")
            for k, v in enumerate(items):
                out.append(f"    {prefix}{camel(v)} = {self.slot[kind][v] if kind else k},")
            out.append("};")
            out.append(f"constexpr int k{name}Count = {len(items)};")
            out.append("")
        out += [f"constexpr int kPickupCount = {len(self.pickups)};", f"constexpr int kGroupCount = {max(1, len(self.groups))};", ""]
        for k, g in enumerate(self.groups):
            out.append(f"constexpr u8 kG{camel(g)} = {k};")
        out += ["", "}  // namespace ec::story", ""]
        return "\n".join(out)

    def data(self):
        o = ["// Generated by tools/story/build_story.py from story/*.story: do not edit by hand.",
             "// Included by core/story.cpp only (D137).", ""]
        o.append(f"const CondDef kConds[] = {{")
        for op, neg, a, b in self.conds or [(COND_OPS.index("true"), 0, ("int", 0), 0)]:
            o.append(f"    {{{op}, {neg}, {self.arg(a)}, {int(b) if not isinstance(b, tuple) else self.arg(b)}}},")
        o.append("};")
        o.append("const EffectDef kEffects[] = {")
        for op, a, b in self.effects or [(EFFECT_OPS.index("gleam"), ("int", 0), 0)]:
            o.append(f"    {{{op}, {self.arg(a)}, {int(b)}}},")
        o.append("};")
        pidx = {p["id"]: k for k, p in enumerate(self.persons)}
        o.append("const LineDef kLines[] = {")
        for text, feel, speaker, group, cond in self.lines:
            sp = pidx[speaker] if speaker else -1
            o.append(f"    {{{self.cpp_str(text)}, {feel}, {sp}, {group}, {cond[0]}, {cond[1]}}},")
        o.append("};")
        o.append("const RuleDef kRules[] = {")
        for r in self.rules or [{"person": self.persons[0]["id"], "cond": (0, 0), "eff": (0, 0), "line": (0, 0), "groups": 1, "important": 0}]:
            o.append(f"    {{{pidx[r['person']]}, {r['important']}, {r['groups']}, {r['cond'][0]}, {r['cond'][1]}, "
                     f"{r['eff'][0]}, {r['eff'][1]}, {r['line'][0]}, {r['line'][1]}}},")
        o.append("};")
        o.append("const StepDef kSteps[] = {")
        steps = []
        for q in self.quests:
            q["stepFrom"] = len(steps)
            steps += q["steps"]
        for s in steps:
            kind, args = s["where"]
            w = [0, 0, 0, 0]
            if kind in ("place", "lantern"):
                w[0] = PLACES[args[0]]
            elif kind == "cup":
                w[0] = CHALLENGES[args[0]]
            elif kind == "person":
                w[0] = pidx[args[0]]
            elif kind == "area":
                w = [PLACES[args[0]], int(float(args[1]) * 10), int(float(args[2]) * 10), int(float(args[3]) * 10)]
            elif kind == "spot":
                w = [PLACES[args[0]], int(float(args[1]) * 10), int(float(args[2]) * 10), 0]
            elif kind == "group":
                w[0] = self.groups.index(args[0])
            talk = pidx[s["talk"]] if s["talk"] else -1
            o.append(f"    {{{self.cpp_str(s['text'])}, {s['cond'][0]}, {s['cond'][1]}, {talk}, {WHERE_KINDS.index(kind)}, "
                     f"{w[0]}, {w[1]}, {w[2]}, {w[3]}}},")
        if not steps:
            o.append('    {"", 0, 0, -1, 0, 0, 0, 0, 0},')
        o.append("};")
        o.append("const QuestDef kQuests[] = {")
        for q in self.quests:
            giver = pidx[q["giver"]] if q["giver"] else -1
            o.append(f"    {{{self.cpp_str(q['id'])}, {self.slot['quest'][q['id']]}, {self.cpp_str(q['title'])}, {LINES.index(q['line'])}, {giver}, "
                     f"{int(q['gleam'])}, {q['auto']}, {q['cond'][0]}, {q['cond'][1]}, {self.cpp_str(q['rumour'])}, "
                     f"{q['stepFrom']}, {len(q['steps'])}, {q['reward'][0]}, {q['reward'][1]}}},")
        o.append("};")
        o.append("const LetterDef kLetters[] = {")
        for l in self.letters or []:
            o.append(f"    {{{self.cpp_str(l['id'])}, {self.slot['letter'][l['id']]}, {pidx[l['from']]}, {self.cpp_str(l['subject'])}, {l['cond'][0]}, "
                     f"{l['cond'][1]}, {l['eff'][0]}, {l['eff'][1]}, {l['line'][0]}, {l['line'][1]}}},")
        if not self.letters:
            o.append('    {"", 0, "", 0, 0, 0, 0, 0, 0},')
        o.append("};")
        o.append("const SpotDef kSpots[] = {")
        for s in self.spots:
            place, x, y, facing, clip = s["at"]
            o.append(f"    {{{pidx[s['person']]}, {place}, {int(float(x) * 10)}, {int(float(y) * 10)}, {int(facing * 100)}, "
                     f"{self.cpp_str(clip)}, {s['cond'][0]}, {s['cond'][1]}}},")
        if not self.spots:
            o.append('    {0, 0, 0, 0, 0, "", 0, 0},')
        o.append("};")
        o.append("const PickupDef kPickups[] = {")
        for p in self.pickups:
            o.append(f"    {{{self.cpp_str(p['id'])}, {p['place']}, {int(float(p['x']) * 10)}, {int(float(p['y']) * 10)}, "
                     f"{self.cpp_str(p['prompt'])}, {p['sign']}, {p['group']}, {p['glint']}, {p['cond'][0]}, {p['cond'][1]}, "
                     f"{p['eff'][0]}, {p['eff'][1]}, {p['line'][0]}, {p['line'][1]}}},")
        if not self.pickups:
            o.append('    {"", 0, 0, 0, "", 0, -1, 0, 0, 0, 0, 0, 0, 0},')
        o.append("};")
        o.append("const PersonDef kPersons[] = {")
        for p in self.persons:
            t = p["tint"]
            o.append(f"    {{{self.cpp_str(p['id'])}, {self.cpp_str(p['name'])}, {self.cpp_str(p['title'])}, {int(p['voice'])}, "
                     f"{p['pitch']:.3f}f, {p['villager']}, {self.cpp_str(p['body'])}, {self.cpp_str(p['portrait'])}, "
                     f"{{{t[0]}, {t[1]}, {t[2]}}}, kF{camel('met_' + p['id'])}}},")
        o.append("};")
        o.append(f"constexpr int kCondCount = {max(1, len(self.conds))}, kEffectCount = {max(1, len(self.effects))}, "
                 f"kLineCount = {len(self.lines)}, kRuleCount = {len(self.rules)}, kStepCount = {len(steps)}, "
                 f"kSpotCount = {len(self.spots)};")
        o.append("")
        return "\n".join(o)

    def index(self):
        o = ["# The story, as built (generated)", "",
             "*Written by `python tools/story/build_story.py --summary` from `story/*.story`; don't edit by hand.*", ""]
        for line in LINES:
            qs = [q for q in self.quests if q["line"] == line]
            if not qs:
                continue
            o.append(f"## {line.capitalize()}")
            o.append("")
            for q in qs:
                after = f" (after {', '.join(q['after'])})" if q["after"] else ""
                o.append(f"- **{q['title']}** (`{q['id']}`, {q['giver'] or 'no giver'}, {int(q['gleam'])} Gleam){after}")
                for k, s in enumerate(q["steps"]):
                    o.append(f"  {k + 1}. {s['text']}")
            o.append("")
        o.append("## Letters")
        o.append("")
        for l in self.letters:
            o.append(f"- **{l['subject']}** from {l['from']} (`{l['id']}`)")
        o.append("")
        o.append(f"{len(self.quests)} quests, {len(self.letters)} letters, {len(self.rules)} talk rules, "
                 f"{len(self.lines)} lines, {len(self.persons)} people, {len(self.pickups)} pickups and signs, "
                 f"{len(self.spots)} spots.")
        return "\n".join(o) + "\n"


def build(check_only=False, summary=False):
    st = Story()
    files = sorted(f for f in os.listdir(STORY) if f.endswith(".story"))
    order = ["people.story"] + [f for f in files if f != "people.story"]  # people first (their flags)
    for f in order:
        if os.path.exists(os.path.join(STORY, f)):
            st.parse_file(os.path.join(STORY, f))
    st.check()
    st.rules.sort(key=lambda r: r["tier"])  # (stable: the files' order within a tier)
    lock_path = os.path.join(STORY, "ids.lock")
    st.assign_slots(lock_path)
    ids, data = st.ids(), st.data()
    if not check_only:
        for path, text in ((OUT_IDS, ids), (OUT_INC, data), (lock_path, st.lock_text)):
            old = open(path, encoding="utf-8").read() if os.path.exists(path) else ""
            if old != text:
                open(path, "w", encoding="utf-8", newline="\n").write(text)
        if summary:
            open(OUT_INDEX, "w", encoding="utf-8", newline="\n").write(st.index())
    print(f"[story] {len(st.quests)} quests, {len(st.rules)} rules, {len(st.lines)} lines, {len(st.letters)} letters, "
          f"{len(st.persons)} people, {len(st.pickups)} pickups, {len(st.spots)} spots, {len(st.flags)} flags, "
          f"{len(st.vars)} vars")
    return st


if __name__ == "__main__":
    try:
        build("--check" in sys.argv, "--summary" in sys.argv)
    except Problem as e:
        print(f"[story] {e}")
        sys.exit(1)
