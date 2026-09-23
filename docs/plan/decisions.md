# Decision Log

Approved decisions, newest first. "Approved" means the project owner signed off.

## 2026-09-23 — Project kickoff

| # | Decision | Why | Status |
|---|---|---|---|
| D1 | The first creature is **dragons** (not aquarium, horses or pegasi) | Growth stages, flight-based competitions without riding, a stylized look that suits old-3DS limits, and a wing rig that carries over to pegasi and alicorns | Approved |
| D2 | Name: **Emberclutch** | Warm (ember) + eggs and breeding (clutch). No existing game found with this name | Approved |
| D3 | **Old 3DS is the performance floor**. Native C++17, libctru, citro3d (top screen) and citro2d (bottom screen) | Same stack as 3D-Claw and asteria-ds, and the toolchain is already installed | Approved |
| D4 | **Riding allowed anywhere in free roam**, never in training or competitions. Events are split between flying and grounded | Owner direction | Approved |
| D5 | Egg → Adult in **about 2 weeks**, gated by real days (3DS clock) **and** care stars | Owner direction: growth driven by needs met over time | Approved |
| D6 | **Forgiving but upset**: no death, running away or regression. Neglect makes the dragon Upset until you make up | Owner direction | Approved |
| D7 | Own **many dragons of any breed**: den (3 active + 2 egg nests), off-map **Sanctuary**, egg **Cold Vault** | Owner direction; the 3-dragon cap in the den comes from old-3DS performance | Approved |
| D8 | **Breeding with lots of variants**: Mendelian element alleles (6 base breeds + 15 hybrids) plus inherited parts, colors and rare traits | Owner direction: creative but not overly complex | Proposed, awaiting review |
| D9 | Visual theme: **warm light inside**. A heart-shaped heartglow is the signature and the mood indicator. Babies super cute, adults majestic | Owner direction ("super cute", "majestic") | Proposed, awaiting review |
| D10 | Voice commands are **on-device and optional**, with cue buttons always available | Accessibility, public release, no cloud | Approved (from suggestions) |
| D11 | Species data has `bodyPlan` + `modules` fields from day one, so **the equine line and alicorns** fit later | Owner wants alicorns planned | Approved |
| D12 | **Open source**, starting private. Code MIT, original art CC BY-SA 4.0, music under its own notice | Owner direction; Suno terms don't allow relicensing the music | Approved |
| D13 | Deploy over **Wi-Fi** (3dslink / ftpd), not by pulling the SD card | Owner direction | Approved |
| D14 | Concept art is AI-generated **reference only** and is not shipped | Keeps the open-source asset licensing clean | Approved |
| D15 | Music is made in **Suno**, then looped, blended, loudness-matched and compressed to Ogg by `tools/audio/make_loop.py` | Owner direction | Approved |
| D16 | Skip StreetPass, amiibo, face tracking and online play | Not practical for homebrew on an old 3DS | Approved |
