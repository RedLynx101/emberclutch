# Star of the Glade (docs/design/story.md, section 5A): Celestine's pageant line, with Linnet, Madder,
# Primrose the rival, and Fig as the world's worst stage crew. Celestine is FIERY about pageantry.

quest pageant_invite "An invitation in glitter"
  line pageant
  giver celestine
  gleam 60
  after market_day
  rumour "Celestine at Moonpetal Glade has been asking after you. Loudly."
  step "Visit Celestine at Moonpetal Glade" talk celestine where place glade
  step "Hear how the pageant works (the How it works button)" until flag pageant_explained where place glade
  step "Enter a show at the Glade and see it through" until count shows 1 where place glade
  step "Tell Celestine how it went" talk celestine where place glade

quest pageant_dazzle "Dressed to dazzle"
  line pageant
  giver linnet
  gleam 80
  after pageant_invite
  rumour "Linnet has opinions about what {D} should wear. Many, many opinions."
  step "Visit Linnet's Finery at the Glade" talk linnet where place glade
  step "Dress {D} in something from the stall" until flag dressed_up where place glade
  step "Win the Look round at a show" until flag look_won where place glade
  step "Tell Linnet how {D} looked" talk linnet where place glade

quest pageant_ember "The Ember ribbon"
  line pageant
  giver celestine
  gleam 120
  after pageant_dazzle
  step "Win all four Ember shows (their themes change each day)" until shows 1 where place glade
  step "Tell Celestine" talk celestine where place glade

quest pageant_flame "The Flame ribbon"
  line pageant
  giver celestine
  gleam 160
  after pageant_ember
  step "Win all four Flame shows" until shows 2 where place glade
  step "Tell Celestine" talk celestine where place glade

quest pageant_blaze "The Blaze ribbon"
  line pageant
  giver celestine
  gleam 200
  after pageant_flame
  step "Win all four Blaze shows" until shows 3 where place glade
  step "Tell Celestine" talk celestine where place glade

quest pageant_star "Star of the Glade"
  line pageant
  giver celestine
  gleam 300
  after pageant_blaze
  step "Win a Starfire show, the summit of pageantry" until showwon 3 where place glade
  step "The Grand Gala: see Celestine" talk celestine where place glade
  reward wear starfire_crown

# ------------------------------------------------------------------------------------------ Celestine

talk celestine
  rule first if step lantern_festival 3 and hour 18 6
    [huff] The great lantern is a touch too orange. I've said so. Nobody listens. It is still, I admit, MAGNIFICENT.
  rule once if not met celestine
    do start pageant_invite, advance pageant_invite
    ? read celestine_invite | [excited] YOU CAME! Of course you came. My letters are irresistible. Everyone comes.
    ? not read celestine_invite | [excited] A new face! At MY Glade! Of course you came. Everyone comes, eventually.
    [proud] I am Celestine. I host the Moonpetal Pageant. Twelve years. Undefeated. In matters of TASTE.
    [thinking] Let me look at you both. Turn. Slowly. Slower. Yes. Yes...
    ? not wearing starfire_crown | [shock] Is that... is that MUD? On a DRAGON? At MY Glade?!
    [angry] No. No, no, no. We do not panic. Pageantry does not panic.
    [thinking] We can work with this. Sparkle is a DISCIPLINE, darling. And discipline starts with a BATH.
    [happy] Press How it works on my board and I'll tell you everything. Then enter a show. Feel the MAGIC.
  rule chat if step pageant_invite 2
    [excited] How it works, darling! On my board! Knowledge is sparkle's older sister!
  rule chat if step pageant_invite 3
    [love] Enter a show! Any show! The lights! The judges! Three rounds of pure, radiant DRAMA!
  rule if step pageant_invite 4
    do finish pageant_invite, start pageant_dazzle
    [love] Did you FEEL that? The lights! The judges! The DRAMA! That, darling, is PAGEANTRY.
    ? count shows 2 | [proud] And you'd already done it before! A natural. I KNEW it. I know everything.
    [thinking] Now. {D} has potential. Raw, glorious potential. But potential needs a hat.
    [happy] Go and see Linnet at her Finery. She'll dress you. Badly, perhaps. But with enthusiasm.
  rule if step pageant_ember 2
    do finish pageant_ember, start pageant_flame
    [crying] The EMBER RIBBON! I'm not crying. The moonpetals are very pollen-y tonight.
    @fig [proud] I did the lights!
    [huff] You did the lights ON FIRE, Fig.
    @fig [happy] Dramatic lighting!
    [proud] ...It was dramatic. I'll give him that. On to the Flame shows, darling!
  rule if step pageant_flame 2
    do finish pageant_flame, start pageant_blaze
    [excited] The FLAME ribbon! Plume fell off her chair. Wick dropped his card. Tansy SANG. Unheard of!
    [proud] You two are becoming something, darling. Something with SPARKLE.
  rule if step pageant_blaze 2
    do finish pageant_blaze, start pageant_star
    [proud] The Blaze ribbon. Sit with me a moment, darling.
    [wistful] Before I hosted, I had a dragon of my own on this stage. Ruby. Oh, she SHONE.
    [sad] She's old now. She naps in the sun behind my wagon and she doesn't like the lights anymore.
    [happy] So I make the stage shine for everyone else's dragons. For yours. That's why I shout, you know.
    [excited] Only the Starfire shows remain! The summit! Win just ONE and I'll throw you a GALA!
  rule if step pageant_star 2
    do finish pageant_star
    * The whole Glade glows. Moonpetals open, every one, and the judges stand to applaud.
    [crying] The GRAND GALA! For you, darling! For {D}! For PAGEANTRY ITSELF!
    [proud] By the power vested in me by absolutely nobody, I crown you STAR OF THE GLADE.
    * A small, old red dragon pads out from behind Celestine's wagon and nuzzles {D}. Ruby.
    [crying] She came out. She never comes out. She came out for you.
    [love] The Starfire crown is yours. Wear it with sparkle. Wear it with DISCIPLINE.
  rule chat if done pageant_star
    vary
      [love] Star of the Glade! I tell everyone. I told a tree. The tree was impressed.
      --
      [proud] Ruby's been sitting in the front row since the gala. In the sun. She approves of you.
  rule
    vary
      [excited] The themes change every day, darling! Read the theme, dress for it, and SHINE.
      --
      [angry] Someone entered a dragon with a BURR in its tail yesterday. A BURR. I had to lie down.
      --
      [love] Look at the moonpetals. They open for dragons that shine. They're opening for {D}. A little.
      --
      [huff] Linnet says my gowns are too purple. There is no such thing as too purple. Write that down.

# ------------------------------------------------------------------------------------------ Linnet and Madder

talk linnet
  rule if step pageant_dazzle 1
    do advance pageant_dazzle
    [love] OH. Oh, look at {D}. A blank canvas! A glorious, unaccessorised canvas!
    [excited] Welcome to Linnet's Finery, darling! Ribbons soft as a sigh, hats bright as a secret!
    [huff] Celestine sent you? Celestine thinks sequins are a food group.
    [happy] Pick anything from my stall and dress {D} in the wardrobe. Then win a Look round. Show them!
  rule chat if step pageant_dazzle 2
    [happy] Anything at all, darling! A bow, a bell, a hat. The wardrobe's right here.
  rule chat if step pageant_dazzle 3
    [excited] Now the Look round! The judges love a good match to the theme. And a CLEAN dragon.
  rule if step pageant_dazzle 4
    do finish pageant_dazzle, start pageant_ember
    [love] You won the Look round! In MY finery! I'm going to faint. I'm going to faint beautifully.
    @celestine [huff] It was the bath, Linnet. The bath did most of it.
    [huff] Celestine thinks beige is a colour.
    @celestine [angry] I have NEVER.
    [happy] Come back any time, darling. New stock every day.
  rule once if not met linnet
    [love] Oh, look at {D}! Welcome to Linnet's Finery! Hats, bows, ribbons, darling. Life is short: wear the hat.
  rule
    vary
      [love] This ribbon is simply too lovely to be legal.
      --
      [excited] A hat for every head, darling! Even the pointy ones.
      --
      [huff] Madder says I talk too much about hats. I say he talks too little about hats.

talk madder
  rule
    vary
      [calm] Dyes. Colours. Every shade of a dragon's dream. Well, most shades.
      --
      [thinking] Madder root makes red. That's where the name comes from. My mum was very practical.

# ------------------------------------------------------------------------------------------ Primrose

talk primrose
  rule once if not met primrose
    [proud] Primrose Pembrook. And this is Duchess, the finest dragon ever to grace Moonpetal Glade.
    [huff] You must be the new keeper. How... rustic. Is that grass on its claws?
    [cool] Beginner's luck is a lovely thing. It runs out. Duchess and I will see you at the shows.
  rule chat if shows 3
    vary
      [shy] Fine. FINE. You're good. You're very good. Duchess likes you. I suppose I do too. A bit.
      --
      [happy] Duchess wanted to say hello to {D}. She insisted. I didn't want to come over. I came over.
  rule chat if shows 1
    vary
      [huff] One title. It could happen to anyone. It has never happened to me, but it could.
      --
      [thinking] Your dragon's coat is... acceptable. Adequate. Fine, it's lovely. Don't tell Duchess I said so.
  rule
    vary
      [proud] Duchess has won eleven ribbons. Eleven. I've had them framed. The frames have ribbons.
      --
      [cool] Do try to keep up at the next show. It's so dull winning by a mile.

spot primrose glade -6 -4 facing 0.8 when met celestine
