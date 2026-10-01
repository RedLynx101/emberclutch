# Tam's Tall Tales (docs/design/story.md, section 5D): Fig sends you to Tam at Driftwood Cove, and Tam
# talks about nothing but fish. Old Whiskers, the one that got away, bites only at dusk.

quest cove_meet "Tam's Tall Tales"
  line cove
  giver tam
  gleam 60
  after market_day
  when flag fig_told_tam
  rumour "Fig says Tam at Driftwood Cove once caught a fish the size of a (small) house."
  step "Meet Tam at Driftwood Cove" talk tam where place cove
  step "Catch three fish off the shore" until count fish 3 where place cove
  step "Pick up a shell on the beach" until count shells 1 where place cove
  step "Tell Tam" talk tam where place cove

quest cove_whiskers "The One That Got Away"
  line cove
  giver tam
  gleam 150
  after cove_meet
  rumour "Tam has seen Old Whiskers off the jetty. At dusk."
  step "Catch Old Whiskers off the cove's shore, at dusk" until flag caught_whiskers where place cove
  step "Show Tam" talk tam where place cove
  reward wear lucky_lure

talk tam
  rule first if step lantern_festival 3 and hour 18 6
    [happy] Lovely lanterns. You know what else is lovely? Fish. I brought one. Want to see? It's in my pocket.
  rule once if not met tam
    do start cove_meet, advance cove_meet
    [calm] Oh. Hello there. I'm Tam. I fish.
    ? flag fig_told_tam | [laugh] Fig sent you? He told you about the house-sized fish. It was a shed. A big shed.
    [happy] You want to learn? Easy. Watch the bobber. Little nibbles, wait. Big dip, strike. That's the whole secret.
    [thinking] Then keep the line's tension in the band while it runs. Too tight, it snaps. Too loose, it's off.
    [happy] River Fish, mostly. Now and then a shell, or a pearl if you're lucky. Shells wash up on the beach too.
    [calm] Catch three, find a shell, come tell me. I'll be here. I'm always here.
  rule if step cove_meet 1
    do advance cove_meet
    [calm] Oh. Hello. Fig mentioned you. You want to fish? Watch the bobber. Big dip, strike. Keep the tension. Easy.
    [happy] Catch three, find a shell on the beach, come tell me. I'll be here. I'm always here.
  rule if step cove_meet 4
    do finish cove_meet, start cove_whiskers
    [happy] Three fish and a shell. You're a natural. Took me a whole summer. I was very small.
    [thinking] Now. Between you and me. There's a catfish in this cove. Old Whiskers. Big as a boat.
    [calm] A small boat. A rowing boat. A big rowing boat. He bites at dusk, and he's never been landed.
    [excited] If anyone can, it's you. Just... tell me first. I want to see his face. His fish face.
  rule if step cove_whiskers 2
    do finish cove_whiskers
    [shock] That's... that's HIM. That's Old Whiskers.
    [crying] Forty years. Forty years I've... he's beautiful. Look at his whiskers. Look at them.
    [happy] Here. My lucky lure. I won't need it now. I've seen him. That's enough for one life.
    * Tam gently lets Old Whiskers back into the water. He swims off, looking very pleased with himself.
  rule chat if active cove_whiskers
    [thinking] Dusk. Off the shore. Old Whiskers doesn't like to be rushed. Neither do I.
  rule
    vary
      [happy] Caught a fish this morning. Let it go. It looked busy.
      --
      [laugh] Why don't fish play cards? Afraid of the net. I've got more. Want more? I've got loads.
      --
      [calm] Just for the halibut, I tried fishing with a sock once. Caught a sock.
      --
      [thinking] Every fish I've ever caught was this big. Give or take. Mostly take.
