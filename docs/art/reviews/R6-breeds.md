# Review R6 — every breed, every look (not blocking)

**For Noah, 2026-09-24.** The dragons update (WP12) is built. Nothing waits on this review:
look through it whenever it suits you, and anything you'd change goes into the next pass.
Every picture is from the game in Azahar, by scripted runs (`tests/autotest/`).

## The 21 breeds
The six purebreds, then the fifteen hybrids, each a grown male in the same look (the
starter's; here Tallneck), whole in the brush's view (dev menu: Next breed).
`tests/autotest/breeds.txt`; the faces up close are `breeds_face.txt`.

![Breeds 1](R6-breeds-01.jpg)
![Breeds 2](R6-breeds-02.jpg)
![Breeds 3](R6-breeds-03.jpg)
![Breeds 4](R6-breeds-04.jpg)
![Breeds 5](R6-breeds-05.jpg)
![Breeds 6](R6-breeds-06.jpg)

## The four looks (D54)
Classic, Pebbleback and Tallneck about equally likely, the wild look rarer (8%; about a
quarter with a wild parent). Each has its own name in the hatching toast and the profile:
"It's a Pebbleback Tide!", "It's a Cinderveined Ember!" `looks.txt`.

![Looks 1](R6-looks-01.jpg)
![Looks 2](R6-looks-02.jpg)
![Looks 3](R6-looks-03.jpg)

## The rare traits
Iridescent, melanistic, leucistic and starspeckle (glints that come and go). `rares.txt`.

![Rares 1](R6-rares-01.jpg)
![Rares 2](R6-rares-02.jpg)

## The Dragondex (D55, D66)
From the system menu (START). Seven breeds a page, their four looks across; the top screen
shows the one picked, turning, once you've met it. A new entry, a rare trait or a completed
breed is told after the naming. A completed breed pays 150 Gleam and gives its banner
(the breed's colours, an egg with its heartglow), hung in the den's banner spot at once the
first time; Hang banner / Take it down in the book. `dex.txt`.

![Dragondex 1](R6-dex-01.jpg)
![Dragondex 2](R6-dex-02.jpg)
![Dragondex 3](R6-dex-03.jpg)

## Photo mode (D66)
The camera under the heartglow. The den holds still, the names and hints hide; **A** or the
big button snaps the top screen in a gold frame with the name and the date, saved as
`sdmc:/3ds/emberclutch/photos/photo_NNNN.bmp`. **X** switches between the whole den and a
close framing of the one you care for; the D-pad picks whose name goes on it. `photo.txt`.

![Three photos: the den, close up, another dragon](R6-photos.jpg)

## Dust and mud (D46)
Dust dulls a region evenly over a day or two; mud comes home from the Wanderings in spots on
the legs, belly and tail. Brushing lifts mud at half the dust's rate, the bath at once, and
it flakes off by itself over a few days. Here the dev menu's Dust/mud/bath: clean, dusty,
muddy all over, bathed. `mud.txt`.

![Clean, dusty, muddy](R6-mud-01.jpg)

## Budgets with mixed looks
Three grown dragons in three looks in the den (`perfmix.txt`, dev menu: Mix looks): 6,814
triangles on the top screen and 2,954 on the bottom (of 8,000 and 3,500), 13 draws, 17.8 MB
of linear memory free with every look loaded. Frame times come from the hardware run (WP13).
