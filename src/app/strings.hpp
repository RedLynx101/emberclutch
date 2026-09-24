// Every player-facing string (D30: English only for v1; a translation adds a table here).
#pragma once

namespace ec::str {

inline constexpr const char* kGameTitle = "Emberclutch";
inline constexpr const char* kTagline = "raise, breed and fly with dragons";
inline constexpr const char* kTouchToBegin = "Touch to begin";
inline constexpr const char* kBuildLabel = "Alpha 1 in development";

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
inline constexpr const char* kShine = "Shine";
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
inline constexpr const char* kGroomed = "Scales polished to a shine.";
inline constexpr const char* kPlayed = "Fetch! It bounds after the ball.";
inline constexpr const char* kMadeUp = "Its heartglow lights up again.";
// Hands-on care (WP7)
inline constexpr const char* kSweetSpot = "It loves that spot!";
inline constexpr const char* kRefused = "Not that one... it turns its nose up.";
inline constexpr const char* kGleaming = "Gleaming from nose to tail!";
inline constexpr const char* kComeHere = "Come here!";
inline constexpr const char* kRinse = "Rinse";
inline constexpr const char* kHintPet = "Stroke it. Hold still off it to call.";
inline constexpr const char* kHintFood = "Pick a food, hold it to its mouth.";
inline constexpr const char* kHintBrush = "Brush head to tail.  L / R: turn.";
inline constexpr const char* kHintCloth = "Polish in little circles.";
inline constexpr const char* kHintBath = "Rub in the suds, then rinse.";
inline constexpr const char* kHintBall = "Flick to throw, drag to roll.";
inline constexpr const char* kSplash = "Splash! Squeaky clean.";
inline constexpr const char* kProfileClose = "Close";
inline constexpr const char* kRename = "Rename";
inline constexpr const char* kPersonality = "Personality";
inline constexpr const char* kRenamed = "A fine new name.";

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
inline constexpr const char* kHatching = "It's hatching!";
inline constexpr const char* kSkipHint = "A: skip";
inline constexpr const char* kNameHint = "Name your dragon";
inline constexpr const char* kRenameHint = "A new name";
inline constexpr const char* kAnotherName = "Another";
inline constexpr const char* kOk = "OK";
inline constexpr const char* kCancel = "Cancel";
inline constexpr const char* kSayHello = "Say hello to %s!";

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

}  // namespace ec::str
