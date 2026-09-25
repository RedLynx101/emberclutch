"""Concept images for review R7 (Beta WP2, D74): AI-generated, reference only; nothing made
from them ships (everything in the game is built by script in Blender, as always).

  python tools/concept/make_concepts.py [--only name,name] [--model gpt-image-1] [--quality medium]

Calls the OpenAI images API with the standard library only (the key from OPENAI_API_KEY),
writes the full images to build/concept/<name>.png; tools/concept/to_jpg.ps1 makes the small
JPGs the review page shows.
"""
import base64
import json
import os
import sys
import urllib.error
import urllib.request

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "build", "concept")

STYLE = ("Concept art for a cozy Nintendo 3DS dragon-raising game. Low-poly, toon-shaded 3D look with soft flat "
         "colours and simple readable shapes, like a handheld game (not photoreal, not painterly detail), warm and "
         "friendly, gentle haze in the distance. Where the scene has dragons, they are cute and rounded (a turquoise Tide "
         "dragon, an orange Ember dragon with a cream belly, soft wing membranes); put none where it doesn't ask for them. "
         "No text, no logos, no UI.")

# Look B for R7: crisper, faceted low-poly 3D like the game's own dragons (flat-shaded facets,
# vertex colours, few textures), to set beside the softer storybook look above.
STYLE_LOWPOLY = ("Concept render for a cozy Nintendo 3DS dragon-raising game in crisp faceted low-poly 3D: flat-shaded "
                 "triangles you can count, vertex colours, almost no textures, clean silhouettes, like a handheld "
                 "3D game's real-time graphics with simple toon lighting and soft fog in the distance; warm, friendly "
                 "colours. Where the scene has dragons, they are cute faceted low-poly dragons (a turquoise Tide dragon, "
                 "an orange Ember dragon with a cream belly); put none where it doesn't ask for them. No text, no logos, "
                 "no UI.")

PROMPTS = {
    "valley_overview": (
        "Seen from high above, almost like a map (no characters in view), Skyreach Valley, a green bowl about a kilometre across ringed by snowy mountains. A river "
        "winds from the snowy northern heights into a round blue lake near the middle. On the west side a tall "
        "warm-grey cliff with a cave mouth (the dragons' den) and a waterfall pouring from the plateau above. East of "
        "the lake a small village with rounded warm roofs and market stalls on flat ground. A round hilltop with an "
        "ancient flat stone ring. A meadow with keepers' huts in the south-west, a circular arena in the south-east, "
        "a trailhead path climbing the southern hills, four small floating grassy islands in the sky, forests of "
        "simple cone-shaped pines. Afternoon light."),
    "den_cliff": (
        "The dragons' den in the valley: a round cave mouth in a tall warm-grey cliff, lanterns glowing at the "
        "entrance, a waterfall falling from the plateau beside it into a stream, a small grassy clearing in front "
        "where a young turquoise dragon plays with a ball while a keeper watches. Morning light."),
    "market_village": (
        "A cozy village square: market stalls under striped awnings in red, cream and teal, a stone well, strings "
        "of lanterns between wooden houses with rounded warm roofs, flower boxes, a few friendly villagers with cute "
        "rounded proportions, an orange dragon resting by a fruit stall. Midday."),
    "arena": (
        "A small open-air arena for dragon challenges: a round grassy field ringed by low stone walls and banners in "
        "six element colours (ember orange, tide teal, gale slate blue, grove green, frost ice blue, lumen gold), a "
        "wooden judge's stand, crystal lanterns on posts, glowing rings floating in the sky above for a flying race, "
        "a crowd of cheerful villagers. Bright day."),
    "nesting_stone": (
        "A hilltop at golden hour: an ancient flat stone ring with a bed of straw at its centre, two grown dragons "
        "nuzzling on it, a few standing stones, the green valley and the lake far below."),
    "floating_islands": (
        "Small floating grassy islands with rocky undersides and a few pine trees, drifting above the valley among "
        "soft clouds; a grown turquoise dragon with a small rider on its back glides between them, wings spread."),
    "player_character": (
        "Character turnaround sheet on a plain light background: the player, a young dragon keeper with cute "
        "rounded proportions (a large head, a small body), a simple outfit of tunic, satchel, scarf and boots; shown "
        "front, side and back, in two body shapes side by side, and a small riding pose on a dragon's back."),
    "villagers": (
        "Character lineup on a plain light background, cute rounded proportions: the old dragon keeper (a kind "
        "elderly person with a staff and a lantern), the market keeper (cheerful, apron, basket of fruit), the "
        "Sanctuary keeper (gentle, gardening gloves), the arena steward (a sash and a whistle), a curious child with "
        "a toy dragon, a traveller with a walking stick and backpack."),
    "palette_times": (
        "A palette study in four panels of the same view of the valley with its lake and village: dawn (soft pink "
        "and gold), day (clear blue sky, fresh greens), dusk (orange and violet), night (deep blue with warm lantern "
        "glows in the village and a starry sky); a small dragon in each panel for scale."),
    "keyart_flight": (
        "Key art: the player riding a grown turquoise dragon, wings spread wide, gliding low over the valley toward "
        "the lake at sunset, a floating island above, the village lights coming on below, a sense of freedom."),
}


def generate(name, prompt, model, quality, size, style=STYLE):
    body = json.dumps({"model": model, "prompt": style + "\n\n" + prompt, "size": size, "quality": quality,
                       "n": 1}).encode()
    req = urllib.request.Request("https://api.openai.com/v1/images/generations", data=body, method="POST",
                                 headers={"Authorization": "Bearer " + os.environ["OPENAI_API_KEY"],
                                          "Content-Type": "application/json"})
    try:
        with urllib.request.urlopen(req, timeout=300) as r:
            data = json.loads(r.read())
    except urllib.error.HTTPError as e:
        print(f"[concept] {name}: HTTP {e.code}: {e.read().decode(errors='replace')[:400]}")
        return False
    img = base64.b64decode(data["data"][0]["b64_json"])
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name + ".png")
    open(path, "wb").write(img)
    print(f"[concept] {path}: {len(img)} bytes")
    return True


def main():
    argv = sys.argv[1:]
    arg = lambda k, d: argv[argv.index(k) + 1] if k in argv else d
    only = arg("--only", "").split(",") if "--only" in argv else list(PROMPTS)
    model, quality = arg("--model", "gpt-image-1"), arg("--quality", "medium")
    ok = 0
    lowpoly = "--lowpoly" in argv  # look B: written as <name>_lowpoly.png
    for name in only:
        size = "1536x1024"
        ok += generate(name + ("_lowpoly" if lowpoly else ""), PROMPTS[name], model, quality, size,
                       STYLE_LOWPOLY if lowpoly else STYLE)
    print(f"[concept] {ok} of {len(only)} made")


if __name__ == "__main__":
    main()
