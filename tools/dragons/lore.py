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

# name: (tier, effect: what the game does, D150 / docs/design/traits.md). Tiers: common, uncommon, rare, legendary.
TRAITS = {
    "Swift": ("common", "flies 6% faster (the valley and Sky Rings)"),
    "Sturdy": ("common", "games, challenges, battles and shows cost a quarter less Energy"),
    "Keen Nose": ("common", "a quarter more chances of a find on the Wanderings"),
    "Tidy": ("common", "Clean fades, and dust settles, 40% slower"),
    "Hearty Eater": ("common", "food fills its Belly a quarter more"),
    "Early Riser": ("common", "7 am to noon: Play and Love fade half as fast"),
    "Night Owl": ("common", "5 pm to 10 pm: Play and Love fade half as fast"),
    "Sure-Footed": ("common", "one extra heart in the Lantern Trial"),
    "Cuddly": ("common", "petting and brushing build bond faster (+1 a stroke)"),
    "Sunbather": ("common", "10 am to 4 pm: Love fades half as fast"),
    "Water-Lover": ("common", "a bath also fills Play (+20) and Love (+10)"),
    "Chatty": ("common", "walks together build bond twice as fast"),
    "Quick Learner": ("uncommon", "a fifth more experience"),
    "Strong Wings": ("uncommon", "wingbeats and bursts cost a quarter less stamina"),
    "Treasure Hunter": ("uncommon", "half again as much Gleam from the Wanderings"),
    "Brave Heart": ("uncommon", "moves lowering its stats fail half the time"),
    "Gentle Giant": ("uncommon", "+6 Poise in shows"),
    "Warm-Blooded": ("uncommon", "Frost moves do a quarter less damage to it"),
    "Cool-Headed": ("uncommon", "Ember moves do a quarter less damage to it"),
    "Deep Sleeper": ("uncommon", "sleep and naps bring Energy back 30% faster"),
    "Showoff": ("uncommon", "+6 Look in shows"),
    "Loyal": ("uncommon", "never upset by your being away"),
    "Skydancer": ("rare", "turns 15% tighter and sinks 20% slower gliding"),
    "Ironhide": ("rare", "takes a tenth less damage in battles"),
    "Lucky": ("rare", "rare Wanderings finds twice as likely"),
    "Songbird": ("rare", "the show's Performance window a fifth wider"),
    "Elemental": ("rare", "breath moves of its own element hit 15% harder"),
    "Glowheart": ("rare", "its mood counts 5 points higher"),
    "Mossback": ("rare", "shows favouring Grove count it as a Grove dragon"),
    "Starborn": ("legendary", "one more point in every stat"),
    "Ancient Blood": ("legendary", "its eggs come out in the rare colouring twice as often"),
    "Phoenix Heart": ("legendary", "a sulk never turns into being upset"),
    "Moonlit": ("legendary", "every battle stat 10% higher, 8 pm to 6 am"),
    "Sunkissed": ("legendary", "every battle stat 10% higher, 9 am to 5 pm"),
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
