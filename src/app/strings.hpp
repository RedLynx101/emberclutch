// Every player-facing string (D30: English only for v1; a translation adds a table here).
#pragma once

namespace ec::str {

inline constexpr const char* kGameTitle = "Emberclutch";
inline constexpr const char* kTagline = "raise, breed and fly with dragons";
inline constexpr const char* kTouchToBegin = "Touch to begin";
inline constexpr const char* kBuildLabel = "Beta 1";

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
inline constexpr const char* kOutingRide = "%s is grown: ride it, fly, land and walk about.";
inline constexpr const char* kOutingLead = "%s walks beside you on its lead, and follows you anywhere.";
inline constexpr const char* kMakePartner = "Make %s your partner";
inline constexpr const char* kHeadOut = "Head out together";
inline constexpr const char* kJournal = "Journal";
inline constexpr const char* kJournalQuests = "Quests";
inline constexpr const char* kJournalEgg = "The egg";
inline constexpr const char* kJournalPlaces = "Places";
inline constexpr const char* kQuestDone = "Done!";
inline constexpr const char* kNoQuests = "No quests yet. Head out into the valley.";
inline constexpr const char* kPlacesFound = "Places found: %d of %d";
inline constexpr const char* kDenTitle = "Your den";
inline constexpr const char* kDecorSpotNames[5] = {"Rug", "Lantern", "Perch", "Plant", "Banner"};
inline constexpr const char* kDecorNone = "(bare)";
inline constexpr const char* kDenHint = "New things turn up at the Market's stall each day.";
// The valley (Beta)
inline constexpr const char* kFoundPlace = "Found: %s";
inline constexpr const char* kStarEgg = "A star-born %s egg is yours!";
inline constexpr const char* kPromptTalk = "A: talk to %s";
inline constexpr const char* kFoundStray = "%s sniffed out the stray! She's safe.";
inline constexpr const char* kStallEmpty = "Sold for today";
inline constexpr const char* kStallTomorrow = "New things on the stall tomorrow.";
inline constexpr const char* kLanternLit = "This lantern is burning bright.";
inline constexpr const char* kLanternFestival = "The great lantern waits for the festival night.";
inline constexpr const char* kPartnerCame = "%s came running!";
inline constexpr const char* kLanternLitAt = "A lantern lit: %s";
inline constexpr const char* kQuestFinished = "Quest done: %s";
inline constexpr const char* kPromptEnter = "A: go into %s";
inline constexpr const char* kPromptLight = "A: light the lantern";
inline constexpr const char* kPromptRide = "A: climb on and ride";
inline constexpr const char* kPromptCall = "A: call your partner";
inline constexpr const char* kTravelledTo = "Off to %s";
inline constexpr const char* kRidingGround = "Riding (on the ground)";
inline constexpr const char* kFlying = "Flying";
inline constexpr const char* kFreeCamera = "Looking about";
inline constexpr const char* kOnFoot = "On foot";
inline constexpr const char* kPlacesAndLanterns = "Places %d/%d  Lanterns %d/%d";
inline constexpr const char* kHelpFoot = "Pad: walk  B: run\nL/R: turn the view\nA: what's near\nTap a pin to travel";
inline constexpr const char* kHelpRideGround = "Pad: walk  B: run\nA: take off\nDown: get off";
inline constexpr const char* kHelpFly = "Pad: steer  A: flap\nB: dive  L/R: bank\nLet go: glide";
inline constexpr const char* kHelpFreeCam = "Pad: move  L/R: turn\nD-pad: up, down, tilt\nB: faster  A: back";
inline constexpr const char* kLook = "Look";
inline constexpr const char* kCall = "Call";
inline constexpr const char* kHomeX = "Home (X)";
inline constexpr const char* kIntoBowl = "Into the bowl.";
inline constexpr const char* kBowlFull = "The bowl is full.";
inline constexpr const char* kTreat = "A treat!";
inline constexpr const char* kOrbEmpty = "The orb is empty for now.";
inline constexpr const char* kProfileClose = "Close";
inline constexpr const char* kRename = "Rename";
inline constexpr const char* kPersonality = "Personality";
inline constexpr const char* kRenamed = "A fine new name.";

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
inline constexpr const char* kCupPlaced = "So close! A prize for placing.";
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
inline constexpr const char* kHelpRings = "Pad: steer  A: flap\nB: dive  L/R: bank\nThrough every ring!";
inline constexpr const char* kHelpPicker = "L/R: challenge\nLeft/Right: cup\nA: start  B: leave";
// The hosts: Wren at the arena, Maple at the orchard ({D} your dragon, {P} you).
inline constexpr const char* kHostRings[] = {"Sky Rings! Fly through every ring in order. A missed ring costs three seconds.",
                                              "Beat the clock and the cup is yours. Your best run flies along as a little wisp!"};
inline constexpr const char* kHostLanterns[] = {"The Lantern Trial! The crystal lanterns will light up in a pattern.",
                                                 "Then tap them in the same order, and {D} will breathe each one alight."};
inline constexpr const char* kHostFruit[] = {"Fruit Catch! Flick a fruit down the meadow and {D} will run for it.",
                                              "Longer throws score more, fancy catches extra, and golden pears count double!"};
inline constexpr const char* kHostWon[] = {"Wonderful! What a pair you two are.", "That's the cup! The crowd loves you.",
                                            "Now that's how it's done!"};
inline constexpr const char* kHostPlaced[] = {"So close! Next time, I'm sure of it.", "Nearly! Have another go."};
inline constexpr const char* kHostTry[] = {"Don't worry, everyone starts somewhere.", "A little practice, and you'll get it!"};

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
inline constexpr const char* kSwitchHint = "X: map     < > : switch";
inline constexpr const char* kMapHint = "X: map";
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

}  // namespace ec::str
