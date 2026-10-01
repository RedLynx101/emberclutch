"""The people's colours (sRGB 0-255, as the 3DS shows them: the lead's table generator can copy
them as they are; `linear()` gives the 0-1 linear values the Blender previews use).

A person is drawn with the dragons' shader: every vertex names two palette slots, a mix and an
emissive weight (PAINT below), and each person uploads its own palette (src/core/model.hpp
Palette). The slots as the people use them:

  base      skin                      accent   outfit (the main garment)
  pattern   outfit trim (scarf, cuffs, apron, cap ...)
  horn      hair (and brows, beards)  membrane leather and wood (boots, belts, bags, staffs)
  iris      eye colour                pupil    pupils, the mouth, dark soles
  glint     eye glints (emissive), white bits
  glow      a villager's third colour, or a lantern's flame (emissive where painted "flame")
  tongue    rosy: blush, the nose's tint, the mouth's warmth

The creator (D74) changes base (SKIN_TONES), horn (HAIR_COLOURS), and accent + pattern
(OUTFITS); the other slots are PLAYER_FIXED. Each villager has a whole palette (VILLAGERS).
"""

BASE, ACCENT, PATTERN, HORN, MEMBRANE, IRIS, PUPIL, GLINT, GLOW, TONGUE = range(10)
SLOTS = ["base", "accent", "pattern", "horn", "membrane", "iris", "pupil", "glint", "glow", "tongue"]

# material -> (palette A, palette B, mix 0-255 toward B, emissive 0-255)
PAINT = {
    "skin": (BASE, BASE, 0, 0),
    "blush": (BASE, TONGUE, 125, 0),
    "nose": (BASE, TONGUE, 38, 0),
    "mouth": (PUPIL, TONGUE, 95, 0),
    "outfit": (ACCENT, ACCENT, 0, 0),
    "outfit_shade": (ACCENT, PUPIL, 55, 0),   # the inside of a hood, a coat's lining
    "trim": (PATTERN, PATTERN, 0, 0),
    "trim_shade": (PATTERN, PUPIL, 45, 0),
    "hair": (HORN, HORN, 0, 0),
    "hair_shade": (HORN, PUPIL, 40, 0),
    "leather": (MEMBRANE, MEMBRANE, 0, 0),
    "leather_dark": (MEMBRANE, PUPIL, 105, 0),  # trousers, leggings
    "sole": (PUPIL, MEMBRANE, 70, 0),
    "extra": (GLOW, GLOW, 0, 0),
    "extra_dark": (GLOW, PUPIL, 70, 0),
    "white": (GLINT, GLINT, 0, 0),
    "plush": (ACCENT, GLINT, 120, 0),         # the child's toy dragon: a paler outfit colour
    "metal": (GLINT, PUPIL, 120, 0),
    "brass": (PATTERN, GLINT, 70, 0),
    "flame": (GLINT, GLOW, 150, 235),
    "iris": (IRIS, IRIS, 0, 0),
    "pupil": (PUPIL, PUPIL, 0, 0),
    "glint": (GLINT, GLINT, 0, 200),
    "heart": (TONGUE, TONGUE, 0, 90),         # heart eyes (faces.py)
    "tongue": (TONGUE, PUPIL, 25, 0),         # inside an open mouth
}

# ---------------------------------------------------------------------------- the creator
SKIN_TONES = [  # light to deep
    ("Peach", (255, 222, 196)),
    ("Honey", (240, 192, 152)),
    ("Sand", (212, 158, 112)),
    ("Umber", (164, 108, 72)),
    ("Cocoa", (112, 70, 48)),
]
HAIR_COLOURS = [
    ("Chestnut", (132, 78, 44)),
    ("Dark brown", (82, 54, 40)),
    ("Black", (46, 40, 46)),
    ("Ginger", (208, 104, 50)),
    ("Golden", (236, 194, 104)),
    ("Silver", (220, 222, 232)),
]
OUTFITS = [  # accent (the tunic), pattern (scarf, trim)
    ("Moss", (126, 152, 76), (216, 102, 60)),
    ("Sky", (92, 146, 198), (248, 222, 150)),
    ("Berry", (186, 74, 90), (246, 200, 110)),
]
PLAYER_FIXED = {
    "membrane": (126, 84, 54),   # boots, belt, bag
    "iris": (86, 56, 40),
    "pupil": (32, 24, 30),
    "glint": (255, 255, 255),
    "glow": (250, 238, 210),     # body B's egg in its pouch
    "tongue": (236, 124, 124),
}


def player_palette(skin=0, hair=0, outfit=0):
    """The ten slots for a creator choice (indices into the lists above)."""
    p = dict(PLAYER_FIXED)
    p["base"] = SKIN_TONES[skin][1]
    p["horn"] = HAIR_COLOURS[hair][1]
    p["accent"], p["pattern"] = OUTFITS[outfit][1], OUTFITS[outfit][2]
    return p


# ---------------------------------------------------------------------------- the villagers
_EYES = {"pupil": (30, 24, 30), "glint": (255, 255, 255), "tongue": (234, 122, 122)}
VILLAGERS = {
    # the old dragon keeper: a teal long coat with gold trim, white beard and hair, a wooden staff
    "keeper": dict(_EYES, base=(238, 200, 170), accent=(62, 112, 124), pattern=(226, 180, 88),
                   horn=(240, 238, 232), membrane=(134, 92, 58), iris=(80, 100, 120),
                   glow=(236, 222, 190)),       # glow: unused (free for a later detail)
    # the Market's keeper: a marigold dress, a cream apron, a teal headscarf
    "market": dict(_EYES, base=(228, 172, 130), accent=(218, 136, 70), pattern=(250, 238, 214),
                   horn=(128, 70, 42), membrane=(84, 146, 160), iris=(92, 60, 40),
                   glow=(112, 72, 50)),          # glow: her shoes
    # the Sanctuary's keeper: denim overalls over a wheat shirt, a straw hat, a tin bucket
    "sanctuary": dict(_EYES, base=(200, 142, 102), accent=(82, 118, 170), pattern=(238, 212, 142),
                      horn=(116, 58, 40), membrane=(232, 198, 118), iris=(70, 110, 70),
                      glow=(104, 76, 56)),       # glow: boots
    # the arena's steward: a red tabard with gold trim, a cream shirt, a fox's tail
    "steward": dict(_EYES, base=(242, 204, 176), accent=(180, 62, 66), pattern=(236, 192, 86),
                    horn=(210, 112, 54), membrane=(116, 82, 56), iris=(96, 70, 40),
                    glow=(242, 236, 222)),       # glow: shirt
    # the child: a red cap, a sunny t-shirt, brown shorts, white trainers
    "child": dict(_EYES, base=(252, 216, 188), accent=(214, 86, 74), pattern=(252, 206, 90),
                  horn=(150, 92, 50), membrane=(116, 98, 78), iris=(84, 60, 44),
                  glow=(244, 244, 238)),         # glow: trainers
    # the traveller: a forest-green cloak, a sand tunic, a great leather pack, a lantern
    "traveller": dict(_EYES, base=(206, 150, 112), accent=(82, 112, 78), pattern=(216, 198, 162),
                      horn=(88, 62, 48), membrane=(130, 90, 58), iris=(76, 96, 60),
                      glow=(255, 196, 96)),      # glow: the lantern's flame
}

# ---------------------------------------------------------------------------- the story's people (D138)
STORY = {
    # Fig: a moss tunic, a rust-red hat and scarf far too big for him, a pale gold feather, sandy hair
    "fig": dict(_EYES, base=(246, 206, 170), accent=(104, 142, 76), pattern=(176, 70, 60), horn=(176, 108, 58),
                membrane=(120, 82, 52), iris=(90, 130, 70), glow=(250, 236, 150)),
    # Tam: dark green waders, a cream knit jumper, a yellow sou'wester, greying hair
    "tam": dict(_EYES, base=(222, 170, 128), accent=(60, 84, 74), pattern=(236, 224, 196), horn=(150, 140, 130),
                membrane=(110, 80, 56), iris=(80, 110, 140), glow=(246, 200, 70)),
    # Tove: a winter-blue coat with white fur, a cranberry knit hat and mittens, a pale blonde bob
    "tove": dict(_EYES, base=(238, 214, 200), accent=(84, 116, 160), pattern=(176, 60, 70), horn=(226, 214, 180),
                 membrane=(90, 70, 60), iris=(110, 150, 190), glow=(240, 240, 250)),
    # Linnet: a plum jacket and hat, a blush-pink skirt and hatband, a yellow tape measure and flower
    "linnet": dict(_EYES, base=(250, 214, 190), accent=(150, 86, 150), pattern=(240, 214, 226), horn=(100, 60, 44),
                   membrane=(110, 74, 60), iris=(110, 80, 60), glow=(246, 214, 90)),
    # Madder: a madder-red apron, an indigo bandana, black curls, hands dyed purple
    "madder": dict(_EYES, base=(180, 120, 86), accent=(190, 80, 70), pattern=(90, 70, 150), horn=(40, 32, 36),
                   membrane=(100, 70, 50), iris=(70, 50, 40), glow=(130, 80, 170)),
    # Celestine: a magenta gown, a pink boa, a fiery red updo, a gold star pin
    "celestine": dict(_EYES, base=(236, 190, 160), accent=(200, 70, 130), pattern=(246, 200, 224),
                      horn=(210, 70, 50), membrane=(120, 80, 60), iris=(120, 80, 160), glow=(255, 224, 120)),
    # Primrose: pink and cream, deep pink bows, blonde ringlets
    "primrose": dict(_EYES, base=(255, 226, 210), accent=(240, 150, 180), pattern=(220, 90, 130),
                     horn=(250, 226, 150), membrane=(130, 90, 70), iris=(90, 140, 190), glow=(255, 240, 246)),
    # The champions keep their league colours (core/league): Marigold's ember dress and gold, a straw hat
    "marigold": dict(_EYES, base=(255, 222, 196), accent=(214, 90, 56), pattern=(245, 196, 81), horn=(220, 120, 40),
                     membrane=(232, 198, 118), iris=(118, 110, 124), glow=(255, 226, 110)),
    # Rook: a navy coat with gold (brass) trim, black hair, dark boots
    "rook": dict(_EYES, base=(164, 108, 72), accent=(60, 70, 110), pattern=(245, 196, 81), horn=(46, 40, 46),
                 membrane=(52, 40, 40), iris=(86, 56, 40), glow=(240, 236, 230)),
    # Seraphine: a wine bodice, a dark plum wrap skirt, black hair
    "seraphine": dict(_EYES, base=(212, 158, 112), accent=(120, 40, 70), pattern=(70, 34, 74), horn=(30, 24, 36),
                      membrane=(40, 30, 40), iris=(118, 110, 124), glow=(230, 180, 90)),
    # Solenne (run 22, Noah: after Emilia): fair, violet eyes, silver hair with a lavender tint, a lavender
    # capelet trimmed in purple over a white dress
    "solenne": dict(_EYES, base=(255, 234, 224), accent=(226, 214, 244), pattern=(140, 96, 190), horn=(238, 234, 248),
                    membrane=(200, 190, 210), iris=(152, 104, 200), glow=(196, 222, 250)),
}


def linear(c):
    """sRGB 0-255 -> linear 0-1 (the dragons' VARIANTS space; Blender previews)."""
    out = []
    for v in c:
        s = v / 255.0
        out.append(s / 12.92 if s <= 0.04045 else ((s + 0.055) / 1.055) ** 2.4)
    return tuple(out)


def palette_list(p):
    return [p[s] for s in SLOTS]
