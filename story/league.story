# The Road to Champion (docs/design/story.md, section 5B): Wren and the four champions. Beating
# Solenne makes you the fifth champion of Skyreach Valley (Noah: "One of the five champions in the
# valley"); the Hollow doesn't have to be finished. Levels top out at 42 in Skyreach.

quest league_signup "The Road to Champion"
  line league
  giver wren
  gleam 50
  when place arena
  rumour "Wren keeps the battle league's board at the arena. Challengers stand all about the valley."
  step "Sign up at the league board with Wren" talk wren where place arena
  step "Win a battle against an Ember challenger" until beaten 0 0 where place market
  step "Tell Wren" talk wren where place arena

quest league_ember "The Ember league"
  line league
  giver wren
  gleam 100
  after league_signup
  step "Beat the four Ember challengers (the board says where)" until beaten 0 0 and beaten 0 1 and beaten 0 2 and beaten 0 3 where place arena
  step "Beat Marigold, the Ember champion, at Emberpeak Caldera" until league 1 where place caldera
  step "Tell Wren" talk wren where place arena

quest league_flame "The Flame league"
  line league
  giver wren
  gleam 150
  after league_ember
  step "Beat the four Flame challengers" until beaten 1 0 and beaten 1 1 and beaten 1 2 and beaten 1 3 where place arena
  step "Beat Captain Rook at Emberpeak Caldera" until league 2 where place caldera
  step "Tell Wren" talk wren where place arena

quest league_blaze "The Blaze league"
  line league
  giver wren
  gleam 200
  after league_flame
  step "Beat the four Blaze challengers" until beaten 2 0 and beaten 2 1 and beaten 2 2 and beaten 2 3 where place arena
  step "Beat Seraphine at Emberpeak Caldera" until league 3 where place caldera
  step "Tell Wren" talk wren where place arena

quest league_starfire "The Starfire league"
  line league
  giver wren
  gleam 400
  after league_blaze
  step "Beat the four Starfire challengers" until beaten 3 0 and beaten 3 1 and beaten 3 2 and beaten 3 3 where place arena
  step "Beat Solenne, the Starfire champion, at Emberpeak Caldera" until league 4 where place caldera
  step "Tell Wren: you're a champion of Skyreach Valley" talk wren where place arena

# ------------------------------------------------------------------------------------------ Wren

talk wren
  rule once if new league_signup
    do start league_signup, advance league_signup
    [proud] You want the battle league? Excellent! Name, dragon, a steady nerve. Done! You're on the board.
    [happy] Four challengers stand about the valley each league. Beat all four, and the champion waits at the caldera.
    [thinking] Tamsin's the first, at the Market. She and Pepper practise all week. Be kind. Or don't. It's a battle.
  rule if step league_signup 3
    do finish league_signup, start league_ember
    [excited] A win! Your first league win! I blew my whistle so hard a pigeon fell over.
    [proud] Three more Ember challengers, then Marigold at the caldera. The board shows who's where.
  rule if step league_ember 3
    do finish league_ember, start league_flame
    [excited] You beat MARIGOLD! The Ember title! Oh, my clipboard. Oh, my WHISTLE.
    [shy] I'm not a fan. I'm the steward. I'm very professional. I have all four of her trading cards.
    [proud] The Flame league's challengers have arrived. And Captain Rook waits at the caldera. He's... intense.
  rule if step league_flame 3
    do finish league_flame, start league_blaze
    [laugh] You beat Captain Rook! Did he say something dramatic? He always says something dramatic.
    [happy] Then he goes and sits on the Windmill Bridge and writes poems. Everybody knows. Nobody mentions it.
    [proud] The Blaze league now. Seraphine at the caldera. She's graceful. Terrifyingly graceful.
  rule if step league_blaze 3
    do finish league_blaze, start league_starfire
    [proud] The Blaze title. Only the Starfire league remains, and then... Solenne. The Starfire champion herself.
    [worried] She's never lost. Not once in nine years. Not that I'm worried. I'm the steward. Stewards don't worry.
  rule if step league_starfire 3
    do finish league_starfire
    [crying] You BEAT her. You beat SOLENNE. I'm not crying. The whistle's in my eye.
    [proud] {P}, you and {D} are champions of Skyreach Valley. One of the five, now: Marigold, Rook, Seraphine, Solenne, and YOU.
    [happy] Here's your Champion badge. Wear it with pride. I made it myself. Professionally.

# ------------------------------------------------------------------------------------------ the champions, out in the valley

talk marigold
  rule first if step lantern_festival 3 and hour 18 6
    [happy] Sunny's been glowing all evening! Look, he's matching the lanterns!
  rule once if league 1
    [happy] Champion! Well, an Ember champion. Sunny and I tend the orchard now, when we're not at the caldera.
    [happy] Here, a marigold. I give everyone flowers. It's a whole thing.
    [proud] You're welcome to a rematch any time. Sunny never says no.
  rule
    vary
      [happy] The skyberries are coming along beautifully! Sunny keeps them warm at night.
      --
      [proud] You've got a real fire in you, you know. And so does {D}.

talk rook
  rule first if step lantern_festival 3 and hour 18 6
    [cool] Nice lantern. It's fine. I've seen better. I haven't seen better.
  rule once if league 2
    [cool] You found me. Nobody finds me. I come here to be alone with the wind.
    [proud] It's not a poem. It's notes. For a poem. About the wind. It's a very serious poem.
    [huff] Don't read it. ...Fine. "The wind. It blows. Like me." It's not finished.
    [cool] Bramble likes you, by the way. Bramble likes NOBODY. Bramble is fearsome.
    * Bramble, a small mossy dragon, is asleep in a patch of daisies.
  rule
    vary
      [cool] The sky is always watching. So I watch it back.
      --
      [proud] Bramble is the most fearsome dragon on the lake. Look at him. Terrifying.
      * Bramble sneezes a daisy.
      --
      [cool] Rematch? Heh. You're brave. Or foolish. The line is thin. Like my patience. Which is long, actually.
      --
      [shy] I've written you a poem. "Keeper. Dragon. Win." It's a haiku. It's not a haiku.

talk seraphine
  rule first if step lantern_festival 3 and hour 18 6
    [calm] The lanterns are well placed. Symmetrical. I approve.
  rule once if league 3
    [calm] You found me at practice. Nightshade and I dance here at dusk. It sharpens the mind.
    [proud] You fought with grace. Your posture, less so. Shoulders back.
    [happy] ...Better.
  rule
    vary
      [calm] Balance. In battle, in dance, in breakfast.
      --
      [proud] Nightshade learned this turn in a week. I took a year. She is insufferable about it.

talk solenne
  rule first if step lantern_festival 3 and hour 18 6
    [wistful] Starlight came down with the star dragon, long ago. Tonight she's watching the sky too.
  rule once if league 4
    [happy] Hello, champion. It's nice to say that to someone new.
    [wistful] I come here at night to watch the sky. The old tower sees further than anywhere in the valley.
    [thinking] Beyond the gap there are other valleys. Other keepers, other champions, other dragons.
    [happy] Skyreach is just the first of many, {P}. Isn't it wonderful? There's always further to go.
  rule
    vary
      [wistful] Look, there. That bright one. Tove taught me its name. I've forgotten it. Don't tell her.
      --
      [happy] One of the five champions of Skyreach Valley. How does it feel? It felt strange to me too, once.
      --
      [thinking] Starlight wants a rematch. She's very polite about it. She's also very strong.

# (the festival night's first: a person's first spot that holds is where they stand)
spot marigold arena -8.5 9 facing -2.81 when step lantern_festival 3 and hour 18 6 and league 1
spot rook arena 8.5 9 facing 2.81 when step lantern_festival 3 and hour 18 6 and league 2
spot seraphine arena -10.5 6 facing -2.69 when step lantern_festival 3 and hour 18 6 and league 3
spot solenne arena 10.5 6 facing 2.69 when step lantern_festival 3 and hour 18 6 and league 4
spot marigold orchard -6 6 facing 0.9 clip tidy when league 1
spot rook mill -6 1 facing 1.6 clip look_around when league 2 and hour 16 23
spot rook lake -3 6 facing 0.5 when league 2
spot seraphine glade 7 -5 facing -0.9 clip stretch when league 3
spot solenne ruins 2 3 facing 0 clip look_around when league 4 and hour 19 5
spot solenne caldera 0 6 facing 3.14 when league 4
