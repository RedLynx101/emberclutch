"""Concept images for review R7 (Beta WP2, D74): AI-generated, reference only; nothing made
from them ships (everything in the game is built by script in Blender, as always).

  python tools/concept/make_concepts.py [--only name,name] [--model gpt-image-1] [--quality medium]
  python tools/concept/make_concepts.py --lowpoly ...    (R7's look B)
  python tools/concept/make_concepts.py --dragons ...    (R11: the new dragons' growth sheets)
  python tools/concept/make_concepts.py --looklab ...    (the look lab: faceted, and faceted + painted)

Calls the OpenAI images API with the standard library only (the key from OPENAI_API_KEY),
writes the full images to build/concept/<name>.png; tools/concept/to_jpg.ps1 makes the small
JPGs the review pages show.
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

# The look lab (2026-09-28, Noah: "consider a low poly option for terrain"): the valley in crisp
# faceted low-poly, flat-shaded, and the same with soft painted textures on the facets.
STYLE_FACETED = ("Screenshot-like concept render for a cozy Nintendo 3DS dragon game, real-time low-poly 3D with a "
                 "deliberate faceted style: the ground is large flat-shaded triangles you can count, each face one "
                 "flat colour lit by the sun on its own slope, crisp bands of colour for grass, earth paths, sand and "
                 "rock, chunky faceted trees (round two-tone crowns and simple cone pines), faceted mountains with "
                 "snow caps against a clear blue sky, light haze only far away. Clean, readable, charming, like a "
                 "polished handheld game. No text, no logos, no UI.")
STYLE_FACETED_PAINTED = ("Screenshot-like concept render for a cozy Nintendo 3DS dragon game, real-time low-poly 3D: "
                         "large faceted flat-shaded triangles for the ground and mountains, but each facet carries a "
                         "soft hand-painted texture (painterly grass strokes, speckled earth paths, stone grain), "
                         "chunky faceted trees with lightly painted leaves, snow-capped faceted mountains, a clear sky. "
                         "Warm storybook colours, readable at a handheld's small screen. No text, no logos, no UI.")
LOOKLAB_PROMPTS = {
    "lab_walk": (
        "A third-person view from just behind and above a small cute keeper character walking a young orange "
        "dragon on a red lead along an earth path through a green meadow; ahead the path crosses a stone bridge "
        "by a little windmill on a river, then climbs toward a ring of snowy mountains; scattered round trees and "
        "pine woods. Late morning."),
    "lab_den": (
        "A warm grey cliff with a round cave mouth (a dragons' den) with lanterns, a waterfall pouring from the "
        "plateau above into a round pool, a small wooden lodge beside it, a clearing where a turquoise dragon "
        "stands; forest and mountains beyond."),
    "lab_lake": (
        "Seen from high above (from a flying dragon's back), a round blue lake with a wooden jetty, a village of "
        "rounded roofs beside it, a circular arena, earth paths between them, woods, two small floating grassy "
        "islands in the sky, snowy mountains around the valley."),
}

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

# R11 (after R7 picked look A, D75): four wholly new dragon silhouettes in the storybook look,
# a cozy life-sim feel crossed with a dragon-riding film's lovable, readable dragons, and a
# touch of majesty. One growth sheet per kind, then all four side by side.
STYLE_DRAGONS = ("Concept art sheet for a cozy handheld dragon-raising game, soft storybook 3D style of a cozy "
                 "life-sim game: chunky rounded toy-like shapes, hand-painted soft textures with simple patterns "
                 "(spots, stripes, soft gradients; no fine scale detail), soft warm lighting with gentle shading, rich "
                 "friendly colours, big glossy expressive eyes, clean readable silhouettes. Every dragon has a soft "
                 "heart-shaped glow in its chest. The dragons are wholly original designs that resemble no existing "
                 "film or game character. The background is one plain, flat, pale cream colour across the whole image (no "
                 "coloured backdrop, no vignette). No text, no labels, no logos, no UI.")
GROWTH = ("A growth lineup on one line, all in side view facing left, the same individual at each age, growing in "
          "size from left to right: its egg, the tiny hatchling, the juvenile, the adolescent, and the grown adult "
          "(much larger, at the right). ")
DRAGON_PROMPTS = {
    "dragon_pouncer": GROWTH + (
        "The kind: a sleek, cat-like dragon, ember orange with a cream belly and darker stripes. Egg: smooth, "
        "cream with orange flame speckles. Hatchling: a round kitten-like baby with an oversized head, huge round "
        "eyes, tiny paws, stubby wing nubs, a little tail with a leaf-shaped fin. Juvenile: a lanky, playful "
        "kitten-like dragon with soft swept-back horns and half-grown wings. Adolescent: long-legged and agile. "
        "Adult: a graceful panther-like dragon, low and lithe, a long tail ending in twin fins, large bat-like "
        "wings with scalloped edges, swept-back horns, a calm proud pose; cool and awe-inspiring but kind."),
    "dragon_puffback": GROWTH + (
        "The kind: a round, gentle giant of a dragon, leaf green with a cream belly and a mossy back. Egg: "
        "almost round, mossy green with pale spots. Hatchling: a nearly spherical baby like a round bun, tiny stubby "
        "legs, tiny wings, a short thick tail, a little leaf sprout on its head. Juvenile: a chubby round pup-like "
        "dragon with small fast wings. Adolescent: bigger and sturdier, the first rounded plates on its back. Adult: "
        "a huge, round, heavy and very gentle dragon like a walking hill, a broad back covered with rounded mossy "
        "plates, little flowers and ferns growing on them, small wings that look too small for it, a club-like "
        "tail, stout legs, a kind sleepy face; awe from its size and calm."),
    "dragon_crestwing": GROWTH + (
        "The kind: a feathered dragon, slate blue with gold, clearly a dragon and not a bird: a dragon's snout with "
        "nostrils and a gentle smile (no beak), a scaled body and belly, four legs (strong hind legs and smaller "
        "front legs with little claws, both always visible), a long dragon tail; feathers only as a crest on the "
        "head, feathered wing edges and plumes at the tail's end. Egg: tall, pale blue with gold speckles. "
        "Hatchling: a fluffy baby dragon with a downy crest, huge eyes, stubby four legs, tiny wings. Juvenile: "
        "gangly and leggy, a growing crest of soft feather-like spines. Adolescent: taller, its tail plumes growing. "
        "Adult: a tall, elegant dragon with a long graceful neck, a crest of feather-like spines, wide wings with "
        "feathered edges, a fan of long flowing plumes at the end of its tail in gold and blue; pretty and "
        "majestic, like a crane or a peacock made dragon."),
    "dragon_ribbontail": GROWTH + (
        "The kind: a long, serpentine dragon, sea teal with pearly white. Egg: pearly with a soft spiral pattern "
        "like a seashell. Hatchling: a little noodle-shaped baby with a round head, big eyes, four tiny legs, "
        "fin-like ear frills, curled up. Juvenile: longer and wiggly, small fin-wings. Adolescent: long, with "
        "ribbon-like fins along its back. Adult: a long flowing serpentine dragon with four short legs, flowing "
        "ribbon-like fins along its back and tail, long soft whiskers, frilled fin-wings, drifting gracefully in "
        "the air in an S-curve; majestic and pretty, like a river spirit."),
    # R11b (D78): the other four base breeds and the first crossbreed, by the same recipe.
    "dragon_flurrytail": GROWTH + (
        "The kind: a snow-fox dragon (Frost), snow white with pale ice-blue markings and silver-lilac tips. Egg: "
        "round, frosted white with pale blue snowflake speckles. Hatchling: a round fluffball with a huge "
        "pom-pom tail, big eyes, tiny crystal antler buds, stubby legs. Juvenile: fox-like and bouncy, a soft "
        "fluffy chest ruff, the plumed tail growing bigger than its body. Adolescent: leggier, small crystal "
        "antlers. Adult: an elegant fox-like dragon with a thick fluffy mane and chest ruff, a huge fluffy plumed "
        "tail curling up over its back, branching ice-crystal antlers, medium wings with frosted scalloped edges "
        "like icicles, snowflake-like markings on its flanks; majestic like a snow spirit."),
    "dragon_curlstone": GROWTH + (
        "The kind: an armoured stone dragon (Stone) like a pangolin or armadillo, warm sandstone and slate grey "
        "with glowing amber geode crystals peeking between its plates. Egg: a smooth river-stone egg, grey with "
        "sandy bands. Hatchling: a little round pebble of a baby, curled half into a ball, tiny overlapping plates, "
        "big eyes and a long button snout. Juvenile: low and long, rows of rounded overlapping plates down its back "
        "and tail, a longish snout for digging, strong front claws, tiny wings tucked under the plates. Adolescent: "
        "bigger plates, small crystal points along its spine. Adult: a low, long, sturdy dragon covered in "
        "rounded overlapping stone plates like a pangolin, a long gentle snout, big digging claws, a thick tail "
        "ending in a rounded stone club, short sturdy wings folded under the plates, a crown of amber crystals "
        "on its head; a calm mountain guardian."),
    "dragon_glimmermoth": GROWTH + (
        "The kind: a moth dragon of light (Lumen), cream white and soft gold with sunny yellow eyespots. Egg: "
        "pearly cream, softly glowing, with gold rings. Hatchling: a round fuzzy baby with a fluffy collar, tiny "
        "feathery antennae, four little rounded wing buds, huge dark shiny eyes. Juvenile: slender and fuzzy, "
        "two pairs of small rounded wings. Adolescent: longer wings with the first glowing eyespots. Adult: a "
        "slender, graceful dragon with a big fluffy white collar ruff, long feathery antennae, FOUR broad soft "
        "moth wings (a pair of forewings and a pair of hindwings) with glowing golden sun-ring eyespots, the "
        "hindwings ending in long trailing tails like a luna moth, a long thin tail with a fluffy tip; luminous "
        "and majestic."),
    "dragon_duskwing": GROWTH + (
        "The kind: a bat-winged night dragon (Shade), deep indigo and violet with a soft lavender belly and tiny "
        "glowing star speckles on its wings. It is a wyvern: it stands on two strong hind legs and its forelegs "
        "ARE its wings, and it walks on the wings' folded wrists like a bat. Egg: dark indigo with tiny silver "
        "star speckles. Hatchling: a round fluffball with enormous bat ears, huge eyes, tiny wing-arms, stubby "
        "hind legs. Juvenile: big ears, a fluffy neck ruff, leathery wing-arms it leans on. Adolescent: longer "
        "wings, a long whip tail. Adult: a sleek, elegant wyvern with tall bat ears, a fluffy neck ruff, huge "
        "wing-arms whose membranes look like a night sky full of little stars, a long whip tail ending in a "
        "crescent-moon shape; mysterious and majestic but kind."),
    "dragon_blazeplume": GROWTH + (
        "The kind: a phoenix-like crossbreed of a sleek cat-like dragon and a feathered dragon (Ember and Gale). It "
        "has a CAT-LIKE dragon head with a short rounded snout, whiskers and pointed ears (no beak), four sturdy cat "
        "legs of equal length, and a lithe, cat-like body in flame orange and scarlet with a golden belly, feathered wings whose feathers "
        "shade from gold to scarlet to a hot orange tip, a crest of flame-shaped feathers on its head, and a long "
        "tail ending in a spray of fiery tail plumes that glow at the tips. Egg: warm gold with red flame "
        "swirls. Hatchling: a round kitten-like chick with fluffy flame-coloured down and a little feather "
        "crest. Juvenile: lanky and cat-like with half-grown feathered wings. Adolescent: longer plumes. Adult: "
        "a graceful, majestic firebird dragon, cat-like and proud, wings of flame-coloured feathers spread wide."),
    # R12 (DR4, D83): the next five crossbreeds.
    "dragon_kindlemoss": GROWTH + (
        "The kind: a crossbreed of a sleek cat-like fire dragon and a round mossy gentle-giant dragon (Ember and "
        "Grove). A plump, cosy moss-lynx: a round-bellied cat-like dragon body carpeted in soft green moss, a cat face "
        "with a short rounded snout and lynx ear tufts made of little fern fronds, curled fern fronds along its back "
        "shaped like little flames with glowing ember-orange tips, a few glowing ember berries in the moss, small "
        "stubby leafy wings, and a bushy fern-frond tail ending in a glowing ember. Egg: mossy green with ember "
        "speckles. Hatchling: a fluffy round moss kitten with a single sprout on its head whose bud glows like an "
        "ember. Adult: plump, warm and gentle, a walking hearth in a forest."),
    "dragon_cindershell": GROWTH + (
        "The kind: a crossbreed of a sleek cat-like fire dragon and a pangolin-like stone dragon that curls into a "
        "ball (Ember and Stone). A lava-pangolin panther: a cat-like dragon head with tall ears capped in smooth "
        "stone, a body covered in overlapping dark obsidian plates with glowing orange magma seams between them, "
        "four sturdy legs, short sturdy wings with stone-edged membranes, a tail of stacked plates ending in a "
        "glowing coal; it can roll up into a glowing ball. Egg: black with glowing orange cracks. Hatchling: a round "
        "pebble-like kitten with one glowing crack down its back. Adult: strong, calm and warm, like a friendly "
        "volcano."),
    "dragon_bloomstone": GROWTH + (
        "The kind: a crossbreed of a round mossy gentle-giant dragon and a pangolin-like stone dragon (Grove and "
        "Stone). A walking rock garden: a stout, tortoise-like dragon (still a dragon: a dragon head with a short "
        "snout and a sleepy gentle smile, no shell opening) whose domed stone back is a tiny garden of moss, "
        "little flowers, pebbles and one small sapling, stubby elephant-like legs, tiny leaf-shaped wings, and a "
        "mossy rock club at the end of its short tail. Egg: grey speckled stone with a flower pattern. Hatchling: "
        "a little pebble with tiny legs, big eyes and one flower growing on top. Adult: huge, slow, peaceful."),
    "dragon_lilyfin": GROWTH + (
        "The kind: a crossbreed of a round mossy gentle-giant dragon and a long serpentine water dragon with ribbon "
        "fins (Grove and Tide). A marsh axolotl-dragon: a soft, long newt-like dragon body on four short webbed "
        "legs, frilly gill fronds like fern leaves round its head, a lily pad on its head worn like a hat with a "
        "pink lotus flower, a long paddle tail with reed-like fins, small leafy wings that flutter, teal green with "
        "a pale belly and dappled spots. Egg: pale jade with lily-pad rings. Hatchling: a tiny tadpole-like newt "
        "baby with a little lily pad on its head. Adult: graceful and calm, gliding through ponds."),
    "dragon_frostcurl": GROWTH + (
        "The kind: a crossbreed of a pangolin-like stone dragon that curls into a ball and a snow-fox dragon with "
        "a huge fluffy tail (Stone and Frost). A glacier armadillo-fox: fluffy snow-white fur on its face, chest, "
        "belly and a huge plume of a tail, a back covered in translucent ice-blue crystal plates, frosty crystal "
        "tips on its fox-like ears, small crystalline wings, a fox-like dragon face with a short snout; it curls up "
        "under its plates into a snowball. Egg: white with ice-blue crystal facets. Hatchling: a round snowball "
        "with tiny ice-plate nubs and a fluffy tail. Adult: elegant, cool and kind."),
    "dragon_adults": (
        "Four grown dragons side by side, full body, to scale: a sleek panther-like ember orange dragon with bat-like "
        "wings and a twin-finned tail; a huge round leaf-green gentle giant with mossy flowering plates on its back "
        "and small wings; a tall, elegant slate blue feathered dragon on four long legs (a dragon's snout, no beak) "
        "with a feather-spine crest, wings with gold feathered edges and long gold plumes at its tail's end; a long serpentine sea-teal dragon with ribbon fins and whiskers "
        "floating in an S-curve. A small cute keeper character stands beside them for scale. All four have the same "
        "kind of rounded dragon head with a short snout, nostrils and a gentle smile; none has a beak. The feathered "
        "one is a dragon with scales on its body and belly and a long dragon tail, feathers only on its crest, wing "
        "edges and tail tip."),
    "dragon_hatchlings": (
        "Four baby dragons playing together on a soft grassy patch: a round kitten-like ember orange baby with huge "
        "eyes and wing nubs; a round bun-shaped leaf-green baby with a leaf sprout on its head; a fluffy slate blue "
        "baby dragon with a downy crest, a little snout (no beak) and four stubby legs; a little noodle-shaped sea-teal baby with fin-like ear "
        "frills. Adorable, bouncy, each clearly its own silhouette."),
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
    if "--looklab" in argv:  # the look lab: each scene faceted, and faceted with painted textures
        only = arg("--only", "").split(",") if "--only" in argv else list(LOOKLAB_PROMPTS)
        for name in only:
            ok += generate(name + "_faceted", LOOKLAB_PROMPTS[name], model, quality, "1536x1024", STYLE_FACETED)
            ok += generate(name + "_painted", LOOKLAB_PROMPTS[name], model, quality, "1536x1024", STYLE_FACETED_PAINTED)
        print(f"[concept] {ok} of {2 * len(only)} made")
        return
    if "--dragons" in argv:  # R11: the new dragons' sheets
        only = arg("--only", "").split(",") if "--only" in argv else list(DRAGON_PROMPTS)
        for name in only:
            ok += generate(name, DRAGON_PROMPTS[name], model, quality, "1536x1024", STYLE_DRAGONS)
        print(f"[concept] {ok} of {len(only)} made")
        return
    for name in only:
        size = "1536x1024"
        ok += generate(name + ("_lowpoly" if lowpoly else ""), PROMPTS[name], model, quality, size,
                       STYLE_LOWPOLY if lowpoly else STYLE)
    print(f"[concept] {ok} of {len(only)} made")


if __name__ == "__main__":
    main()
