// Every player-facing string (D30: English only for v1; a translation adds a table here).
#pragma once

#ifndef EC_DEV
#define EC_DEV 1
#endif
#ifndef EC_VERSION
#define EC_VERSION "0.0.0"  // (the Makefile's VERSION)
#endif

namespace ec::str {

inline constexpr const char* kGameTitle = "Emberclutch";
inline constexpr const char* kGameSubtitle = "Skyreach Valley";  // (1.0: Emberclutch: Skyreach Valley, D120)
inline constexpr const char* kGameFullTitle = "Emberclutch: Skyreach Valley";
inline constexpr const char* kTagline = "raise, breed and fly with dragons";
inline constexpr const char* kTouchToBegin = "Touch to begin";
// Under the title: the player build shows its version, the dev build says it is one (D135).
inline constexpr const char* kBuildLabel = EC_DEV ? "dev build " EC_VERSION : "v" EC_VERSION;
inline constexpr const char* kCredits = "Credits";
inline constexpr const char* kCreditsBy = "A game by Noah Hicks";
inline constexpr const char* kCreditsEmi = "Inspired by Emi";
inline constexpr const char* kCreditsEmiLine = "who makes every day feel like hatching day";
inline constexpr const char* kCreditsTools = "Nunito and Cinzel Decorative (SIL OFL)  -  devkitPro, citro2d, citro3d";
inline constexpr const char* kTitleByLine = "by Noah, for Emi";

inline constexpr const char* kChooseEgg = "Choose your first egg";
inline constexpr const char* kTapAgain = "Tap again to choose";
inline constexpr const char* kEggSuffix = "egg";
inline constexpr const char* kStarterBlurb[3] = {
    "Warm-hearted and bold. Breathes flame.",
    "Gentle and clever. Loves the water.",
    "Light and quick. Born to fly.",
};

inline constexpr const char* kDefaultName = "Kindle";
inline constexpr const char* kRubEgg = "Rub the egg to keep it warm";
inline constexpr const char* kGettingCold = "It's getting cold...";
inline constexpr const char* kIncubated = "incubated";
inline constexpr const char* kHatched = "It hatched! Say hello.";
inline constexpr const char* kWarmth = "Warmth";

inline constexpr const char* kBelly = "Belly";
inline constexpr const char* kEnergy = "Energy";
inline constexpr const char* kClean = "Clean";
inline constexpr const char* kPlay = "Play";
inline constexpr const char* kBond = "Bond";
inline constexpr const char* kDay = "Day";
inline constexpr const char* kNapping = "napping";

inline constexpr const char* kStrokeToPet = "Stroke here to pet";
inline constexpr const char* kMakeUpHint = "Hold still, then offer a treat...";
inline constexpr const char* kFeed = "Feed";
inline constexpr const char* kGroom = "Groom";
inline constexpr const char* kPlayBtn = "Play";
inline constexpr const char* kMakeUp = "Make up";

inline constexpr const char* kFedFavorite = "Its favourite! A happy wiggle.";
inline constexpr const char* kFed = "Munch munch.";
inline constexpr const char* kPlayed = "Fetch! It bounds after the ball.";
inline constexpr const char* kMadeUp = "Its heartglow lights up again.";
// Hands-on care (WP7)
inline constexpr const char* kSweetSpot = "It loves that spot!";
inline constexpr const char* kRefused = "Not that one... it turns its nose up.";
inline constexpr const char* kGleaming = "Clean and gleaming, nose to tail!";
inline constexpr const char* kComeHere = "Come here!";
inline constexpr const char* kRinse = "Rinse";
inline constexpr const char* kHintPet = "Stroke it.  L / R: turn.  Hold still off it to call.";
inline constexpr const char* kHintFood = "Pick a food, hold it to its mouth.";
inline constexpr const char* kHintBrush = "Brush head to tail: it loves it.  L / R: turn.";
inline constexpr const char* kHintBath = "Rub in the suds, then rinse.";
inline constexpr const char* kHintBall = "Flick to throw, drag to roll.";
inline constexpr const char* kHintFeather = "Dangle the feather near its face.";
inline constexpr const char* kHintRope = "Hold on: it bites and tugs. Let go!";
inline constexpr const char* kHintOrb = "Roll the orb about: a treat drops out.";
inline constexpr const char* kMoreToys = "More toys at the Market.";
// The tray's places (D85): Outing, Journal, Den.
inline constexpr const char* kOuting = "Outing";
inline constexpr const char* kPartnerIs = "Your travel partner: %s the %s";
inline constexpr const char* kNoPartner = "No travel partner yet";
inline constexpr const char* kPartnerWhy = "Your partner goes out into the valley with you.";
inline constexpr const char* kEggStays = "An egg stays warm in its nest.";
inline constexpr const char* kEggStaysHome = "It's dangerous to go alone! Eggs stay warm at home: hatch a dragon to take along.";
inline constexpr const char* kDangerousAlone = "It's dangerous to go alone! Take a dragon with you.";
// Reaching for a sleeping dragon (run 21: it just didn't answer); one %s, its name.
inline constexpr const char* kAsleep[3] = {"%s is fast asleep.", "%s is sleeping soundly.", "%s is snoozing. Let it rest."};
inline constexpr const char* kOutingRide = "%s is grown: ride it, fly, land and walk about.";
inline constexpr const char* kOutingLead = "%s walks beside you on its lead, and follows you anywhere.";
inline constexpr const char* kMakePartner = "Make %s your partner";
inline constexpr const char* kHeadOut = "Head out together";
inline constexpr const char* kHeadOutWith = "Head out with %s";
inline constexpr const char* kJournal = "Journal";
inline constexpr const char* kJournalQuests = "Quests";
inline constexpr const char* kJournalEgg = "The egg";
inline constexpr const char* kJournalPlaces = "Places";
inline constexpr const char* kQuestDone = "Done!";
inline constexpr const char* kNoQuests = "No quests yet. Care for your egg: someone will write when it hatches.";
inline constexpr const char* kRumourTitle = "Rumour: %s";
inline constexpr const char* kPlacesFound = "Places found: %d of %d";
inline constexpr const char* kDenTitle = "Your den";
inline constexpr const char* kDecorSpotNames[5] = {"Rug", "Lantern", "Perch", "Plant", "Banner"};
inline constexpr const char* kDecorNone = "(bare)";
inline constexpr const char* kDenHint = "New things turn up at the Market's stall each day.";
// The valley (Beta)
inline constexpr const char* kFoundPlace = "Found: %s";
inline constexpr const char* kStarEgg = "A star-born %s egg is yours!";
inline constexpr const char* kPromptTalk = "A: talk to %s";
inline constexpr const char* kFindGleam = "Found %u Gleam!";
inline constexpr const char* kFindTrinket = "Found a %s for the hoard!";
inline constexpr const char* kFindsFound = "Finds: %d of %d";
inline constexpr const char* kFindEgg = "Found a wild egg! It's waiting at home.";
inline constexpr const char* kPromptEggStand = "A: the egg of the day";
inline constexpr const char* kPromptGoods = "A: see the goods";
inline constexpr const char* kFoundStray = "%s sniffed out the stray! She's safe.";
inline constexpr const char* kStrayScent = "%s has caught a scent... keep going!";
inline constexpr const char* kStallEmpty = "Sold for today";
inline constexpr const char* kStallTomorrow = "New things on the stall tomorrow.";
inline constexpr const char* kLanternLit = "This lantern is burning bright.";
inline constexpr const char* kLanternFestival = "The great lantern waits for the festival night.";
inline constexpr const char* kPartnerCame = "%s came running!";
inline constexpr const char* kLanternLitAt = "A lantern lit: %s";
inline constexpr const char* kQuestFinished = "Quest done: %s";
inline constexpr const char* kQuestStarted = "New quest: %s";
inline constexpr const char* kMailArrived = "A letter in your mailbox: %s";
inline constexpr const char* kPromptEnter = "A: go into %s";
inline constexpr const char* kPromptLight = "A: light the lantern";
inline constexpr const char* kPromptRide = "A: climb on and ride";
inline constexpr const char* kPromptCall = "A: call your partner";
inline constexpr const char* kTravelledTo = "Off to %s";
inline constexpr const char* kRidingGround = "Riding (on the ground)";
inline constexpr const char* kFlying = "Flying";
inline constexpr const char* kSwimming = "Swimming";
inline constexpr const char* kLetterTitle = "A letter, left at a picnic";
inline constexpr const char* kLetterTo = "Emi,";
inline constexpr const char* kLetterBody = "I love you more than sky reaches above.";
inline constexpr const char* kLetterFrom = "-Noah";
inline constexpr const char* kLetterGleamLine = "and %u Gleam tucked in the basket";
inline constexpr const char* kLetterClose = "Fold it away";
inline constexpr const char* kFreeCamera = "Looking about";
inline constexpr const char* kOnFoot = "On foot";
inline constexpr const char* kPlacesAndLanterns = "Places %d/%d  Lanterns %d/%d";
inline constexpr const char* kHelpFoot = "Pad: walk  B: run\nL/R: turn the view\nA: what's near\nTap a pin to travel";
inline constexpr const char* kHelpRideGround = "Pad: walk  B: run\nA: take off\nDown: get off";
inline constexpr const char* kHelpFly = "Pad: steer  A: flap\nB: dive  R: burst\nL: brake  Let go: glide";
inline constexpr const char* kHelpFreeCam = "Pad: move  L/R: turn\nD-pad: up, down, tilt\nB: faster  A: photo\nX: back";
inline constexpr const char* kLook = "Look";
inline constexpr const char* kCall = "Call";
inline constexpr const char* kHomeX = "Home";
// Run 19 (D89): the valley's Journal on X, fast travel asked first, home at the den's door, photos.
inline constexpr const char* kTravelAsk = "Travel to %s?";
inline constexpr const char* kFindFood = "Found a %s! Into the pouch.";
inline constexpr const char* kFeedOut = "Feed";
inline constexpr const char* kTooTiredToPlay = "%s is too tired to play. Let it rest.";
inline constexpr const char* kTreatPick = "Something from the pouch:";
inline constexpr const char* kTreatNone = "The pouch is empty.";
inline constexpr const char* kTreatAte = "%s ate it up!";
inline constexpr const char* kTreatRefused = "%s turns its nose up at that.";
inline constexpr const char* kTreatFull = "%s is full.";
inline constexpr const char* kTravelGo = "Go";
inline constexpr const char* kTravelStay = "Stay";
inline constexpr const char* kPromptHome = "A: go home";
inline constexpr const char* kJournalX = "Journal (X)";
inline constexpr const char* kValleyPhoto = "Skyreach Valley";
inline constexpr const char* kIntoBowl = "Into the bowl.";
inline constexpr const char* kBowlFull = "The bowl is full.";
inline constexpr const char* kTreat = "A treat!";
inline constexpr const char* kOrbEmpty = "The orb is empty for now.";
inline constexpr const char* kProfileClose = "Close";
inline constexpr const char* kRename = "Rename";
inline constexpr const char* kPersonality = "Personality";
inline constexpr const char* kRenamed = "A fine new name.";

// The valley's critters (workstream L, core/critters Kind order): what A does near one, the toast
// when it's befriended (%s: your partner), and the Journal's page.
inline constexpr const char* kCritterPrompt[7] = {"A: whistle to the birds", "A: play chase", "A: play chase", "A: hold still",
                                                  "A: croak back", "A: call the ducks", "A: sit quietly"};
inline constexpr const char* kCritterName[7] = {"Songbirds", "Rabbits", "Snow hares", "Butterflies", "Frogs", "Ducks", "Foxes"};
inline constexpr const char* kCritterNote[7] = {
    "Peck about the meadows in little flocks. Walk softly, then whistle.",
    "Nibble at the woods' edge. Quick to bolt, and quicker to play.",
    "White as the snow round Frostspire Hollow. Champions at hide-and-seek.",
    "Dance over the flowers by day. Hold still and one may land.",
    "Sing at the water's edge, louder at night. Croak back!",
    "Paddle about in a little line. Call, and over they come.",
    "Out at dusk by the woods. Shy, but curious about dragons."};
inline constexpr const char* kCritterFriend[7] = {"A songbird hopped over to say hello!", "%s chased a rabbit. It got away, giggling!",
                                                  "%s chased a snow hare round the snow!", "A butterfly landed on %s!",
                                                  "The frog croaked back at you!", "The ducks paddled over to say hi!",
                                                  "The fox booped noses with %s!"};
inline constexpr const char* kCritterFriendAlone = "A butterfly landed on your head!";
inline constexpr const char* kCritterSpotted = "New in the Journal: %s";
inline constexpr const char* kCritterFirst = "A new critter friend: %s! +%d Gleam";
inline constexpr const char* kCritterDaily = "A critter friend today: +%d Gleam";
inline constexpr const char* kCrittersTitle = "Valley critters";
inline constexpr const char* kCrittersButton = "Critters";
inline constexpr const char* kCrittersCount = "Friends %d of %d";
inline constexpr const char* kCrittersUnseen = "???";
inline constexpr const char* kCrittersUnseenNote = "Not spotted yet. Keep exploring the valley, by day and by night.";
inline constexpr const char* kCrittersSeen = "Spotted";
inline constexpr const char* kCrittersFriends = "Friends x%d";
inline constexpr const char* kCrittersToday = "Friends today: %d";
inline constexpr const char* kCrittersTreats = "Treats left today: %d";
inline constexpr const char* kCrittersPlaces = "< Places";
inline constexpr const char* kCrittersTap = "Tap one for its note.";

// The challenges (Beta WP8-WP11: Sky Rings, the Lantern Trial, Fruit Catch)
inline constexpr const char* kPromptBoard = "A: the challenges";
inline constexpr const char* kChallenges = "Challenges";
inline constexpr const char* kHeldAt = "at %s";
inline constexpr const char* kStartCup = "Start!";
inline constexpr const char* kLeave = "Leave";
inline constexpr const char* kGoThere = "Go there";
inline constexpr const char* kCupLocked = "Locked";
inline constexpr const char* kBestTime = "Best %.1f s";
inline constexpr const char* kParTime = "Par %.1f s";
inline constexpr const char* kBestPoints = "Best %d";
inline constexpr const char* kGoalPoints = "Goal %d";
inline constexpr const char* kNoBest = "Not run yet";
inline constexpr const char* kCupWon = "%s won!";
inline constexpr const char* kCupPlaced = "So close! Nearly there.";
inline constexpr const char* kCupTryAgain = "Not this time. Try again!";
inline constexpr const char* kNewRibbon = "A new ribbon and trophy for your den!";
inline constexpr const char* kNewBest = "A new best!";
inline constexpr const char* kGleamPrize = "+%lu Gleam";
inline constexpr const char* kAgain = "Again";
inline constexpr const char* kDone = "Done";
inline constexpr const char* kGoCount = "Go!";
inline constexpr const char* kGiveUp = "X: give up";
inline constexpr const char* kRingsCount = "%d/%d rings";
inline constexpr const char* kRingsMissed = "%d missed (+%d s)";
inline constexpr const char* kRingBehind = "The next ring is behind you!";
inline constexpr const char* kTimeUp = "Time's up!";
inline constexpr const char* kYourTime = "Your time: %.1f s";
inline constexpr const char* kHighLantern = "The high lantern is lit!";
inline constexpr const char* kWatch = "Watch the lanterns...";
inline constexpr const char* kYourTurn = "Your turn!";
inline constexpr const char* kRoundOf = "Round %d of %d";
inline constexpr const char* kRoundCleared = "Round cleared!";
inline constexpr const char* kWatchAgain = "Oops! Watch again...";
inline constexpr const char* kTapLanterns = "Tap the lanterns in the same order.";
inline constexpr const char* kGreatLantern = "The great lantern blazes over the valley!";
inline constexpr const char* kFlickFruit = "Flick a fruit up and away!";
inline constexpr const char* kThrowOf = "Throw %d of %d";
inline constexpr const char* kPoints = "%d points";
inline constexpr const char* kGoldenNext = "A golden pear: double points!";
inline constexpr const char* kAteItAnyway = "Missed... but it eats it anyway.";
inline constexpr const char* kHelpRings = "Pad: steer  A: flap\nB: dive\nR: burst  L: brake";
inline constexpr const char* kHelpPicker = "L/R: challenge\nLeft/Right: cup\nA: start  B: leave";
// The hosts: Wren at the arena, Maple at the orchard ({D} your dragon, {P} you).
inline constexpr const char* kHostRings[] = {"Sky Rings! Race the other dragons through every ring in order. A missed ring costs three seconds.",
                                              "R bursts ahead while {D}'s Stamina lasts, L brakes into the tight turns. First to the last ring wins!"};
inline constexpr const char* kHostLanterns[] = {"The Lantern Trial! The crystal lanterns will light up in a pattern.",
                                                 "Then tap them in the same order, and {D} will breathe each one alight."};
inline constexpr const char* kHostFruit[] = {"Fruit Catch! Flick a fruit down the meadow and {D} will run for it.",
                                              "Longer throws score more, fancy catches extra, and golden pears count double!"};
inline constexpr const char* kHostWon[] = {"Wonderful! What a pair you two are.", "That's the cup! The crowd loves you.",
                                            "Now that's how it's done!"};
inline constexpr const char* kHostPlaced[] = {"So close! Next time, I'm sure of it.", "Nearly! Have another go."};
inline constexpr const char* kHostTry[] = {"Don't worry, everyone starts somewhere.", "A little practice, and you'll get it!"};

// The challenges, 1.0 (workstream C, D89): the day's prizes, energy, the record, Sky Rings' race
// (rivals, the burst and brake, Stamina), Fruit Catch's breeze, what the next cup needs.
inline constexpr const char* kTooTired = "Too tired! Let it rest first.";
inline constexpr const char* kPrizeFirst = "First win: %lu Gleam and a trophy";
inline constexpr const char* kPrizeToday = "Today's prize: %lu Gleam";
inline constexpr const char* kPrizeTaken = "Today's prize won. Come back tomorrow!";
inline constexpr const char* kGleamXp = "+%lu Gleam    +%lu xp";
inline constexpr const char* kXpOnly = "+%lu xp";
inline constexpr const char* kPaidToday = "Today's prize for this cup is yours already.";
inline constexpr const char* kDragonFirstCup = "%s's first %s!";
inline constexpr const char* kTrophyUp = "The %s trophy is on your den's shelf!";
inline constexpr const char* kLevelUpTo = "%s reached level %d!";
inline constexpr const char* kNextRivals = "Next: the %s. Beat %d faster rivals!";
inline constexpr const char* kNextGoal = "Next: the %s. %d points to win.";
inline constexpr const char* kNextTrial = "Next: the %s. %d lanterns, %d rounds, %d hearts.";
inline constexpr const char* kNextGrown = "(For a grown dragon.)";
inline constexpr const char* kNextJuvenile = "(For a juvenile dragon or older.)";
inline constexpr const char* kAllCupsWon = "Every cup won! The Starfire trophy is yours.";
inline constexpr const char* kBeatRivals = "Beat %d rivals";
inline constexpr const char* kTrialNeeds = "%d lanterns, %d rounds";
inline constexpr const char* kStamina = "Stamina";
inline constexpr const char* kPlaceNth[] = {"1st", "2nd", "3rd", "4th"};
inline constexpr const char* kRaceYou = "You";
inline constexpr const char* kRaceDone = "%s place!  %.1f s";
inline constexpr const char* kRaceMissed = "%s place!  %.1f s (%d missed)";
inline constexpr const char* kRaceWinner = "%s won in %.1f s";
inline constexpr const char* kRaceBeatAll = "Ahead of %s by %.1f s";
inline constexpr const char* kRaceLine = "%s  %s  %.1f s";
inline constexpr const char* kBreeze = "Breeze";
inline constexpr const char* kCalm = "Calm";

// Driftwood Cove (workstream C, D90): Tam the fisher, fishing off the shore, shells on the beach.
inline constexpr const char* kFisherName = "Tam";
inline constexpr const char* kFisherTitle = "The fisher";
inline constexpr const char* kPromptFish = "A: fish here";
inline constexpr const char* kPromptShell = "A: pick up the shell";
inline constexpr const char* kFisherHello[] = {
    "Ahoy there! Name's Tam. The fish are biting nicely at the cove today.",
    "Here, take my spare rod. Stand at the water's edge and press A to cast.",
    "When the bobber dips right under, strike with A, quick! Little twitches are only nibbles: wait for the dip.",
    "Then reel it in gently. Keep the line taut, but not too taut, or it'll snap!",
    "And save a nibble for {D}. Dragons love a bite of fresh fish!"};
inline constexpr const char* kFisherTips[] = {
    "Dawn and dusk are the best times. The big ones come up to feed.",
    "When a fish runs, ease off the reel. Let it tire itself out.",
    "Shells wash up along the beach every morning. Keep an eye out for pearls!",
    "River fish are the pick of the lake. Hardly a dragon in the valley says no to one."};
inline constexpr const char* kFisherBiting = "Plenty still biting today. Off you go!";
inline constexpr const char* kFisherRested = "That's the lake fished out for today. The fish will be back tomorrow!";
inline constexpr const char* kFisherCount = "That's %d fish you've landed now. A proper angler!";
inline constexpr const char* kNoRod = "Talk to Tam first: he'll lend you a rod.";
inline constexpr const char* kCoveTitle = "Driftwood Cove";
inline constexpr const char* kCastHint = "A: cast your line";
inline constexpr const char* kPutAway = "B: put the rod away";
inline constexpr const char* kWatchBobber = "Watch the bobber... A when it dips right under!";
inline constexpr const char* kReelIn = "A: reel in";
inline constexpr const char* kStrikeNow = "Now! Press A!";
inline constexpr const char* kTooSoon = "Too soon! It was only a nibble, and it swam off.";
inline constexpr const char* kTooSlow = "Too slow... it got away.";
inline constexpr const char* kNothingYet = "Nothing yet. Cast again!";
inline constexpr const char* kReelHint = "Hold A, or crank the reel round with the stylus.\nKeep the line in the green!";
inline constexpr const char* kSnapped = "Snap! The line broke.";
inline constexpr const char* kEscaped = "It slipped off the hook...";
inline constexpr const char* kLandedCatch = "You caught %s!";
inline constexpr const char* kIntoPouch = "Into your pouch: %s";
inline constexpr const char* kPouchFull = "Your pouch is full of those: Tam gives you %lu Gleam instead.";
inline constexpr const char* kCatchWorth = "+%lu Gleam";
inline constexpr const char* kNibbleOf = "%s has a nibble!";
inline constexpr const char* kFishLeft = "The fish are biting: %d more today";
inline constexpr const char* kFishResting = "The fish are resting till tomorrow.";
inline constexpr const char* kFishCaught = "Fish caught: %d    Shells: %d";
inline constexpr const char* kShellFound = "You found %s!";
inline constexpr const char* kTension = "Tension";
inline constexpr const char* kTheFish = "The fish";

// Egg care and hatching (WP7)
inline constexpr const char* kTurn = "Turn";
inline constexpr const char* kListen = "Listen";
inline constexpr const char* kHintEgg = "Rub to warm.  Hold still on it to listen.";
inline constexpr const char* kTurned = "You turn the egg. It hums, snug.";
inline constexpr const char* kTurnedRecently = "Turned not long ago. It's comfy.";
inline constexpr const char* kTurnedPlenty = "It's been turned plenty. Keep it warm.";
inline constexpr const char* kHeartFaint = "A faint, slow flutter. Still very small.";
inline constexpr const char* kHeartScratching = "Scritch, scratch! It wants out.";
// What the heartbeat sounds like, per temperament (Personality order).
inline constexpr const char* kHeartTemperament[6] = {
    "A strong, steady heartbeat. Fearless.",       // Brave
    "A soft, quick patter. A little shy?",         // Shy
    "A skipping, bouncy heartbeat. Playful!",      // Playful
    "A slow, stately thump. Rather grand.",        // Proud
    "A slow, drowsy heartbeat. Zzz...",            // Sleepy
    "A busy heartbeat that keeps changing pace.",  // Curious
};
inline constexpr const char* kItsA = "It's a %s!";   // the hatching: its breed, as it blinks
inline constexpr const char* kItsAn = "It's an %s!";
inline constexpr const char* kHatching = "It's hatching!";
inline constexpr const char* kSkipHint = "A: skip";
inline constexpr const char* kNameHint = "Name your dragon";
inline constexpr const char* kRenameHint = "A new name";
inline constexpr const char* kAnotherName = "Another";
inline constexpr const char* kOk = "OK";
inline constexpr const char* kCancel = "Cancel";
inline constexpr const char* kSayHello = "Say hello to %s!";
inline constexpr const char* kNoBed = "Ready to hatch, but all three beds are taken: send one to the Sanctuary (its profile).";
inline constexpr const char* kBackToSanctuary = "Back to a full den: resting in the Sanctuary.";
inline constexpr const char* kSwitchHint = "X: outing     < > : switch";
inline constexpr const char* kMapHint = "X: outing";
// The world map, the Sanctuary and the Cold Vault (Alpha 2)
inline constexpr const char* kNotOpenYet = "Not open yet.";
inline constexpr const char* kGo = "Go";
inline constexpr const char* kHome = "Home";
inline constexpr const char* kMap = "Map";
inline constexpr const char* kSanctuary = "The Sanctuary";
inline constexpr const char* kVault = "The Cold Vault";
inline constexpr const char* kSanctuaryCount = "%d dragons here (room for %d)";
inline constexpr const char* kSanctuaryOne = "1 dragon here (room for %d)";
inline constexpr const char* kVaultCount = "%d of %d eggs";
inline constexpr const char* kSanctuaryEmpty = "No one here yet. Send a dragon from the den: its profile card (tap the heartglow).";
inline constexpr const char* kVaultEmpty = "No eggs here yet. Send one from the den's egg screen.";
inline constexpr const char* kToTheDen = "To the den";
inline constexpr const char* kIsHome = "%s is home.";
inline constexpr const char* kEggHome = "The egg is in a nest.";
inline constexpr const char* kNestsFull = "Both egg nests are taken.";
inline constexpr const char* kToSanctuary = "Sanctuary";
inline constexpr const char* kToVault = "To the Vault";
inline constexpr const char* kStayHome = "Someone should stay home.";
inline constexpr const char* kSentAway = "%s went to the Sanctuary.";
inline constexpr const char* kEggAway = "The egg went to the Cold Vault.";
inline constexpr const char* kVaultFull = "The Cold Vault is full.";
// The Nesting Stone (Alpha 2 WP3)
inline constexpr const char* kNestingStone = "The Nesting Stone";
inline constexpr const char* kHer = "Her";
inline constexpr const char* kHim = "Him";
inline constexpr const char* kNoneInDen = "None in the den";
inline constexpr const char* kPickAPair = "Choose a female and a male from the den.";
inline constexpr const char* kNestingNow = "%s and %s are nesting: their egg comes tomorrow.";
inline constexpr const char* kNestTogether = "Nest together";
inline constexpr const char* kEggTomorrow = "They settle on the stone. An egg by tomorrow!";
inline constexpr const char* kNewEggNest = "A new egg, warm in the nest!";
inline constexpr const char* kNewEggVault = "A new egg! The nests are full: it's in the Cold Vault.";
// The Wanderings (Alpha 2 WP4)
inline constexpr const char* kWanderings = "The Wanderings";
inline constexpr const char* kWhoComes = "Who comes along?";
inline constexpr const char* kNoOneToWander = "Juveniles and older can come along: there's no one old enough in the den.";
inline constexpr const char* kSetOff = "Set off";
inline constexpr const char* kSetOffHint = "Close your 3DS and walk: your steps are counted.";
inline constexpr const char* kOffWeGo = "Off you go together!";
inline constexpr const char* kOutWandering = "%s is out on the trails.";
inline constexpr const char* kStepsSoFar = "%lu steps so far";
inline constexpr const char* kCallBack = "Call back";
inline constexpr const char* kIsBack = "%s is back!";
inline constexpr const char* kWalked = "%lu steps together";
inline constexpr const char* kFoundGleam = "Gleam: +%lu";
inline constexpr const char* kFoundNothing = "Nothing this time: a longer walk finds more.";
inline constexpr const char* kFoundWildEgg = "A wild %s egg!";
inline constexpr const char* kToTheHoard = "The trinkets go on the hoard.";
inline constexpr const char* kLovely = "Lovely";
// The Market, Gleam and the pouch (Alpha 2 WP5)
inline constexpr const char* kMarket = "The Market";
inline constexpr const char* kNoneLeft = "None left: the Market has more.";
inline constexpr const char* kTabFood = "Food";
inline constexpr const char* kTabSell = "Sell";
inline constexpr const char* kTabEgg = "Egg of the day";
inline constexpr const char* kTapToBuy = "Tap a food to buy one.";
inline constexpr const char* kNotEnoughGleam = "Not enough Gleam.";
inline constexpr const char* kBought = "Into the pouch!";
inline constexpr const char* kTapToSell = "Tap a trinket to sell one.";
inline constexpr const char* kHoardEmpty = "The hoard is empty: the Wanderings find trinkets.";
inline constexpr const char* kSold = "Sold!";
inline constexpr const char* kTodaysEgg = "A %s %s egg";
inline constexpr const char* kBuyEgg = "Buy it";
inline constexpr const char* kEggBought = "Yours! It's in a nest (or the Cold Vault).";
inline constexpr const char* kEggTomorrowMarket = "Today's egg is sold. Another tomorrow!";
inline constexpr const char* kMarketLocked = "Opens when a dragon grows to Juvenile.";
inline constexpr const char* kPrice = "%lu Gleam";
// The profile (WP8)
inline constexpr const char* kTabAbout = "About";
inline constexpr const char* kProfile = "Profile";
inline constexpr const char* kTabFamily = "Family";
inline constexpr const char* kStatWing = "Wing";
inline constexpr const char* kStatWit = "Wit";
inline constexpr const char* kStatSpark = "Spark";
inline constexpr const char* kSweetSpotIs = "Sweet spot: %s";
inline constexpr const char* kSweetSpotUnknown = "Sweet spot: not found yet. Scratch around!";
inline constexpr const char* kFavouriteIs = "Favourite food: %s";
inline constexpr const char* kFavouriteUnknown = "Favourite food: not found yet";
inline constexpr const char* kFoundSweetSpot = "Its sweet spot: %s!";
inline constexpr const char* kMother = "Mother";
inline constexpr const char* kFather = "Father";
inline constexpr const char* kUnknownKin = "Unknown";
inline constexpr const char* kYoungCount = "%d young";
// Things to keep (WP7): the Market's goods
inline constexpr const char* kTabGoods = "Goods";
inline constexpr const char* kBuyFor = "Buy: %lu Gleam";
inline constexpr const char* kPutUp = "Put it up";
inline constexpr const char* kTakeDown = "Take it down";
inline constexpr const char* kInTheDen = "In the den";
inline constexpr const char* kYours = "Yours";
inline constexpr const char* kUpInDen = "Up in the den!";
inline constexpr const char* kBackInChest = "Back in the chest.";
inline constexpr const char* kBoughtThing = "Yours! It's waiting in the den.";
inline constexpr const char* kBoughtKeep = "Yours to keep!";
inline constexpr const char* kTapToPick = "Tap a thing to see it.";
inline constexpr const char* kDenFull = "The den is full (three beds).";
inline constexpr const char* kEggToVault = "The nests are full: it went to the Vault.";
inline constexpr const char* kScreenshotSaved = "Screenshot %s saved.";  // Y, anywhere
inline constexpr const char* kScreenshotFailed = "Couldn't save the screenshot. Is the SD card full?";
// Photo mode (D66)
inline constexpr const char* kPhotoMode = "Photo mode";
inline constexpr const char* kPhotoHint = "The den holds still for the picture.";
inline constexpr const char* kSnap = "Snap (A)";
inline constexpr const char* kPhotoClose = "Closer (X)";
inline constexpr const char* kPhotoWide = "The den (X)";
inline constexpr const char* kPhotoSaved = "Photo %s saved to the SD card.";
inline constexpr const char* kPhotoFailed = "Couldn't save the photo. Is the SD card full?";

// Title, system menu and settings (WP10)
inline constexpr const char* kContinue = "Continue";
inline constexpr const char* kNewGame = "New game";
inline constexpr const char* kStartOverAsk = "Start a new game?";
inline constexpr const char* kStartOverBody = "%s would be gone for good.";
inline constexpr const char* kKeepPlaying = "Keep playing";
inline constexpr const char* kStartOver = "Start over";
inline constexpr const char* kReally = "Really? This can't be undone.";
inline constexpr const char* kNo = "No";
inline constexpr const char* kYesStartOver = "Yes, start over";
inline constexpr const char* kYourNameHint = "Your name, keeper";
inline constexpr const char* kBack = "Back";
inline constexpr const char* kResume = "Resume";
inline constexpr const char* kSettings = "Settings";
inline constexpr const char* kYourLook = "Your look";
inline constexpr const char* kLookClothes = "Clothes";
inline constexpr const char* kLookHair = "Hair";
inline constexpr const char* kLookHairColour = "Hair colour";
inline constexpr const char* kLookSkin = "Skin";
inline constexpr const char* kLookOutfit = "Outfit";
inline constexpr const char* kLookEyes = "Eyes";
inline constexpr const char* kLookHelp = "Up/Down: pick a row\nLeft/Right: change it\nL/R: turn around";
inline constexpr const char* kSaveQuit = "Save & quit";
inline constexpr const char* kQuit = "Quit";
inline constexpr const char* kMusic = "Music";
inline constexpr const char* kSounds = "Sounds";
inline constexpr const char* kClockNote1 = "Time in the den follows your 3DS clock:";
inline constexpr const char* kClockNote2 = "your dragon grows while you're away.";
inline constexpr const char* kDeleteSave = "Delete save";
inline constexpr const char* kDeleteAsk = "Delete your save?";
inline constexpr const char* kDeleteBody = "Your dragon and everything with it will be gone.";
inline constexpr const char* kKeepIt = "Keep it";
inline constexpr const char* kDelete = "Delete";
inline constexpr const char* kDeleteSure = "Are you sure? This can't be undone.";
inline constexpr const char* kYesDelete = "Yes, delete";
inline constexpr const char* kDeleted = "Save deleted.";
inline constexpr const char* kKeeper = "Keeper";
// The Dragondex (WP12)
inline constexpr const char* kDex = "Dragondex";
inline constexpr const char* kDexUnknown = "Not met yet";
inline constexpr const char* kDexBreedLine = "%s: %d of %d colourings";
inline constexpr const char* kDexDone = "Complete! Its banner is yours.";
inline constexpr const char* kDexHang = "Hang banner";
inline constexpr const char* kDexRares = "Rare colourings";
inline constexpr const char* kDexTakeDown = "Take it down";
inline constexpr const char* kDexNew = "New in the Dragondex: %s";
inline constexpr const char* kDexRare = "A rare colouring for the Dragondex: %s!";
inline constexpr const char* kDexComplete = "Every %s colouring found! +150 Gleam and a banner";

// ---- 1.0, the interface (workstream U): the needs, the profile's training and record, the
// Journal's tracked goal, the settings, the Market's and the Wanderings' top screens.
inline constexpr const char* kLove = "Love";
inline constexpr const char* kTired = "Tired";
// Settings
inline constexpr const char* kStereo3d = "3D";
inline constexpr const char* kVoices = "Voices";
inline constexpr const char* kOn = "On";
inline constexpr const char* kOff = "Off";
inline constexpr const char* kToggleIs = "%s: %s";
inline constexpr const char* kTipsAgain = "Show tips again";
inline constexpr const char* kTipsReset = "The tips will show again as you play.";
inline constexpr const char* kClockNote = "Time follows your 3DS clock: your dragon grows while you're away.";
// The profile: About, Training, Record, Family
inline constexpr const char* kTabTraining = "Training";
inline constexpr const char* kTabRecord = "Record";
inline constexpr const char* kLevel = "Level %d";
inline constexpr const char* kXpToNext = "%lu / %lu to the next";
inline constexpr const char* kXpTop = "The highest level!";
inline constexpr const char* kStatNames[5] = {"Wing", "Wit", "Might", "Breath", "Stamina"};
inline constexpr const char* kMoves = "Moves";
inline constexpr const char* kNoMove = "-";
inline constexpr const char* kNoMovesYet = "No moves learned yet: battles teach them.";
inline constexpr const char* kPickMove = "Swap in which move?";
inline constexpr const char* kMoveSwapped = "%s learned it by heart.";
inline constexpr const char* kPower = "Pow %d";
inline constexpr const char* kStatusMove = "Status";
inline constexpr const char* kWears = "Wears";
inline constexpr const char* kWearNothing = "Nothing yet";
inline constexpr const char* kDye = "Dye: %s";
inline constexpr const char* kDyeNatural = "Its own colours";
inline constexpr const char* kDressUp = "Dress up";
inline constexpr const char* kWardrobeSoon = "The wardrobe is at Moonpetal Glade.";
inline constexpr const char* kTitles = "Titles";
inline constexpr const char* kNoTitles = "No titles yet";
inline constexpr const char* kBattleWins = "Battles won";
inline constexpr const char* kShowWins = "Shows won";
inline constexpr const char* kWildWins = "Wild dragons";
inline constexpr const char* kCupsWon = "Cups";
inline constexpr const char* kRibbons = "Ribbons";
inline constexpr const char* kHollowDeepest = "Hollow floor";
inline constexpr const char* kCupNames[4] = {"Ember", "Flame", "Blaze", "Starfire"};
inline constexpr const char* kChallengeShort[3] = {"Fruit Catch", "Sky Rings", "Lantern Trial"};
inline constexpr const char* kFullProfile = "Full profile";
// The Journal's tracked goal
inline constexpr const char* kTrack = "Track";
inline constexpr const char* kTracking = "Tracking";
inline constexpr const char* kNowTracking = "Now tracking: %s";
inline constexpr const char* kTrackHint = "Tap a goal to track it:\nthe map shows the way.";
inline constexpr const char* kTrackPlaceHint = "Tap a place to track it on the map.";
inline constexpr const char* kGoalBattle = "The battle league";
inline constexpr const char* kGoalShow = "The pageant";
inline constexpr const char* kGoalHollow = "Frostspire Hollow";
inline constexpr const char* kGoalBattleStep = "%s league: beat its challengers";
inline constexpr const char* kGoalShowStep = "%s league: win its shows at the glade";
inline constexpr const char* kGoalHollowStep = "Deepest floor so far: %d";
inline constexpr const char* kGoalHollowNew = "Wild dragons wait, floor after floor";
inline constexpr const char* kGoalPlaceStep = "Head for %s";
// The Market's top screen
inline constexpr const char* kHoardCount = "%d trinkets in the hoard";
inline constexpr const char* kStallFoodCap = "Fresh food for the pouch";
inline constexpr const char* kPouchHolds = "%d in your pouch";
inline constexpr const char* kTradeCap = "Maple buys trinkets for Gleam";
inline constexpr const char* kEggRarityPrice = "%s  -  %lu Gleam";
inline constexpr const char* kGoodsCap = "New things on the stall each day";
// The Wanderings' top screen
inline constexpr const char* kTrailSign = "Trails";
inline constexpr const char* kReadyToGo = "%s is ready to go!";
inline constexpr const char* kTrailStart = "Just setting off";
inline constexpr const char* kTrailPond = "Past the lily pond";
inline constexpr const char* kTrailCopse = "Through the old copse";
inline constexpr const char* kTrailRidge = "Up on the ridge";
inline constexpr const char* kTrailFar = "Far along the long trail";
inline constexpr const char* kStepsOut = "%lu steps";
inline constexpr const char* kFindsHint = "It sniffs for treasure every %d steps.";
inline constexpr const char* kBackWith = "%s is back with treasure!";
inline constexpr const char* kBackEmpty = "%s is back, happy and muddy.";
// ---- The pageant, accessories and dyes (1.0, D90: Moonpetal Glade, app/glade*.cpp, app/scene_wardrobe.cpp)
inline constexpr const char* kWardrobe = "Wardrobe";
inline constexpr const char* kSlotHead = "Head";
inline constexpr const char* kSlotNeck = "Neck";
inline constexpr const char* kSlotBack = "Back";
inline constexpr const char* kSlotTail = "Tail";
inline constexpr const char* kWardrobeDye = "Dye";
inline constexpr const char* kWardrobeNone = "None";
inline constexpr const char* kWardrobeNatural = "Natural";
inline constexpr const char* kWardrobeEmpty = "Nothing for its %s yet: the stalls at Moonpetal Glade sell pretty things.";
inline constexpr const char* kWardrobeEgg = "An egg has nothing to wear yet.";
inline constexpr const char* kWardrobeHelp = "Tap to try on. L/R: slots. Pad: turn.";
inline constexpr const char* kWardrobeDyed = "Dyed %s.";
// The glade's people (the prompt's %s is their name)
inline constexpr const char* kHostName = "Celestine";
inline constexpr const char* kHostTitle = "The pageant's host";
inline constexpr const char* kMilliner = "Linnet";
inline constexpr const char* kMillinerTitle = "Accessories";
inline constexpr const char* kDyer = "Madder";
inline constexpr const char* kDyerTitle = "Dyes";
inline constexpr const char* kJudgeNames[3] = {"Plume", "Wick", "Tansy"};
inline constexpr const char* kPromptPageant = "The pageant: talk to %s";
inline constexpr const char* kPromptStall = "Browse %s's stall";
inline constexpr const char* kHostHello[3] = {
    "Welcome to Moonpetal Glade, {P}! I'm Celestine, and this is where dragons shine.",
    "Every show has a theme. Dress {D} to suit it: the judges score Look, Poise and Performance.",
    "Win all four shows of a league for its title. The themes change every day!",
};
inline constexpr const char* kHostAgain = "Which show will you and {D} enter tonight?";
inline constexpr const char* kMillinerHello = "Hats, bows, capes and charms! Tap one to see it on {D}.";
inline constexpr const char* kDyerHello = "A dip in my dyes and {D} will be the talk of the glade.";
// The board
inline constexpr const char* kBoardTitle = "The Pageant";
inline constexpr const char* kBoardLeague = "%s League";
inline constexpr const char* kBoardShowsWon = "%d of 4 shows won";
inline constexpr const char* kBoardLeagueWon = "League won!";
inline constexpr const char* kBoardEnter = "Enter";
inline constexpr const char* kBoardWon = "Won";
inline constexpr const char* kBoardPaid = "Today's prize won";
inline constexpr const char* kBoardLocked = "Win the %s League first.";
inline constexpr const char* kBoardTired = "%s is too tired for a show. A good sleep first!";
inline constexpr const char* kBoardAlone = "Bring a dragon along to show!";
inline constexpr const char* kBoardFavours = "Favours %s dragons; %s things.";
inline constexpr const char* kBoardYourLook = "Look %d  Poise %d";
// The show
inline constexpr const char* kShowWelcome = "Welcome, one and all, to the %s!";
inline constexpr const char* kShowRivals = "Tonight: %.15s and %.15s, %.15s and %.15s, and {P} with {D}!";
inline constexpr const char* kShowLookLine = "First, the Look! How do they suit the %s?";
inline constexpr const char* kShowPoiseLine = "Now, Poise: how they carry themselves.";
inline constexpr const char* kShowPerfLine = "And the Performance! {P}, cue {D}'s tricks in time with the music.";
inline constexpr const char* kShowRound = "%s";
inline constexpr const char* kShowCueHelp = "Press each cue as it reaches the ring!";
inline constexpr const char* kShowTap = "Tap!";
inline constexpr const char* kShowPerfect = "Perfect!";
inline constexpr const char* kShowGood = "Good";
inline constexpr const char* kShowMiss = "Miss";
inline constexpr const char* kShowRivalsPerform = "The rivals perform...";
inline constexpr const char* kShowResults = "The results";
inline constexpr const char* kShowPlaces[4] = {"1st", "2nd", "3rd", "4th"};
inline constexpr const char* kShowWinner = "%s wins the %s!";
inline constexpr const char* kShowPlaced = "%s came %s. Well shown!";
inline constexpr const char* kShowRibbon = "A ribbon for the %s!";
inline constexpr const char* kShowGleam = "+%lu Gleam";
inline constexpr const char* kShowPrize = "A prize: the %s!";
inline constexpr const char* kShowDyePrize = "And a dye: %s!";
inline constexpr const char* kShowTitle = "%s is now the %s!";
inline constexpr const char* kShowPaidAlready = "(Today's prize for this show is already won.)";
inline constexpr const char* kShowContinue = "Continue";
inline constexpr const char* kShowPoints = "%.1f";
inline constexpr const char* kShowTotal = "Total";
// The stalls
inline constexpr const char* kStallAccessories = "Linnet's Finery";
inline constexpr const char* kStallDyes = "Madder's Dyes";
inline constexpr const char* kStallOwned = "Yours";
inline constexpr const char* kStallBoughtThing = "Bought! Put it on in the wardrobe.";
inline constexpr const char* kStallBoughtDye = "Bought! Dye in the wardrobe.";
inline constexpr const char* kStallPrize = "Won at the shows";
inline constexpr const char* kGladeStallTomorrow = "New things tomorrow.";
inline constexpr const char* kStallStyles = "Styles: %s";
// ---- 1.0: battles, the league and Frostspire Hollow (workstream B: app/battle_view.cpp,
// feature_league.cpp, feature_hollow.cpp)
inline constexpr const char* kPromptBattle = "A: battle %s";
inline constexpr const char* kPromptRematch = "A: rematch %s";
inline constexpr const char* kPromptRead = "A: read %s";
inline constexpr const char* kLeagueBoardName = "the league board";
inline constexpr const char* kBattleUsed = "%s used %s!";
inline constexpr const char* kBattleDodged = "%s dodged it!";
inline constexpr const char* kBattleMissed = "It missed!";
inline constexpr const char* kBattleCrit = "A critical hit!";
inline constexpr const char* kBattleStrong = "It's super effective!";
inline constexpr const char* kBattleWeak = "It's not very effective...";
inline constexpr const char* kBattleRose = "%s's %s rose!";
inline constexpr const char* kBattleRoseSharply = "%s's %s rose sharply!";
inline constexpr const char* kBattleFell = "%s's %s fell!";
inline constexpr const char* kBattleNothing = "Nothing more happened.";
inline constexpr const char* kBattleHealed = "%s caught its breath!";
inline constexpr const char* kBattleTiredOut = "%s is tired out!";
inline constexpr const char* kBattleTimeUp = "Both are worn out! %s has more left.";
inline constexpr const char* kBattleStageNames[5] = {"Might", "Breath", "Wit", "Wing", "guard"};
inline constexpr const char* kBattleStatNames[5] = {"Wing", "Wit", "Might", "Breath", "Stamina"};
inline constexpr const char* kBattleWhatWill = "What will %s do?";
inline constexpr const char* kBattleWatching = "...";
inline constexpr const char* kBattleFaster = "A: faster";
inline constexpr const char* kBattleWon = "You won!";
inline constexpr const char* kBattleLost = "Not this time...";
inline constexpr const char* kBattleGaveUpTitle = "You called it off.";
inline constexpr const char* kBattleGiveUp = "Give up";
inline constexpr const char* kBattleGiveUpAsk = "Give up this battle?";
inline constexpr const char* kBattleYes = "Yes";
inline constexpr const char* kBattleNo = "Keep going";
inline constexpr const char* kBattleContinue = "Continue";
inline constexpr const char* kBattleWantsTo = "%s wants to battle!";
inline constexpr const char* kBattleWildAppears = "A wild %s comes out!";
inline constexpr const char* kBattleGuardian = "The Hollow's guardian!";
inline constexpr const char* kBattleWildName = "the wild %s";
inline constexpr const char* kBattleNeedsBreather = "Needs a breather";
inline constexpr const char* kBattleUsedUp = "Used";
inline constexpr const char* kBattleStrongTag = "Strong!";
inline constexpr const char* kBattleWeakTag = "Weak";
inline constexpr const char* kBattlePowerAcc = "%s  Power %d  Acc %d";
inline constexpr const char* kBattleHelp = "Tap a move, or the D-pad and A";
inline constexpr const char* kBattleXp = "%s gained %lu exp";
inline constexpr const char* kBattleLevelUp = "%s grew to level %d!";
inline constexpr const char* kBattleLearned = "%s learned %s!";
inline constexpr const char* kBattleLearnedSwap = "Learned %s: swap it in from the profile";
inline constexpr const char* kBattleGleam = "+%lu Gleam";
inline constexpr const char* kBattlePaid = "(today's prize was won already)";
inline constexpr const char* kBattleTitle = "%s earned a title: %s!";
inline constexpr const char* kBattlePrize = "Prize: %d %s and a %s";
inline constexpr const char* kBattleChampionOpen = "The champion waits at Emberpeak Caldera!";
inline constexpr const char* kBattleNextLeague = "The %s league arrives in the valley!";
inline constexpr const char* kBattleAllWon = "Every league is yours!";
inline constexpr const char* kBattleTooTired = "{D} looks tired. Let it rest, then come back and we'll battle!";
inline constexpr const char* kBattleNoPartner = "Come back with a dragon, and we'll battle!";
inline constexpr const char* kBattleNoRoom = "There's no room to battle here.";
inline constexpr const char* kBattleAskTitle = "Battle %s?";
inline constexpr const char* kBattleAskGo = "Battle!";
inline constexpr const char* kBattleAskNot = "Not now";
inline constexpr const char* kBattleAskTheirs = "%s: %s, level %d";
inline constexpr const char* kBattleAskYours = "%s: level %d";
inline constexpr const char* kBattleAskEnergy = "Energy %d (a battle uses %d)";
inline constexpr const char* kChampionWaits = "Beat my league's four challengers first. Then come and find me here, at the ring.";
inline constexpr const char* kLeagueBoardTitle = "The %s league";
inline constexpr const char* kBoardBeaten = "Beaten";
inline constexpr const char* kBoardAt = "at %s";
inline constexpr const char* kLeagueBoardLocked = "Beat the four to meet the champion";
inline constexpr const char* kBoardOpen = "The champion waits at the caldera";
inline constexpr const char* kLeagueBoardWon = "League won";
inline constexpr const char* kBoardTrack = "Track";
inline constexpr const char* kBoardTracking = "Tracking";
inline constexpr const char* kBoardWheel = "Each beats the next:";
inline constexpr const char* kBoardPair = "Lumen and Shade: each beats the other";
inline constexpr const char* kBoardTitles = "Won: %d of 4 leagues";
// Frostspire Hollow's keeper and floors
inline constexpr const char* kHollowKeeper = "Tove";
inline constexpr const char* kHollowKeeperTitle = "keeps Frostspire Hollow";
inline constexpr const char* kPromptHollow = "A: talk to %s";
inline constexpr const char* kHollowFirst[3] = {
    "Welcome to Frostspire Hollow, keeper. I'm Tove. Wild dragons live in the caves behind me.",
    "Each floor, one comes out to test you. The deeper you go, the stronger they are.",
    "Every fifth floor there's a guardian. Beat one and you can start from there next time."};
inline constexpr const char* kHollowAgain[3] = {"Back for more training? The caves are waiting.",
                                                "The cold makes a dragon strong. Down you go?",
                                                "{D} looks ready. Mind the guardians."};
inline constexpr const char* kHollowTitle = "Frostspire Hollow";
inline constexpr const char* kHollowFloor = "Floor %d";
inline constexpr const char* kHollowFloorOf = "Floor %d of %d";
inline constexpr const char* kHollowPick = "Where will you start?";
inline constexpr const char* kHollowDeepestOf = "%s's deepest: floor %d";
inline constexpr const char* kHollowAllDeepest = "Deepest of all your dragons: floor %d";
inline constexpr const char* kHollowGoDeeper = "Deeper!";
inline constexpr const char* kHollowLeave = "Leave";
inline constexpr const char* kHollowCleared = "Floor %d cleared!";
inline constexpr const char* kHollowCheckpoint = "Checkpoint: next time, start at floor %d";
inline constexpr const char* kHollowTrained = "%s's %s grew by a point!";
inline constexpr const char* kHollowBottom = "The very bottom of the Hollow!";
inline constexpr const char* kHollowTooTired = "{D} is too tired to go deeper. Rest, and come back tomorrow.";
inline constexpr const char* kHollowLost = "Well fought, both of you. The cold makes you stronger: come back soon.";
inline constexpr const char* kHollowLeft = "Good training today. The caves will be here.";
inline constexpr const char* kHollowNext = "Next: floor %d";
inline constexpr const char* kHollowDone = "Back to Tove";

// The roaming trainers and their friendly duels (workstream D; their own lines are core/roamers')
inline constexpr const char* kPromptDuel = "A: duel %s";
inline constexpr const char* kDuelWants = "%s wants a friendly duel!";
inline constexpr const char* kDuelAskTitle = "Duel %s?";
inline constexpr const char* kDuelAskGo = "Duel!";
inline constexpr const char* kDuelFair = "A fair fight: matched to your partner";
inline constexpr const char* kDuelPrize = "Their first loss today pays %lu Gleam";
inline constexpr const char* kDuelPrizeTaken = "Today's Gleam from them is won already";
inline constexpr const char* kDuelNoPartner = "Oh, no dragon with you? Come back with one and we'll have a duel!";
inline constexpr const char* kDuelTooTired = "{D} looks worn out. Rest up, and we'll duel another time!";
inline constexpr const char* kDuelsWonLine = "Friendly duels won: %d";
inline constexpr const char* kDuelsWon = "Your duels won: %d";
inline constexpr const char* kDuelWatching = "Watching %s";

}  // namespace ec::str
