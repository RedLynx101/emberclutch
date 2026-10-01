# Act 1: The Lantern Festival (docs/design/story.md, section 4). The eight quests of Beta, rebuilt:
# each starts with someone asking and ends by going back to them; every step checks the world, so a
# lantern lit early or a dragon already grown moves things straight on.

# ------------------------------------------------------------------------------------------ quests

quest keepers_apprentice "The keeper's apprentice"
  line main
  giver rowan
  gleam 60
  when hatched
  rumour "Old Rowan would like to meet the new keeper. He lives at the Keeper's Lodge by the falls."
  step "Visit Old Rowan at the Keeper's Lodge" talk rowan where person rowan
  step "Light the lantern by your den" until lantern den where lantern den
  step "Tell Rowan the den's lantern is lit" talk rowan where person rowan

quest market_day "Market day"
  line main
  giver maple
  gleam 80
  after keepers_apprentice
  rumour "Rowan says Maple at the Market will set you up."
  step "Find the Market village and meet Maple" talk maple where person maple
  step "Find Fig. Maple thinks he's asleep in the Whisperwood" talk fig where area market -82 70 26
  step "Win a Fruit Catch at the orchard (Fig will cheer)" until cup fruit where cup fruit
  step "Light the Market's lantern with Fig" until lantern market where lantern market
  step "Tell Maple you found Fig" talk maple where person maple

quest hilltop "The hilltop"
  line main
  giver rowan
  gleam 80
  after market_day
  rumour "Old Rowan has a story he wants to tell up on the Nesting Stone's hill."
  step "Climb to the Nesting Stone" until place stone where place stone
  step "Light the hilltop lantern" until lantern stone where lantern stone
  step "Hear Rowan's story by the Stone" talk rowan where person rowan

quest meadow "The meadow"
  line main
  giver maple
  gleam 80
  after market_day
  rumour "Maple has a delivery for someone at the Sanctuary."
  step "Take Maple's feed basket to Bram at the Sanctuary" talk bram where person bram
  step "Walk {D} through the meadow's tall flowers to find the stray" until world found_stray where area sanctuary -46 58 40
  step "Light the meadow's lantern" until lantern sanctuary where lantern sanctuary
  step "Tell Bram" talk bram where person bram

quest cold_heights "The cold heights"
  line main
  giver rowan
  gleam 100
  after market_day
  rumour "Old Rowan remembers gliding off the heights by the Cold Vault."
  step "Climb the path to the Cold Vault" until place vault where place vault
  step "Ride off the heights by the Vault and land 25 m or more below" until world glided where area vault 0 0 60
  step "Light the Vault's lantern" until lantern vault where lantern vault
  step "Tell Rowan about the glide" talk rowan where person rowan

quest wings "Wings"
  line main
  giver rowan
  gleam 150
  after cold_heights
  rumour "Old Rowan wants to see you and {D} in the sky."
  step "Grow up together: a grown dragon to ride" until grown
  step "Fly the Sky Rings at the arena" until cup rings where cup rings
  step "Light the high lantern on the floating isles" until lantern isles where lantern isles
  step "Tell Rowan" talk rowan where person rowan

quest trailhead "The trailhead"
  line main
  giver sable
  gleam 100
  after meadow
  rumour "Bram says a traveller is camped at the Wanderers' Trailhead."
  step "Meet the traveller at the Wanderers' Trailhead" talk sable where person sable
  step "Send a juvenile or older dragon on a Wandering" until world wandered where place trailhead
  step "Light the traveller's lantern with the star shard" until lantern trailhead where lantern trailhead
  step "Tell Sable" talk sable where person sable

quest lantern_festival "The Lantern Festival"
  line main
  giver rowan
  gleam 300
  after hilltop wings trailhead
  rumour "Every lantern but a few is lit. Old Rowan will know what comes next."
  step "Light every lantern in the valley" until lanterns where unlit
  step "Win the Lantern Trial at the arena" until cup trial where cup trial
  step "The festival night: meet everyone at the arena after dark" talk rowan where person rowan

quest two_by_two "Two by two"
  line errand
  giver rowan
  gleam 120
  after hilltop
  rumour "Old Rowan says the Nesting Stone still works its magic for a pair of grown dragons."
  step "Raise two grown dragons" until adults 2
  step "Settle a pair on the Nesting Stone" until flag nest_settled where place stone
  step "Keep the new egg warm until it hatches" until flag nest_hatched where place den
  step "Tell Rowan about the new little one" talk rowan where person rowan
  reward wear nest_blanket

quest fig_map "Fig's flyaway map"
  line errand
  giver fig
  gleam 60
  after market_day
  rumour "Fig was flapping about something. Something about a gust."
  step "Find Fig's five map pages" until var pages_found 5 where group page
  step "Bring the pages back to Fig" talk fig where person fig
  reward wear surveyor_cap

quest sir_flaps "Sir Flaps is missing"
  line errand
  giver pip
  gleam 40
  after market_day
  rumour "Pip was crying by the Market. Something about Sir Flaps."
  step "Follow the paw prints to the fox's den (it's home at dusk)" until flag flaps_found where area market -72 38 18
  step "Bring Sir Flaps back to Pip" talk pip where person pip

quest tea "Tea for Rowan"
  line errand
  giver rowan
  gleam 40
  after market_day
  when days market_day 1
  rumour "Old Rowan is out of honeyroot tea, and grumbling about it."
  step "Take honey from the orchard's hives" until flag honey_taken where spot orchard 14 -6
  step "Share a cup with Rowan" talk rowan where person rowan
  reward food honeyroot 3

# ------------------------------------------------------------------------------------------ Rowan

talk rowan
  rule first if step lantern_festival 3 and hour 18 6
    do finish lantern_festival, world festival, staregg, flag act1_done
    * The great lantern blazes over the arena. The whole valley has come to see it.
    [crying] Look at it, {P}. Every lantern in Skyreach Valley, lit by you and {D}.
    @cinder [happy] Wrrrr.
    @fig [shock] Is that... over the lake! Look! LOOK!
    * A dragon made of starlight comes over the water, circling the great lantern once, twice.
    * Cinder rises on his old legs, spreads his wings wide, and roars. The star dragon answers.
    @pip [excited] It's the STAR DRAGON! I KNEW it! I knew it was real!
    * It dips into the light, and when it rises again, something glows at the lantern's foot. An egg.
    @sable [wistful] A star-born egg. For the keeper who lit the way.
    [crying] Long ago we lit the way home for a lost dragon. Tonight you did it again.
    [happy] You're the valley's keeper now, {P}. Happy Lantern Festival.
  rule first if step lantern_festival 3
    [happy] Tonight's the night! Come to the arena after dark, {P}. Everyone will be there.
    [wistful] Even this old snorer. Cinder hasn't missed a festival in forty years.
  rule once if not met rowan and lantern den
    do start keepers_apprentice, finish keepers_apprentice, world met_keeper, start market_day
    [happy] Oh! There you are! A new keeper, and a young dragon with you. Welcome to Skyreach Valley, {P}.
    ? read rowan_hello | [happy] You got my letter! Mind that third step. It wobbles.
    [surprised] And your den's lantern is glowing already? I saw it from the porch! Well, I never.
    * An old grey dragon snores by the door. A smoke ring drifts up and pops.
    [wistful] That's Cinder. Forty years he lit every lantern in the valley before the Lantern Festival.
    [sad] This year he can barely light his own sneezes. And emberglass only holds a flame for a year.
    [proud] But you two have made a start already. Here, a Keeper's Journal: X shows it, out in the valley.
    [happy] Maple at the Market will set you up. And ask her where that surveyor friend of hers has got to!
  rule once if not met rowan
    do start keepers_apprentice, advance keepers_apprentice, world met_keeper
    [happy] Oh! There you are! A new keeper, and a young dragon with you. Welcome to Skyreach Valley, {P}.
    ? read rowan_hello | [happy] You got my letter! Mind that third step. It wobbles.
    [happy] I'm Rowan. I've kept the dragons here since the falls were a trickle.
    ? grown | [surprised] My, look at the size of {D}! Not so little after all.
    * An old grey dragon snores by the door. A smoke ring drifts up and pops.
    [wistful] That's Cinder. Forty years he lit every lantern in the valley before the Lantern Festival.
    [sad] This year he can barely light his own sneezes. And emberglass only holds a flame for a year.
    [worried] So the lanterns have all gone dark, and the festival's almost here.
    [happy] But {D} has a warm little breath. I can tell. Start with the lantern by your den, then come tell me.
  rule chat if step keepers_apprentice 2
    [happy] The lantern by your den first. Stand by it and press A, and {D} will breathe on it.
    [laugh] Cinder used to sneeze on it. Worked just as well.
  rule if step keepers_apprentice 3
    do finish keepers_apprentice, start market_day
    [proud] Ha! I saw the glow from my porch. Cinder thumped his tail twice. That's high praise, from him.
    [happy] Here, a Keeper's Journal. X shows it, out in the valley. It keeps your quests, and where to go.
    [thinking] Now then. Maple at the Market will set you up with food and such. She's a whirlwind.
    [laugh] And ask her where that surveyor friend of hers has got to. Nobody's seen him since yesterday!
  rule chat if active market_day
    [thinking] Maple's at the Market, down the path past the bridge. You'll hear her before you see her.
  rule if step hilltop 3
    do finish hilltop, world heard_story, start two_by_two
    [happy] You made it up! And the hilltop lantern's lit. Sit a moment. Here's the story I promised.
    [thinking] Long ago, before the Lodge, before the Market, a dragon fell out of the night sky, right here.
    [sad] It was lost, and so tired it couldn't lift its wings.
    [happy] So the valley lit every lantern it had, and on the last night it rose and followed the lights home.
    [wistful] We've lit the way every year since. Some say it still comes back to look.
    [thinking] The Stone stays warm at night, you know. Pairs settle here. Eggs laid near it are lucky.
    [happy] When you've two grown dragons who get along, bring them up here. The Stone does the rest.
  rule once if new hilltop and done market_day and not read rowan_hilltop
    do start hilltop
    [thinking] You know, there's a story that goes with the Nesting Stone. I'd like to tell it where it happened.
    [happy] Light the hilltop lantern, and meet me up there. These old legs will manage.
  rule chat if active hilltop
    [happy] I'll meet you by the Nesting Stone once its lantern's lit. Up the hill past the Market.
  rule once if done market_day and new cold_heights
    do start cold_heights
    [wistful] When I was your age, Cinder and I would climb up by the Cold Vault and glide all the way down.
    [happy] Best feeling in the world. Try it, if {D}'s brave enough.
    ? not grown | [thinking] You'll need {D} big enough to ride first. It won't be long, the way you look after it.
    ? grown | [excited] And look at {D}! Big enough to carry you. Off you go!
  rule if step cold_heights 4
    do finish cold_heights
    [excited] You glided off the heights? All the way down? Ha! Just like Cinder and me.
    * By the door, Cinder opens one eye, snorts a proud little puff of smoke, and goes back to sleep.
    ? met tove | [thinking] And you met Tove! She keeps the count up at Frostspire Hollow. Good woman. Terrifying.
    [proud] You two are ready for the sky, I'd say.
  rule once if done cold_heights and new wings
    do start wings
    ? not grown | [happy] When {D} is grown, I want to see you two in the sky. Properly.
    ? grown | [proud] Look at the size of {D}! Ready for the sky, I'd say.
    [thinking] Wren runs the Sky Rings at the arena. Then the floating isles: theirs is the highest lantern of all.
  rule chat if active wings and not grown
    [happy] Keep caring for {D}. Good food, a bath, a play, a cuddle. It all adds up, one day at a time.
  rule if step wings 4
    do finish wings, flag cinder_woke
    [happy] The isles' lantern! I saw it from here, like a new star.
    * Behind Rowan, Cinder heaves himself up, stretches his old wings wide, and lets out a rumbling roar.
    [crying] Did you see that? He hasn't done that in years. He knows what you've done, {P}.
  rule once if done hilltop and done wings and done trailhead and new lantern_festival
    do start lantern_festival
    [excited] The isles, the trail, the hilltop... nearly every lantern's lit, {P}!
    [happy] Light the last of them, then win the Lantern Trial at the arena. The great lantern lights for the winner.
  rule chat if active lantern_festival
    ? not lanterns | [thinking] A few lanterns are still dark. Your Journal will point you to the nearest.
    ? lanterns | [excited] Every lantern's lit! Now the Lantern Trial. Wren will set it up at the arena.
  rule if step two_by_two 4
    do finish two_by_two
    [crying] A new egg on the hill, and now a new little dragon. The old Stone's still got it.
    [happy] Here. This was the blanket Cinder slept on when he was small. It's yours now, for your dragons.
  rule once if active tea and not flag honey_taken
    [huff] Honeyroot tea. That's all I ask of life. And I'm OUT.
  rule if step tea 2
    do finish tea
    * Rowan pours two cups. The tea smells of honey and woodsmoke.
    [wistful] Cinder was born the same week I took over the Lodge. Tiny thing. He sneezed on the kettle.
    [laugh] Forty years of breakfasts, and he's never once let the tea go cold.
    [happy] Take some honeyroot with you. {D} will like it.
  rule once if new tea and done market_day and days market_day 1
    do start tea
    [huff] Honeyroot tea. That's all I ask of life. And I'm OUT.
    [thinking] The orchard's hives would have honey. The bees are grumpy, mind. Grumpier than me.
    [happy] A dragon's warm breath settles them. Try it with {D}.
  rule chat if done lantern_festival
    vary
      [happy] You're the valley's keeper now, {P}. I'll sit on my porch and watch you two fly.
      --
      [wistful] Some nights I see a light over the lake. The star dragon, looking in on us.
      --
      [laugh] Cinder's been ever so perky since the festival. He ate two breakfasts.
  rule
    vary
      [happy] Every lantern you light, the valley feels a little more awake.
      --
      [thinking] The Nesting Stone, the meadow, the cold heights... they're all waiting for you two.
      --
      [laugh] Mind the third step on your way out. It wobbles. It's wobbled for thirty years.
      --
      [wistful] Cinder and I used to fly the whole valley before breakfast. Now we nap before breakfast.

# ------------------------------------------------------------------------------------------ Maple

talk maple
  rule first if step lantern_festival 3 and hour 18 6
    [excited] Isn't it glorious? I've sold forty lantern-cakes tonight. Forty! Deal!
  rule if step market_day 1
    do advance market_day, world met_market
    [happy] Welcome, welcome! Food, toys, treasures, and an egg of the day! Whatever you need, Maple's got it.
    [thinking] Rowan sent you? Then you're the new keeper! And this must be {D}. Aren't you a love.
    [worried] Say, you haven't seen a lanky fellow in a big feathery hat? That's Fig. He's mapping the valley.
    [worried] Or he's meant to be. Nobody's seen him since yesterday.
    [huff] Knowing Fig, he's asleep under a tree. Try the Whisperwood, in the trees past the Market.
  rule chat if step market_day 2
    [worried] Still no Fig? Try the Whisperwood, in the trees past the Market. Follow the snoring.
  rule chat if step market_day 3
    @fig [excited] Fruit Catch! The orchard! This way! No, THIS way.
    [laugh] He's never been to the orchard in his life. It's east of here, {P}.
  rule chat if step market_day 4
    [happy] Would you light my lantern, dear? It's by the well. Fig's waiting there, I think. Or napping.
  rule if step market_day 5
    do finish market_day, flag fig_friend
    [laugh] Asleep in the woods? Of COURSE he was. Oh, Fig.
    @fig [huff] Surveying. I was surveying.
    [happy] And you won a Fruit Catch, and my lantern's glowing! Thank you, {P}.
    @fig [proud] I've put it on my map! Right... here.
    [thinking] That's the bottom of the lake, Fig. Your map's upside down.
    @fig [cool] It's upside down for SAFETY.
    [happy] Fig knows every corner of this valley, give or take a corner. He'll mark places on your map.
    @fig [excited] And I'll find you! I'm always about somewhere. Usually awake.
  rule once if not met maple
    do world met_market
    [happy] Welcome, welcome! Food, toys, treasures, and an egg of the day! Whatever you need, Maple's got it.
    [thinking] Oh, you're the new keeper! Have you been to see Old Rowan yet? He lives up by the falls.
  rule once if done market_day and new meadow
    do start meadow
    [worried] {P}, could you do me a favour? Bram's feed order came in and he hasn't collected it.
    [huff] That man forgets everything but his animals.
    ? grown | [surprised] Goodness, {D}'s grown! It could carry the basket on its back.
    * Maple hands you a heavy basket of feed. It smells of oats and apples.
    [happy] The Sanctuary's the meadow past the lake. Deal!
  rule chat if step meadow 1
    [thinking] Bram's at the Sanctuary meadow, past the lake. Big fellow, quiet. You can't miss him.
  rule chat if done meadow and step trailhead 1
    [thinking] A traveller at the trailhead? Bram told me. Mysterious sort. Buys nothing. Suspicious!
  rule chat if done lantern_festival
    vary
      [excited] The Keeper of Skyreach Valley, in MY shop! I'll put up a sign.
      --
      [happy] Egg of the day's a good one today. Just saying. Deal?
  rule
    vary
      [happy] Fresh fruit today! And the egg of the day, if you've an empty nest. Deal!
      --
      [thinking] Have you seen Fig? Silly question. He's asleep somewhere.
      --
      [happy] The goods stall has new things every day, you know. Come by tomorrow!
      --
      [huff] Pip's been at the honey cakes again. I can tell. There's honey on the ceiling.

# ------------------------------------------------------------------------------------------ Fig

talk fig
  rule first if step lantern_festival 3 and hour 18 6
    [excited] I've drawn the great lantern on my map! It's the right way up this time! Probably!
  rule if step market_day 2
    do advance market_day, met fig
    * Fig is fast asleep against a tree, his map spread over his face. It flutters with each snore.
    ? not grown | * {D} gives him a long sniff, then a lick right across the ear.
    ? grown | * {D} leans down and snorts a warm gust of breath right into his hat.
    ? not grown | [shock] AAH! A DRAGON! ...Oh. A small dragon. Hello, small dragon.
    ? grown | [shock] AAH! A DRAGON! A BIG ONE! ...Oh. A friendly big one. Hello, friendly big one.
    [cool] I wasn't asleep, you understand. I was surveying the inside of my eyelids. Very detailed work.
    [proud] Fig Thimblewhistle, Royal Surveyor of Skyreach Valley, at your service!
    [thinking] There's no king, before you ask. I checked. But the title was going spare.
    [worried] Maple sent you? Is she cross? She's cross. Oh, she's going to be SO cross.
    [excited] Quick, let's bring her something nice. Fruit! Everybody loves fruit. The orchard's east of the Market!
  rule chat if step market_day 3
    [excited] Go, {D}! Catch the round ones! ...They're all round. Catch all of them!
  rule chat if step market_day 4
    [happy] The Market's lantern! Go on, {D}, give it your warmest. I'll hold the map.
  rule if step market_day 5
    [worried] Maple's waiting. I can feel her waiting. Go on, you first.
  rule once if done market_day and new fig_map and days market_day 1
    do start fig_map
    [shock] My MAP! A gust took it! Every page! My life's work!
    [thinking] Well. My week's work. My Tuesday's work.
    [worried] There were five pages. The Mill, the Orchard, Mirror Lake, the bridge, and... somewhere near the Lodge.
    [excited] If you find them, bring them back! I'll make it worth your while. I have a spare hat!
  rule chat if step fig_map 1
    ? not var pages_found 1 | [worried] Five pages. The Mill, the Orchard, Mirror Lake, the bridge, the Lodge. I'd look, but. Naps.
    ? var pages_found 1 | [excited] You've found some! Keep going! The Journal will point you to the nearest.
  rule if step fig_map 2
    do finish fig_map
    [excited] My PAGES! All five! Look, the ducks! "Here be ducks. Possibly dangerous." Classic Fig.
    [proud] I'll draw it all up properly. Every place you've found, right way up. Mostly.
    [happy] And a promise is a promise: my spare hat, for {D}. The surveyor's cap! Very official.
  rule once if done market_day and not flag fig_told_tam and new cove_meet
    do flag fig_told_tam, start cove_meet
    [excited] Have you met Tam? He fishes at Driftwood Cove. He says he once caught a fish the size of a house.
    [thinking] The house was quite small. But still!
  rule chat if done lantern_festival
    vary
      [wistful] The star dragon... I drew it the second I saw it. My hand was shaking. It looks like a potato.
      --
      [thinking] Still no egg for me. But I have a feeling. A surveyor's feeling. It's in the knees.
      --
      [happy] I've mapped every lantern in the valley. Now what? Mapping the clouds, I suppose.
  rule
    vary
      [sleepy] Mmf. Wha... surveying. I was surveying. The ground. Up close.
      --
      [excited] Did you know the Mill's windmill turns backwards on Tuesdays? I made that up. Or did I.
      --
      [proud] I've named that hill "Fig's Hill". It's not official. Nothing I do is official. That's the beauty.
      --
      [thinking] I'm still waiting for an egg to choose me. Any day now. Any year now.
      --
      [happy] Hello, {D}! Hello, {P}! I've marked your den on my map with a little heart. Don't tell Maple.

# ------------------------------------------------------------------------------------------ Bram and Custard

talk bram
  rule first if step lantern_festival 3 and hour 18 6
    [happy] Custard doesn't like the fireworks. So I brought him a blanket. And a biscuit. Several biscuits.
  rule if step meadow 1
    do advance meadow, world met_sanctuary
    [shy] Oh. Hello. ...Is that for me? The feed. Thank you. Maple sent it? Thank you. Tell her thank you.
    [worried] Sorry. I'm... one of the little ones wandered into the tall flowers this morning. And Custard went after her.
    [worried] Custard's my dog. He's not a good finder. He's a good dog, though.
    [thinking] {D}'s nose might find them. Walk through the flowers with it, on foot or on its back. Along the ground.
  rule chat if step meadow 2
    [worried] The tall flowers, over that way. Keep {D} close to the ground. It'll catch their scent.
  rule chat if step meadow 3
    [happy] They're safe. Both of them. In the hay, fast asleep. Could you light the meadow's lantern? For them.
  rule if step meadow 4
    do finish meadow
    [crying] You found them both. Thank you, {P}. Thank you, {D}.
    @custard [happy] Wuff!
    [happy] Custard says thank you too. He'll be about the meadow now. You can pet him. He likes it behind the ears.
    [shy] And any dragon of yours can rest here, if the den's too full. We've room. Lots of room.
  rule once if done meadow and new trailhead
    do start trailhead
    [thinking] Custard keeps barking at the trailhead. There's a traveller camped up there.
    [thinking] He asked me about... shiny stones. Star shards, he said. He seemed nice. Quiet. Like me.
  rule once if not met bram
    [shy] Oh. Hello. I'm Bram. I look after the dragons who rest here. ...That's all. Hello.
  rule chat if done lantern_festival
    vary
      [happy] The little one's growing fast. She follows Custard everywhere now.
      --
      [shy] You saved the valley. I saved a hedgehog today. Not the same. But nice.
  rule
    vary
      [shy] ...Hello. Custard, say hello. He's saying hello.
      --
      [happy] The resting dragons like the sun on that side of the meadow. Warm stones.
      --
      [thinking] I talk to the animals more than people. They don't mind the pauses.

talk custard
  rule
    vary
      [happy] Wuff! Wuff wuff!
      * Custard rolls over for a belly rub, then chases {D}'s tail round and round. He never catches it.
      --
      [happy] Wuff!
      * Custard brings you a stick. It's mostly mud. He is very proud of it.
      --
      [sleepy] Wuff...
      * Custard yawns, thumps his tail twice, and flops down in the sun.

talk cinder
  rule chat if done lantern_festival
    * Cinder opens one bright eye and rumbles, warm as a hearth. He looks young again, somehow.
    [happy] Wrrrr.
  rule
    * Cinder is fast asleep. A smoke ring drifts up from his nose and pops.
    [sleepy] Zzzz... wrrr...

# ------------------------------------------------------------------------------------------ Pip

talk pip
  rule first if step lantern_festival 3 and hour 18 6
    [excited] Is it coming? The star dragon? Is it coming NOW? How about now? NOW?
  rule once if done market_day and new sir_flaps and days market_day 1
    do start sir_flaps
    [crying] Sir Flaps is GONE! He was right here and now he's NOT!
    [crying] I only put him down for ONE second to eat a honey cake. Maybe two cakes.
    [worried] There were fluffy paw prints! Going into the trees! Something TOOK him!
    * The prints are a fox's. Foxes only come out at dusk.
  rule chat if step sir_flaps 1
    [worried] Did you find him? The paw prints went into the trees west of the Market. Please find him!
  rule if step sir_flaps 2
    do finish sir_flaps
    [shock] SIR FLAPS! You found him!
    * Pip hugs the chewed plush dragon so hard its stuffing squeaks.
    [excited] He's a bit chewed. That's okay. It makes him look BRAVE. Thank you thank you THANK YOU!
    * Pip does a very wobbly happy dance.
  rule if grown and not flag pip_rode_talk
    do flag pip_rode_talk
    [excited] Is {D} BIG enough to RIDE now? You RODE it?! When I grow up I'm going to fly to the isles!
  rule chat if past wings 2 and not done lantern_festival
    [excited] I saw it! I saw the star dragon over the floating isles! It was all sparkly!
  rule once if not met pip
    [excited] Is that your dragon? Can I pet it? Can I? CAN I? Hi, {D}! I'm Pip! This is Sir Flaps!
  rule chat if done lantern_festival
    vary
      [excited] I SAW IT. I saw the star dragon. Everyone else saw it too. But I saw it FIRST.
      --
      [happy] Sir Flaps and I are practising being keepers. He's very good at sitting still.
  rule
    vary
      [excited] Did you know the floating isles have a lantern on top? Only dragons can get up there!
      --
      [happy] When I grow up I'm going to have a dragon THIS big. And a hat like Fig's. But awake.
      --
      [thinking] Why do dragons like baths? I don't like baths. Is it the bubbles? I like bubbles.

# ------------------------------------------------------------------------------------------ Sable

talk sable
  rule first if step lantern_festival 3 and hour 18 6
    [wistful] The shards have been warm all evening. It's close. Watch the lake.
  rule if step trailhead 1
    do advance trailhead, world met_traveller
    [calm] Evening. I'm Sable. I walk the long trails and see what the valleys are hiding.
    [thinking] Bram sent you? He's kind. Quiet. The quiet ones notice things.
    [wistful] Out past the gap there's a meadow where star shards fall. They're warm, like a heartbeat.
    [thinking] Send a dragon on a Wandering from here. Dragons have a nose for shards. Read the sign for how.
    ? not juvenile | [calm] A young one can't go yet. Once it's a juvenile, it's ready for the trail.
  rule chat if step trailhead 2
    [thinking] The trail starts right here. Take a Wandering and see what comes back with you.
  rule chat if step trailhead 3
    [wistful] A star shard. Hold it to the lantern here, and it'll light itself. Go on.
  rule if step trailhead 4
    do finish trailhead
    [wistful] The trail's lantern, burning again. The star dragon is near. I can feel it in the shards.
    [thinking] I've walked valleys beyond the gap you wouldn't believe. Skies of a different blue.
    [calm] But this one... this one's special. Keep the lanterns lit, keeper.
  rule chat if done lantern_festival
    vary
      [wistful] One day I'll walk on, past the gap. There are other valleys, other keepers. Maybe you'll follow.
      --
      [thinking] What does a star remember, I wonder? Where it's been, or where it's going?
  rule
    vary
      [thinking] What does a star remember? The sky, or the ground?
      --
      [calm] The trails are long. Good for thinking. Bad for boots.
      --
      [wistful] Every valley has its own light. This one's is warm.

# ------------------------------------------------------------------------------------------ Wren

talk wren
  rule first if step lantern_festival 3 and hour 18 6
    [proud] Everything to schedule! The lanterns, the crowd, the... is that a STAR DRAGON? That's not on the schedule!
  rule once if not met wren
    do world met_steward
    [proud] A new challenger! I'm Wren, the arena's steward. Rules, schedules, whistles. Mostly whistles.
    [happy] Sky Rings, the Lantern Trial, and Fruit Catch at the orchard: four cups each, Ember to Starfire.
    [excited] And the battle league's board is right here. Ribbons, trophies, Gleam... and bragging rights!
  rule chat if active lantern_festival and past lantern_festival 1
    [excited] The Lantern Trial! Every lantern in the valley's lit, so tonight's the night. Breathe true!
  rule chat if done lantern_festival
    vary
      [proud] The festival went EXACTLY to schedule. Apart from the star dragon. I've added it for next year.
      --
      [happy] Ready when you are. Pick a challenge on the board.
  rule
    vary
      [happy] Ready when you are. Pick a challenge on the board.
      --
      [proud] Rules are rules! Whistle, then go. Not go, then whistle. Some people.
      --
      [thinking] Have you seen the champions up close? I mean, I haven't. I'm very busy. Professionally.

# ------------------------------------------------------------------------------------------ where they stand

# Fig: asleep in the Whisperwood, then with you through Market day, then somewhere new each day.
spot fig market -82 70 facing -2.28 clip doze when step market_day 2
spot fig orchard -2 9 facing 3.0 clip clap when step market_day 3
spot fig market 6 1 facing 0.4 clip wave when step market_day 4
spot fig market 5 -2.5 facing -0.6 when step market_day 5
spot fig arena -6.5 10.5 facing -2.9 clip cheer when step lantern_festival 3 and hour 18 6
spots fig daily market 7 7 facing 1.8 clip doze_stand | mill 10.5 2.5 facing 2.4 | orchard 6 4 facing 0.5 clip doze | lake 4 5 facing 1.0 | stone 5 6 facing 2.0 clip doze when done market_day

# Rowan waits by the Nesting Stone to tell his story, and everyone gathers at the arena on the festival night.
spot rowan stone 4 3 facing 3.14 when step hilltop 3 and lantern stone
spot rowan arena 0 14 facing 0 when step lantern_festival 3 and hour 18 6
# Cinder, Rowan's old dragon (D138): asleep on the Lodge's porch, but never missing a festival; Custard
# about the meadow once the stray's found.
spot cinder arena 0 9 facing 3.14 clip idle when step lantern_festival 3 and hour 18 6
spot cinder keeper -4.2 6.6 facing -1.57 clip sleep
spot custard sanctuary -14 12 facing 0.6 when world found_stray
spot maple arena -4 12 facing -3.0 clip clap when step lantern_festival 3 and hour 18 6
spot bram arena 4 12 facing 3.0 when step lantern_festival 3 and hour 18 6
spot pip arena -1.5 11 facing -3.09 clip cheer when step lantern_festival 3 and hour 18 6
spot sable arena 6 11 facing 2.92 when step lantern_festival 3 and hour 18 6
spot wren arena 2 12.5 facing 3.07 clip clap when step lantern_festival 3 and hour 18 6

# ------------------------------------------------------------------------------------------ pickups and signs

pickup page_mill mill 6 1.2 "Pick up the map page" group page glint when active fig_map and not bit pages 0
  do bit pages 0, add pages_found 1
  * A page of Fig's map, snagged on the bridge's rail. In his scrawl: "The Mill. Turns. Good."
pickup page_orchard orchard 9 -3 "Pick up the map page" group page glint when active fig_map and not bit pages 1
  do bit pages 1, add pages_found 1
  * A page of Fig's map, stuck in an apple tree. "The Orchard. Apples. Possibly pears. Unclear."
pickup page_lake lake 6 -4 "Pick up the map page" group page glint when active fig_map and not bit pages 2
  do bit pages 2, add pages_found 1
  * A page of Fig's map, floating by the shore. "Here be ducks. Possibly dangerous."
pickup page_bridge mill -9 4 "Pick up the map page" group page glint when active fig_map and not bit pages 3
  do bit pages 3, add pages_found 1
  * A page of Fig's map, weighed down by a stone. "The bridge. Do not fall off. (I fell off.)"
pickup page_lodge lodge -5 9 "Pick up the map page" group page glint when active fig_map and not bit pages 4
  do bit pages 4, add pages_found 1
  * A page of Fig's map, tucked under Rowan's wobbly step. "The Lodge. Tea. Old man. Nice."

pickup fox_den market -72 38 "Look in the den" when active sir_flaps and not flag flaps_found and hour 17 21 and pouch bread 1
  do flag flaps_found, take bread 1
  * A fox peeks out of a hollow log, a chewed plush dragon between its paws.
  * You hold out a piece of hearth bread. The fox thinks about it for a long time.
  * It snatches the bread and trots away, leaving Sir Flaps behind. A bit chewed, but brave.
pickup fox_den_empty market -72 38 "Look in the den" when active sir_flaps and not flag flaps_found and hour 17 21 and not pouch bread 1
  * A fox peeks out of a hollow log, a chewed plush dragon between its paws. It won't give him up.
  * It sniffs the air hungrily. Maybe it would trade for some hearth bread. The Market sells it.
pickup fox_den_day market -72 38 "Look in the log" when active sir_flaps and not flag flaps_found and not hour 17 21
  * A hollow log, full of fox smells. Nobody's home. Foxes come out at dusk.

pickup hives orchard 14 -6 "Calm the bees" when active tea and not flag honey_taken
  do flag honey_taken
  * The hives hum, cross and busy. {D} breathes a slow, warm breath over them.
  * The humming softens to a sleepy buzz. You take a comb of golden honey. Not one sting!

sign wander_sign trailhead 2 7 "Read the sign"
  * WANDERINGS. Take a juvenile or older dragon out on the long trail: choose it here, then close your 3DS and walk.
  * Your steps take it further. It comes home with Gleam, trinkets for the hoard, and now and then a wild egg.
  * (One dragon wanders at a time. It keeps its bed, and it comes home muddy.)
