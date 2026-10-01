# The mailbox by the den's door (docs/design/story.md, section 7). A letter with a `when` arrives by
# itself once it holds; its `do` happens when it's first read. "--" alone starts a new page.

letter news_living_valley from rowan "News from the valley"
  when flag migrated
  Dear {P}, I had Bram put up a mailbox by your den's door. You're reading it now, so it works!
  Folk will write to you here: invitations, thank-yous, and now and then a little something.
  --
  And there's a new face about. A lanky fellow in a feathery hat, calls himself a surveyor. Maple knows him.
  Oh, and Celestine at the Glade has been asking after you. Loudly. - Old Rowan

letter rowan_hello from rowan "Come say hello"
  when hatched and not met rowan and new keepers_apprentice
  do start keepers_apprentice
  Dear new keeper, word travels fast in a small valley, and word is there's a brand-new dragon at the den by the falls!
  Come up to the Keeper's Lodge and say hello. I'll put the kettle on. Bring your little one: it's dangerous to go alone, you know.
  --
  Old Rowan. P.S. Mind the third step. It wobbles.

letter rowan_hilltop from rowan "A story for the hill"
  when days market_day 2 and new hilltop
  do start hilltop
  Dear {P}, come up to the Nesting Stone when you can. There's a story that goes with that hill, and I'd like to tell it where it happened.
  Light the hilltop lantern and I'll be there. These old legs will manage the climb. Probably. - Rowan

letter celestine_invite from celestine "DARLING"
  when days market_day 3 and not met celestine and new pageant_invite
  do start pageant_invite
  DARLING. I have heard RUMOURS. Rumours of a new keeper, with a dragon of UNDENIABLE potential.
  You will come to Moonpetal Glade at once. Bring your dragon. Bring your best self. Bring a comb.
  --
  Celestine, Host of the Moonpetal Pageant. Twelve years undefeated in matters of taste.

letter fig_tam from fig "Have you met Tam?"
  when days market_day 2 and not flag fig_told_tam and new cove_meet
  do start cove_meet, flag fig_told_tam
  Dear {P}! Have you met Tam? He fishes at Driftwood Cove. He says he once caught a fish the size of a house.
  I've drawn it. Very accurate. Mostly. (The house was quite small.) Go and see him! - Fig, Royal Surveyor

letter maple_flyer from maple "EGG OF THE DAY!"
  when days market_day 4
  do food candy 1
  EGG OF THE DAY! Rare colours! Fresh fruit! Toys for dragons! Fig-approved (he was asleep)!
  Enclosed: one ember candy, on the house. Don't tell Pip. - Maple's Market. Deal!

letter pip_drawing from pip "(a crayon drawing)"
  when days sir_flaps 1
  (A crayon drawing of you and {D}. {D} is very big and very purple. There are lots of hearts.)
  THANK YOU for finding Sir Flaps!!! He says thank you too. He can't write. He is a dragon. - Pip

letter bram_present from bram "From Custard"
  when days meadow 1
  do food drumstick 2
  Custard dug these up and wanted you to have them. I've washed them. Mostly. - Bram (and Custard)

letter sable_moonpetal from sable "A pressed flower"
  when days trailhead 1
  (A pressed moonpetal falls out of the envelope. It still glows, just a little.)
  Some shards fall where no one looks. Look up, now and then. - S.

letter rowan_festival from rowan "Nearly there"
  when done hilltop and done wings and done trailhead and new lantern_festival
  do start lantern_festival
  Dear {P}, the isles, the trail, the hilltop... nearly every lantern in the valley's lit, and it's your doing.
  Light the last of them, then win the Lantern Trial at the arena. The great lantern lights for the winner. - Rowan

letter cinder_note from cinder "(a sooty envelope)"
  when days wings 1
  (Inside: a sooty paw print, and a small scorch mark shaped a bit like a heart.)
  --
  P.S. He insisted. He's been practising all morning. - Rowan

letter fig_sketch from fig "A proper map"
  when days fig_map 2
  Dear {P}, I've drawn up the whole valley properly! Every place, the right way up. I checked twice. Then I napped. Then I checked again.
  Your den has a little heart on it. Official surveyor's mark. Don't tell Maple. - Fig

letter wren_flame from wren "New challengers!"
  when league 1
  NOTICE from the arena: the Flame league's challengers have arrived in the valley! Check the board for where they stand.
  Also, congratulations on the Ember title. That was very exciting. Professionally speaking. - Wren, steward

letter marigold_seeds from marigold "Seeds"
  when days league_ember 1
  do food skyberry 2
  Hello, champion! Sunny and I grew these skyberries in the orchard. Plant a few, eat the rest! See you at the orchard. - Marigold

letter rook_letter from rook "Don't let it go to your head"
  when league 2
  Don't let it go to your head. The sky is watching. The sky is always watching.
  --
  Captain Rook. P.S. Bramble says hi. Bramble does NOT say hi. Bramble is fearsome.

letter wren_blaze from wren "More challengers!"
  when league 2
  NOTICE from the arena: the Blaze league's challengers are in the valley! They are VERY good. Train well. - Wren

letter seraphine_note from seraphine "Posture"
  when league 3
  You fought with grace. Your posture, less so. Shoulders back, keeper. I shall be dancing at the Glade. - Seraphine

letter wren_starfire from wren "The Starfire league!"
  when league 3
  NOTICE: the Starfire league has arrived. The very best in Skyreach Valley. And then Solenne herself. Oh, my whistle. - Wren

letter solenne_beyond from solenne "Beyond the gap"
  when league 4
  Dear {P}, you are one of the five champions of Skyreach Valley now: Marigold, Rook, Seraphine, myself, and you.
  Beyond the gap there are valleys with dragons I've never seen. Someday, go and look. I'll be watching the sky at Starwatch Ruins. - Solenne

letter tove_numbers from tove "Numbers"
  when hollow 15
  Floors: 15. Days since you started: I counted. Snowflakes counted this week: I stopped at eleven thousand. Keep going. - Tove

letter celestine_ember from celestine "MAGNIFICENT"
  when shows 1
  do gleam 50
  MAGNIFICENT. The Ember title! I wept. I am still weeping. Enclosed: fifty Gleam, and a tissue. - C.

letter celestine_flame from celestine "OUTSTANDING"
  when shows 2
  do gleam 80
  The Flame title, darling! The judges swooned. I swooned. Plume fell off her chair. Enclosed: Gleam, for sparkle. - C.

letter celestine_blaze from celestine "SPECTACULAR"
  when shows 3
  do gleam 120
  The Blaze title. SPECTACULAR. Only the Starfire shows remain, darling. The very summit of pageantry. I believe in you. - C.

letter primrose_challenge from primrose "A word of warning"
  when showwon 0 and met primrose
  A word of warning: beginner's luck runs out. Duchess and I will see you at the next show. Do bring a better ribbon. - P. Pembrook

letter primrose_truce from primrose "(lavender paper, perfumed)"
  when shows 4 and flag beat_primrose
  {P}. Star of the Glade. Congratulations. I've drafted this letter nine times and the first eight were rude.
  --
  Duchess has been sulking since the Gala, so I let her sleep on my good cushion. I think she misses losing to you. I think I might too. Tea at the glade, some afternoon? Bring {D}. - Primrose (not P. Pembrook)

letter linnet_stock from linnet "New stock!"
  when done pageant_dazzle
  Darling keeper! New ribbons at Linnet's Finery, soft as a sigh and bright as a secret. Come and see! - Linnet

letter tam_whiskers from tam "Seen him"
  when days cove_meet 2 and new cove_whiskers
  Seen him. Old Whiskers. At dusk, off the jetty. Big as a boat. Well. A small boat. Come by. - Tam

letter rowan_after from rowan "What comes next"
  when days lantern_festival 1
  Dear {P}, what a night! Cinder's still humming. I've never seen him so pleased with himself.
  Mind that star-born egg. Keep it warm; there's no telling what will hatch from a gift like that.
  --
  And I heard Fig's been mumbling about "a surveyor's feeling in the knees". Keep an eye on him. - Rowan
