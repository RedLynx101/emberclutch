"""The dragons' elements, manners and traits (D78), as data (pure Python). Kinds name them in
META; the game's tables are generated from here (DR3 turns the effects into rules; the
numbers are first drafts for Noah to see).

Stats (1..10 base per kind): wing (flight speed, agility), wit (obedience, tricks), might
(strength), breath (its element's breath), stamina (endurance). Battles, much later: stamina
the health, might and breath the attacks, wing the speed, wit the reliability.
"""

ELEMENTS = [  # one-word types; (name, heartglow colour, what it's like)
    ("Ember", (1.0, 0.55, 0.16), "fire and warmth"),
    ("Grove", (0.60, 0.95, 0.30), "leaves, moss and growing things"),
    ("Stone", (1.0, 0.72, 0.30), "rock, crystal and the deep earth"),
    ("Gale", (0.62, 0.92, 1.00), "wind and open sky"),
    ("Tide", (0.35, 0.95, 0.85), "water, rain and rivers"),
    ("Frost", (0.75, 0.80, 1.00), "snow, ice and clear cold nights"),
    ("Lumen", (1.00, 0.88, 0.45), "sunlight and warm light"),
    ("Shade", (0.70, 0.55, 1.00), "night, starlight and quiet"),
]
ELEMENT_NAMES = [e[0] for e in ELEMENTS]

STATS = ("wing", "wit", "might", "breath", "stamina")

MANNERS = {  # name: (stat nudged up, stat nudged down); the six old personalities first
    "Brave": ("might", "wit"),
    "Shy": ("wit", "might"),
    "Playful": ("wing", "stamina"),
    "Proud": ("breath", "wit"),
    "Sleepy": ("stamina", "wing"),
    "Curious": ("wit", "stamina"),
    "Gentle": ("stamina", "might"),
    "Mischievous": ("wing", "wit"),
    "Greedy": ("might", "breath"),
    "Stubborn": ("stamina", "wit"),
}

# name: (tier, effect). Tiers: common, uncommon, rare, legendary.
TRAITS = {
    "Swift": ("common", "flies a little faster"),
    "Sturdy": ("common", "tires more slowly"),
    "Keen Nose": ("common", "sniffs out finds more often"),
    "Tidy": ("common", "stays clean longer"),
    "Hearty Eater": ("common", "gets more from every meal"),
    "Early Riser": ("common", "livelier in the morning"),
    "Night Owl": ("common", "livelier at night"),
    "Sure-Footed": ("common", "lands softly and never trips"),
    "Cuddly": ("common", "bond grows faster from petting"),
    "Sunbather": ("common", "cheered up by sunshine"),
    "Water-Lover": ("common", "loves baths and splashing"),
    "Chatty": ("common", "calls and trills to you more"),
    "Quick Learner": ("uncommon", "learns tricks faster"),
    "Strong Wings": ("uncommon", "stamina lasts longer in the air"),
    "Treasure Hunter": ("uncommon", "better finds on the Wanderings"),
    "Brave Heart": ("uncommon", "rarely startled"),
    "Gentle Giant": ("uncommon", "other dragons calm down near it"),
    "Warm-Blooded": ("uncommon", "shrugs off the cold"),
    "Cool-Headed": ("uncommon", "shrugs off the heat"),
    "Deep Sleeper": ("uncommon", "rests and recovers faster"),
    "Showoff": ("uncommon", "shines in shows and trials"),
    "Loyal": ("uncommon", "its bond fades very slowly"),
    "Skydancer": ("rare", "turns tighter and glides further"),
    "Ironhide": ("rare", "tougher than it looks"),
    "Lucky": ("rare", "rare finds come more often"),
    "Songbird": ("rare", "its calls soothe the den"),
    "Elemental": ("rare", "a stronger breath of its element"),
    "Glowheart": ("rare", "a brighter heartglow; its mood stays steadier"),
    "Mossback": ("rare", "little plants grow on it; loved by Grove dragons"),
    "Starborn": ("legendary", "a little better at everything"),
    "Ancient Blood": ("legendary", "its eggs hatch rare more often"),
    "Phoenix Heart": ("legendary", "never sulks for long"),
    "Moonlit": ("legendary", "at its best under the stars"),
    "Sunkissed": ("legendary", "at its best in full sun"),
}
TIERS = ("common", "uncommon", "rare", "legendary")


def check_meta(meta):
    """A kind's META names only known elements, manners and traits; its stats are 1..10."""
    els = meta["element"] if isinstance(meta["element"], (tuple, list)) else (meta["element"],)
    for e in els:
        assert e in ELEMENT_NAMES, f"{meta['name']}: unknown element {e}"
    for m in meta["manners"]:
        assert m in MANNERS, f"{meta['name']}: unknown manner {m}"
    for t in meta["traits"]:
        assert t in TRAITS, f"{meta['name']}: unknown trait {t}"
    assert set(meta["stats"]) == set(STATS), f"{meta['name']}: stats must be {STATS}"
    assert all(1 <= v <= 10 for v in meta["stats"].values()), f"{meta['name']}: stats are 1..10"
