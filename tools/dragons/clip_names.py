"""The clips every body plan must have, grown and baby: the game's ClipId list, in order
(src/core/behavior.cpp kClipNames; a PC test checks the two agree). A plan may add a
hatchling version of any clip as "<name>_h" (the baby form uses it when present).

Beta adds flight clips (take-off, hover, landing, banks) and on-foot following; when the game
gains a clip, it goes here and every plan gains it too.
"""

REQUIRED = [
    "idle", "look_around", "scratch", "walk", "trot", "shuffle", "carry", "sit", "sit_loop", "lie_down",
    "lie_loop", "curl_up", "sleep", "wake", "yawn", "nap_flop", "eat", "fav_wiggle", "pet_head", "pet_chin",
    "roll_over", "belly_rub", "shake", "hop", "pounce", "tail_wag", "wing_flutter", "sulk", "sulk_loop",
    "nuzzle", "greet",
    "pick_up", "drop_wait", "leap_catch", "leg_kick", "sniff_refuse", "lift_wing", "sneeze", "pull_away",
    "paw_bat", "tug", "scamper", "gallop", "play_bow", "spar", "stalk", "tail_chase", "fly_flap", "fly_glide",
    "fly_dive",
]

# What each clip is for (so a plan can give it the right character). Loops loop.
PURPOSE = {
    "idle": "standing at rest, breathing (loop)",
    "look_around": "a curious look left and right",
    "scratch": "scratching an itch with a hind foot",
    "walk": "walking (loop, root speed set by the clip)",
    "trot": "a quicker walk (loop)",
    "shuffle": "turning on the spot (loop)",
    "carry": "walking with something in its mouth (loop)",
    "sit": "sitting down", "sit_loop": "sitting (loop)",
    "lie_down": "lying down", "lie_loop": "lying (loop)",
    "curl_up": "curling up to sleep", "sleep": "asleep, slow breathing (loop)", "wake": "waking up",
    "yawn": "a big yawn (jaw)", "nap_flop": "flopping over for a nap",
    "eat": "eating from the floor (loop)", "fav_wiggle": "a happy wiggle at its favourite food",
    "pet_head": "leaning into a head pat (loop)", "pet_chin": "chin up for a scratch (loop)",
    "roll_over": "rolling onto its back", "belly_rub": "on its back, legs paddling (loop)",
    "shake": "shaking off water", "hop": "a little hop on the spot", "pounce": "a playful pounce forward",
    "tail_wag": "wagging, happy (loop)", "wing_flutter": "a quick flutter of the wings",
    "sulk": "turning away to sulk", "sulk_loop": "sulking (loop)",
    "nuzzle": "nuzzling another dragon", "greet": "an excited greeting",
    "pick_up": "picking a toy up in its mouth", "drop_wait": "dropping it and waiting, eager",
    "leap_catch": "leaping to catch a thrown ball", "leg_kick": "the happy leg kick when scratched",
    "sniff_refuse": "sniffing food and turning its nose up", "lift_wing": "lifting a wing to be groomed",
    "sneeze": "a sneeze", "pull_away": "pulling away from a touch it doesn't like",
    "paw_bat": "batting at a toy", "tug": "pulling a rope (loop)",
    "scamper": "a baby's bounding run (loop)", "gallop": "a grown dragon's run (loop)",
    "play_bow": "the play bow before a game", "spar": "play-fighting (loop)", "stalk": "creeping low (loop)",
    "tail_chase": "chasing its own tail (loop)",
    "fly_flap": "flying, wingbeats (loop)", "fly_glide": "flying, gliding (loop)", "fly_dive": "diving (loop)",
}
