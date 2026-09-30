# The Long Count (docs/design/story.md, section 5C): Tove and Frostspire Hollow. A guardian every fifth
# floor; floor 30's is the Frost Warden, the oldest wild dragon in the Hollow. Solenne was Tove's
# best student.

quest hollow_count "The Long Count"
  line hollow
  giver tove
  gleam 80
  rumour "Tove keeps count of the wild dragons at Frostspire Hollow, up in the cold heights."
  step "Meet Tove at Frostspire Hollow" talk tove where place hollow
  step "Clear the Hollow's first floor" until hollow 1 where place hollow
  step "Beat the guardian of floor 5" until hollow 5 where place hollow
  step "Tell Tove" talk tove where place hollow

quest hollow_deeper "Deeper still"
  line hollow
  giver tove
  gleam 150
  after hollow_count
  step "Beat the guardian of floor 10" until hollow 10 where place hollow
  step "Tell Tove" talk tove where place hollow
  step "Beat the guardian of floor 15" until hollow 15 where place hollow
  step "Tell Tove" talk tove where place hollow
  step "Beat the guardian of floor 20" until hollow 20 where place hollow
  step "Tell Tove" talk tove where place hollow

quest hollow_warden "The Frost Warden"
  line hollow
  giver tove
  gleam 300
  after hollow_deeper
  step "Beat the guardian of floor 25" until hollow 25 where place hollow
  step "Tell Tove" talk tove where place hollow
  step "Face the Frost Warden on floor 30" until hollow 30 where place hollow
  step "Tell Tove" talk tove where place hollow
  reward wear warden_scale

talk tove
  rule first if step lantern_festival 3 and hour 18 6
    [calm] Three hundred and twelve lanterns in the crowd. I counted. Also the big one.
  rule once if not met tove and world glided
    do start hollow_count
    [cool] Not bad. Most drop like a stone. Twenty-eight metres. I counted.
    [calm] I'm Tove. I keep Frostspire Hollow, up past the Vault. Wild dragons come there to grow strong.
    [thinking] Come and see me. Bring {D}. We'll see how far you get before I lose count.
  rule once if not met tove
    do start hollow_count, advance hollow_count
    [calm] Welcome to Frostspire Hollow, keeper. I'm Tove. Wild dragons live in the caves behind me.
    [thinking] Each floor, one comes out to test you. The deeper you go, the stronger they are.
    [calm] Every fifth floor there's a guardian. Beat one and you can start from there next time.
    [cool] I count. Floors, dragons, snowflakes. It keeps the Hollow honest.
  rule if step hollow_count 1
    do advance hollow_count
    [calm] You came. Good. Floor one is through the cave door. I'll be counting.
  rule if step hollow_count 4
    do finish hollow_count, start hollow_deeper
    [calm] Five floors. The first guardian. Not bad at all.
    [thinking] The wild ones come here to grow strong before they cross the peaks. They test you the way they test each other.
    [cool] It isn't anger. It's sport. They respect a keeper who comes back.
  rule if step hollow_deeper 2
    do advance hollow_deeper
    [calm] Ten. My teacher kept this count before me. She said the Hollow tells you who you are.
    [thinking] It mostly told me I was cold. But she had a point.
  rule if step hollow_deeper 4
    do advance hollow_deeper
    [calm] Fifteen. You're past where most keepers turn back.
    [wistful] Solenne reached twenty at your age. She was my best student. Don't tell her I said so. She knows.
  rule if step hollow_deeper 6
    do finish hollow_deeper, start hollow_warden
    [happy] Twenty.
    * Tove smiles. It's small, and it goes away quickly, but it was definitely a smile.
    [calm] Past twenty-five, the cold gets into your bones. And at thirty waits the Frost Warden. The oldest one.
  rule if step hollow_warden 2
    do advance hollow_warden
    [calm] Twenty-five. The Warden knows you're coming. It's been waiting a long time.
  rule if step hollow_warden 4
    do finish hollow_warden
    [happy] Thirty. I've counted to thirty for twenty years, waiting for someone to reach it.
    [wistful] The Warden left you something. It does that, for the ones it respects. A scale, cold as starlight.
    [calm] Wear it. And come back sometimes. I'll keep counting.
  rule chat if begun hollow_deeper and not done hollow_warden
    [calm] The door's there. The count's waiting.
  rule
    vary
      [calm] Forty-one snowflakes on your hat. You're late. For nothing in particular.
      --
      [cool] The wild ones are restless today. Good day for the Hollow.
      --
      [thinking] Solenne visits sometimes. She brings tea. It's always cold by the time it gets up here.

spot tove vault 4 5 facing 3.3 when world glided and not met tove
