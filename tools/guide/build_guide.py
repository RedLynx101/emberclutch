"""The player's guide (1.0, R3: docs/plan/guide.md): docs/guide/guide.md into a printable page, then a PDF.

    py -3.12 tools/guide/build_guide.py [--pdf] [--pages]

Writes build/guide/guide.html (A5 portrait, the game's palette, Nunito and Cinzel Decorative from
assets/fonts, both OFL so they embed), and with --pdf prints it to build/guide/Emberclutch-Guide.pdf with
the headless Edge or Chrome already on the PC (nothing to install). --pages also renders each page to
build/guide/pages/page-NN.png (MiKTeX's pdftoppm) for the review page.

The Markdown is the small subset the guide uses: headings, paragraphs, lists, tables, a blockquote,
rules, images, bold, italics, code and links. Screenshots (the 3DS's 400 or 320 x 240) scale up crisply;
the first heading, image and line make the cover.
"""
import html
import re
import shutil
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "docs" / "guide" / "guide.md"
OUT = ROOT / "build" / "guide"
BROWSERS = [Path(r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"),
            Path(r"C:\Program Files\Google\Chrome\Application\chrome.exe")]

CSS = """
@font-face { font-family: "Nunito"; src: url("fonts/Nunito-SemiBold.ttf"); font-weight: 400 800; }
@font-face { font-family: "Cinzel Decorative"; src: url("fonts/CinzelDecorative-Bold.ttf"); }
@page { size: 148mm 210mm; margin: 14mm 13mm 15mm; background: #FFF8EC; }
@page :first { margin: 0; }
:root { --plum: #34233F; --dusk: #5E4466; --gold: #C9962B; --gold-soft: #F5C451; --shell: #FFF8EC; --line: #E6D6BC; }
html { -webkit-print-color-adjust: exact; print-color-adjust: exact; }
body { margin: 0; background: var(--shell); color: var(--plum); font-family: "Nunito", sans-serif;
       font-size: 9.6pt; line-height: 1.42; }
h1, h2, h3 { font-family: "Cinzel Decorative", serif; color: var(--dusk); line-height: 1.15; }
h2 { font-size: 15pt; margin: 0 0 3mm; padding-bottom: 1.5mm; border-bottom: 0.6mm solid var(--gold-soft);
     break-before: page; break-after: avoid; }
h3 { font-size: 10.5pt; margin: 4mm 0 1.5mm; break-after: avoid; }
p { margin: 0 0 2.2mm; }
ul { margin: 0 0 2.5mm; padding-left: 4.5mm; }
li { margin-bottom: 1mm; }
strong { font-weight: 800; color: #2A1A33; }
code { font-family: Consolas, monospace; font-size: 8.4pt; background: #F1E6D2; padding: 0 0.8mm; border-radius: 1mm; }
a { color: var(--dusk); }
hr { display: none; }
table { width: 100%; border-collapse: collapse; margin: 1mm 0 3mm; font-size: 8.6pt; break-inside: avoid; }
th, td { text-align: left; vertical-align: top; padding: 1mm 1.6mm; border-bottom: 0.25mm solid var(--line); }
th { background: #F3E3C4; color: var(--plum); font-weight: 800; }
blockquote { break-inside: avoid; margin: 2mm 0 3mm; padding: 2mm 3mm; background: #F6E9D3; border-left: 1mm solid var(--gold-soft);
             border-radius: 1mm; }
blockquote p { margin: 0; }
figure { margin: 2mm 0 3mm; text-align: center; break-inside: avoid; }
figure img { max-width: 100%; border-radius: 1.6mm; box-shadow: 0 0.6mm 1.6mm rgba(52, 35, 63, 0.25); }
figure img.screen { image-rendering: pixelated; width: 84mm; }
figure img.screen.bottom { width: 66mm; }
figcaption { font-size: 8pt; color: var(--dusk); margin-top: 1.2mm; font-style: italic; }
.cover { height: 210mm; box-sizing: border-box; padding: 26mm 12mm 0; text-align: center;
         background: linear-gradient(#7A6390, #4A3556); color: var(--shell); break-after: page; }
.cover h1 { font-size: 23pt; color: var(--gold-soft); margin: 0 0 3mm; }
.cover p { color: #F3E3C4; font-size: 10pt; }
.cover img { width: 118mm; margin-top: 10mm; border-radius: 3mm; box-shadow: 0 1mm 4mm rgba(0, 0, 0, 0.35); }
.cover .made { position: relative; top: 22mm; font-size: 8.4pt; color: #E6D6BC; }
"""


def png_size(p: Path):
    with open(p, "rb") as f:
        head = f.read(24)
    return struct.unpack(">II", head[16:24]) if head[:8] == b"\x89PNG\r\n\x1a\n" else (0, 0)


def inline(t: str) -> str:
    t = html.escape(t, quote=False)
    t = re.sub(r"`([^`]+)`", r"<code>\1</code>", t)
    t = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", t)
    t = re.sub(r"(?<![*\w])\*([^*]+)\*(?![*\w])", r"<em>\1</em>", t)
    t = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", r'<a href="\2">\1</a>', t)
    return t


def figure(alt: str, src: str) -> str:
    w, h = png_size(SRC.parent / src)
    cls = "screen" + (" bottom" if w == 320 else "") if w and w <= 400 else ""
    return f'<figure><img class="{cls}" src="{html.escape(src)}" alt="{html.escape(alt)}"><figcaption>{inline(alt)}</figcaption></figure>'


def convert(md: str) -> str:
    lines = md.splitlines()
    out, para, i = [], [], 0

    def flush():
        if para:
            out.append("<p>" + inline(" ".join(para)) + "</p>")
            para.clear()

    while i < len(lines):
        line = lines[i]
        s = line.strip()
        img = re.fullmatch(r"!\[([^\]]*)\]\(([^)]+)\)", s)
        if not s:
            flush()
        elif s.startswith("#"):
            flush()
            n = len(s) - len(s.lstrip("#"))
            out.append(f"<h{n}>{inline(s[n:].strip())}</h{n}>")
        elif s == "---":
            flush()
            out.append("<hr>")
        elif img:
            flush()
            out.append(figure(img.group(1), img.group(2)))
        elif s.startswith("|"):
            flush()
            rows = []
            while i < len(lines) and lines[i].strip().startswith("|"):
                rows.append([c.strip() for c in lines[i].strip().strip("|").split("|")])
                i += 1
            i -= 1
            head, body = rows[0], [r for r in rows[2:]]
            t = "<table>"
            if any(head):
                t += "<tr>" + "".join(f"<th>{inline(c)}</th>" for c in head) + "</tr>"
            for r in body:
                t += "<tr>" + "".join(f"<td>{inline(c)}</td>" for c in r) + "</tr>"
            out.append(t + "</table>")
        elif s.startswith("- "):
            flush()
            items = []
            while i < len(lines) and (lines[i].strip().startswith("- ") or (lines[i].startswith("  ") and lines[i].strip())):
                if lines[i].strip().startswith("- "):
                    items.append(lines[i].strip()[2:])
                else:
                    items[-1] += " " + lines[i].strip()
                i += 1
            i -= 1
            out.append("<ul>" + "".join(f"<li>{inline(x)}</li>" for x in items) + "</ul>")
        elif s.startswith(">"):
            flush()
            quote = []
            while i < len(lines) and lines[i].strip().startswith(">"):
                quote.append(lines[i].strip()[1:].strip())
                i += 1
            i -= 1
            out.append("<blockquote><p>" + inline(" ".join(quote)) + "</p></blockquote>")
        else:
            para.append(s)
        i += 1
    flush()
    return "\n".join(out)


def page(md: str) -> str:
    # The cover: the title, the line under it and the first picture.
    title = re.search(r"^# (.+)$", md, re.M).group(1)
    sub = re.search(r"^\*(.+)\*$", md, re.M).group(1)
    cover_img = re.search(r"!\[([^\]]*)\]\(([^)]+)\)", md)
    body = md[cover_img.end():] if cover_img else md
    cover = (f'<section class="cover"><h1>{inline(title)}</h1><p>{inline(sub)}</p>'
             f'<img src="{cover_img.group(2)}" alt="{html.escape(cover_img.group(1))}">'
             f'<p class="made">Made by Noah Hicks · free and open source · for the Nintendo 3DS</p></section>')
    return (f'<!doctype html><html lang="en"><head><meta charset="utf-8"><title>{html.escape(title)}</title>'
            f"<style>{CSS}</style></head><body>{cover}{convert(body)}</body></html>")


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    for d in ("img", "fonts"):
        shutil.rmtree(OUT / d, ignore_errors=True)
    shutil.copytree(SRC.parent / "img", OUT / "img")
    (OUT / "fonts").mkdir()
    shutil.copy(ROOT / "assets/fonts/nunito/Nunito-SemiBold.ttf", OUT / "fonts")
    shutil.copy(ROOT / "assets/fonts/cinzel-decorative/CinzelDecorative-Bold.ttf", OUT / "fonts")
    out = OUT / "guide.html"
    out.write_text(page(SRC.read_text(encoding="utf-8")), encoding="utf-8")
    print(f"[guide] {out}")
    if "--pdf" in sys.argv or "--pages" in sys.argv:
        browser = next((b for b in BROWSERS if b.exists()), None)
        if not browser:
            sys.exit("[guide] no Edge or Chrome found to print the PDF")
        pdf = OUT / "Emberclutch-Guide.pdf"
        pdf.unlink(missing_ok=True)
        subprocess.run([str(browser), "--headless=new", "--disable-gpu", "--no-pdf-header-footer", "--allow-file-access-from-files",
                        f"--print-to-pdf={pdf}", out.as_uri()], check=True, capture_output=True, timeout=120)
        if not pdf.exists():
            sys.exit("[guide] the browser wrote no PDF")
        print(f"[guide] {pdf} ({pdf.stat().st_size // 1024} KB)")
        if "--pages" in sys.argv:
            pages = OUT / "pages"
            pages.mkdir(exist_ok=True)  # (emptied, not removed: a shell may be sitting in it)
            for old in pages.glob("*.png"):
                old.unlink()
            subprocess.run(["pdftoppm", "-r", "80", "-png", str(pdf), str(pages / "page")], check=True)
            print(f"[guide] {len(list(pages.glob('*.png')))} pages -> {pages}")


if __name__ == "__main__":
    main()
