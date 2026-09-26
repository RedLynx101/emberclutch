"""Noah's working checklists as one page: the music and sounds still to make, and the steps
of the current hardware run, with boxes to tick and notes to write as he goes.

  python tools/checklists/make_checklists.py      -> build/checklists/emberclutch-checklists.html

Everything comes from the briefs and the run's steps in docs/, so the page follows them:
  docs/audio/suno-music-batch-3.md   the music cues (style, lyrics, what to keep)
  docs/audio/sfx-batch-2.md          sounds from batch 2 (any already in romfs/sfx/ are left out)
  docs/audio/sfx-batch-3.md          sounds from batch 3
  docs/plan/hardware-check-3.md      the run's sections and steps (from "# Run <RUN>")
The page is published as an Artifact with the `db` capability: the ticks and notes are kept
in its database (collections `checks` and `notes`), where Claude reads them back.
"""
import html
import json
import os
import re

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "build", "checklists", "emberclutch-checklists.html")
RUN = "Run 17"


def read(rel):
    return open(os.path.join(ROOT, rel), encoding="utf-8").read()


def inline(md):
    """The little Markdown the briefs use in running text, as HTML."""
    s = html.escape(md.strip(), quote=False)
    s = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", s)
    s = re.sub(r"`([^`]+)`", r"<code>\1</code>", s)
    s = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", s)
    s = re.sub(r"(?<![\w*])\*([^*]+)\*(?![\w*])", r"<em>\1</em>", s)
    return re.sub(r"\s*\n\s*", " ", s)


def paragraphs(text):
    out, cur = [], []
    for line in text.split("\n"):
        if line.strip() in ("", "---") or line.startswith(("|", "#", ">")):
            if cur:
                out.append(inline("\n".join(cur)))
                cur = []
            continue
        cur.append(line)
    if cur:
        out.append(inline("\n".join(cur)))
    return out


def music():
    s = read("docs/audio/suno-music-batch-3.md")
    table = {}
    for m in re.finditer(r"^\| (\d+) \| `([a-z0-9-]+)` \| ([^|]+)\| ([^|]+)\| ([^|]+)\| ([^|]+)\|$", s, re.M):
        table[m[2]] = dict(kind=m[3].strip(), plays=m[4].strip(), needed=m[5].strip(),
                           first=m[6].strip().strip("*").lower() == "first", optional="optional" in m[6].lower())
    items = []
    for m in re.finditer(r"\*\*Slug:\*\* `([a-z0-9-]+)`(.*?)(?=\*\*Slug:\*\*|\n## Processing|\Z)", s, re.S):
        slug, body = m[1], m[2]
        head = re.findall(r'^#{2,3} \d+\. "([^"]+)"', s[:m.start()], re.M)
        style = re.search(r"\*\*Style\*\*\n```\n(.*?)\n```", body, re.S)
        lyrics = re.search(r"\*\*Lyrics\*\*\n```\n(.*?)\n```", body, re.S)
        keep = re.search(r"\*\*Keep (?:a take|takes) that:\*\* (.*?)(?:\n\n|\Z)", body, re.S)
        mood = re.search(r"\*\*Mood:\*\*(.*?)\n\n", body, re.S)
        items.append(dict(id="m3." + slug, slug=slug, title=head[-1] if head else slug,
                          style=style[1].strip() if style else "", lyrics=lyrics[1].strip() if lyrics else "",
                          keep=inline(keep[1]) if keep else "", mood=inline(mood[1]) if mood else "",
                          **table.get(slug, dict(kind="", plays="", needed="", first=False, optional=False))))
    setup = re.search(r"Same setup as \[batch 1\].*?\n\n", s, re.S)
    exclude = re.search(r"\(`([^`]+)`\)", setup[0]) if setup else None
    return dict(items=items, setup=inline(setup[0]) if setup else "", exclude=exclude[1] if exclude else "")


def sounds(rel, batch):
    s = read(rel)
    have = set(f[:-4] for f in os.listdir(os.path.join(ROOT, "romfs", "sfx")) if f.endswith(".wav"))
    groups = []
    for sec in re.split(r"^## ", s, flags=re.M)[1:]:
        title, body = sec.split("\n", 1)
        rows = re.findall(r"^\| `([a-z0-9-]+)`([^|]*)\| ([^|]+)\| ([^|]+)\| ([^|]+)\|$", body, re.M)
        if not rows:
            continue
        items = [dict(id=f"s{batch}.{r[0]}", slug=r[0], tag=r[1].strip(), prompt=r[2].strip(), kind=r[3].strip(),
                      length=r[4].strip()) for r in rows if r[0] not in have]
        if items:
            first = "needed first" in title
            name = re.sub(r"^\d+\.\s*", "", re.sub(r"\s*\(.*?\)\s*$", "", title)).strip()
            groups.append(dict(id=f"s{batch}.{len(groups)}", title=name,
                               sub=(re.search(r"\((.*?)\)\s*$", title) or [None, ""])[1], first=first,
                               notes=paragraphs(body), items=items))
    return groups


def run_steps():
    s = read("docs/plan/hardware-check-3.md")
    s = s[s.index("# " + RUN):]
    s = s.split("\n# Run ", 1)[0]  # this run only, not the ones after it
    head, rest = s.split("\n", 1)
    intro_text = rest.split("\n## ", 1)[0]
    sections = []
    for sec in re.split(r"^## ", rest, flags=re.M)[1:]:
        title, body = sec.split("\n", 1)
        items, bullets, pre, cur = [], [], [], None
        for line in body.split("\n"):
            m = re.match(r"^(\d+)\. (.*)", line)
            if m:
                cur = [m[2]]
                items.append(cur)
            elif line.startswith("- "):
                cur = [line[2:]]
                bullets.append(cur)
            elif cur is not None and line.startswith("  ") and line.strip():
                cur.append(line.strip())
            elif line.strip() and not items and not bullets:
                pre.append(line)
            elif not line.strip():
                cur = None
                if not items and not bullets:
                    pre.append("")  # keeps the lead's paragraphs apart
        n = re.match(r"(\d+)\. (.*)", title)
        sid = f"r{RUN.split()[1]}.{n[1] if n else 'end'}"
        steps = [dict(id=f"{sid}-{i + 1}", text=inline("\n".join(it))) for i, it in enumerate(items)]
        lead = paragraphs("\n".join(pre))
        if not steps and not bullets and lead:  # a section that is one thing to do (Install)
            steps, lead = [dict(id=f"{sid}-1", text=lead[0])], lead[1:]
        sections.append(dict(id=sid, title=n[2] if n else title.strip(), lead=lead, steps=steps,
                             asks=[inline("\n".join(b)) for b in bullets]))
    return dict(title=head[2:].strip(), intro=paragraphs(intro_text), sections=sections)


def main():
    data = dict(music=music(), sounds=[dict(batch=2, groups=sounds("docs/audio/sfx-batch-2.md", 2)),
                                       dict(batch=3, groups=sounds("docs/audio/sfx-batch-3.md", 3))],
                run=run_steps())
    page = open(os.path.join(os.path.dirname(__file__), "template.html"), encoding="utf-8").read()
    page = page.replace("/*DATA*/", json.dumps(data, ensure_ascii=False).replace("</", "<\\/"))
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    open(OUT, "w", encoding="utf-8", newline="\n").write(page)
    n_m = len(data["music"]["items"])
    n_s = [sum(len(g["items"]) for g in b["groups"]) for b in data["sounds"]]
    n_r = sum(len(x["steps"]) for x in data["run"]["sections"])
    print(f"[checklists] {OUT}: {n_m} music cues, sounds {n_s[0]} (batch 2) + {n_s[1]} (batch 3), {n_r} run steps")


if __name__ == "__main__":
    main()
