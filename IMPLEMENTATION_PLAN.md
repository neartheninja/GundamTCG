# OnePieceTCG_V2 Implementation Plan

**Last Updated**: 2025-11-13
**Status**: Phase 1 - Foundation
**Current Version**: 0.1.0-alpha

---

## 🎯 Project Vision

Build a fully-featured digital One Piece Trading Card Game with:
- **Core Gameplay**: Networked multiplayer matches with authentic TCG rules
- **Single-Player**: Tutorial mode, AI opponents, deck builder
- **Collection**: Card pack opening and inventory system
- **Future**: Potential multi-TCG support (far future)

---

## 📊 Milestone Overview

### **Milestone 1: Core Game Engine** (Phases 1-8)
**Goal**: 2-player networked matches work with 50+ cards
**Duration**: 8-12 weeks
**Status**: 🔴 Not Started

### **Milestone 2: Single-Player Features** (Phases 9-12)
**Goal**: Tutorial, AI opponent, deck builder operational
**Duration**: 4-6 weeks
**Status**: ⚫ Blocked by Milestone 1

### **Milestone 3: Collection System** (Phases 13-14)
**Goal**: Pack opening, card inventory, full collection management
**Duration**: 3-4 weeks
**Status**: ⚫ Blocked by Milestone 2

### **Milestone 4: Polish & Content** (Phases 15-16)
**Goal**: All cards imported, matchmaking, ranked play
**Duration**: 6-8 weeks
**Status**: ⚫ Blocked by Milestone 3

### **Milestone 5: Multi-TCG (Optional Future)**
**Goal**: Platform supports multiple TCG rulesets
**Status**: 🔵 Long-term consideration

---

## 📋 Detailed Phase Breakdown

---

### **PHASE 1: Canonical Data Model** 🟡 IN PROGRESS (C++ Complete, Editor Pending)

**Goal**: One source of truth for UI and rules engine

#### Tasks
- [x] Review existing FCardData structure
- [x] Design FCardDefinition struct (static card data)
  - Identity: CardID, CardName, CardType, Colors (array), Attributes (array), Types (array)
  - Stats: Cost, Power, Life, Counter
  - Presentation: CardText, CardArt (TSoftObjectPtr)
  - Keywords: Set of FName (Blocker, Rush, DoubleAttack, Banish)
  - Flags: HasTrigger
  - Effects: TArray of effect IDs/references + InlineEffects array
  - Helper methods: HasKeyword(), GetPrimaryColor(), IsMulticolor()
- [x] Design FCardInstance struct (runtime state)
  - InstanceID, CardDefinitionID reference
  - CurrentZone, bIsRested, AttachedDonCount, OwnerPlayerID
  - Runtime modifiers array
  - Helper methods: GetTotalPower(), GetTotalCost()
- [x] Design FEffectRow struct (minimal for MVP)
  - Timing, Conditions, Costs, Operations (all FName arrays)
  - Description field for UI tooltips
- [x] Design FActiveModifier struct
  - ModifierType, Amount, Duration, SourceInstanceID
- [x] Design EModifierDuration enum
  - UntilEndOfTurn, UntilEndOfBattle, WhileInPlay, Permanent
- [x] Update TCGTypes.h with all new structs
- [x] Build test data CSV: Cards_Test with 8 real cards:
  1. Vanilla: Roronoa Zoro (ST01-013) ✅
  2. Counter Character: Nami (ST01-007) ✅
  3. Blocker: Jinbe (OP01-068) ✅
  4. Event with [Counter]: Gum-Gum Red Roc (OP01-029) ✅
  5. OnPlay Effect: Trafalgar Law (OP01-047) ✅
  6. Activate Main: Nico Robin (OP01-067) ✅
  7. Trigger Card: Charlotte Katakuri (OP03-123) ✅
  8. Leader: Monkey.D.Luffy (ST01-012) ✅
- [ ] **[REQUIRES EDITOR]** Create S_CardDefinition struct asset in Content/
- [ ] **[REQUIRES EDITOR]** Create DT_Cards_Test DataTable from Cards_Test.csv
- [ ] **[REQUIRES EDITOR]** Verify card art loading

#### Definition of Done ✅
- [x] C++ structures defined and syntactically correct ✅
- [x] Test card CSV created with 8 real cards ✅
- [x] No compile errors or warnings ✅ (verified by syntax check)
- [ ] **[PENDING EDITOR]** Every test card loads name, power, art from table in Unreal Editor
- [ ] **[PENDING EDITOR]** Row Name equals CardID for direct lookup
- [ ] **[PENDING EDITOR]** Blueprint can reference S_CardDefinition table

**Progress**: 75% complete (C++ done, Blueprint/Editor work remaining)

#### Files Modified
- `Source/OnePieceTCG_V2/TCGTypes.h` ✅ (added ~280 lines)
- `Content/Cards/Data/Cards_Test.csv` ✅ (updated with 8 test cards)
- `IMPLEMENTATION_PLAN.md` ✅ (this file)
- `CHANGELOG.md` ✅ (version 0.1.1-alpha)
- `Content/Cards/Data/S_CardDefinition.uasset` (pending - requires editor)
- `Content/Cards/Data/DT_Cards_Test.uasset` (pending - requires editor)

---

### **PHASE 2: WBP_TCG_Card Reads from Table** 🟡 IN PROGRESS (50% C++ Complete)

**Goal**: Data-driven card display with fallbacks

#### Tasks
- [x] Update TCGHandWidget.cpp SetCardData flow:
  - [x] Get DataTable row by CardID
  - [x] Branch: True = use row data, False = use fallback FCardData
  - [x] Log warning once if row not found
  - [x] Added LookupCardDefinition() helper function
  - [x] Modified SpawnCardWidget() to use DataTable with fallback
  - [x] Added per-CardID warning logging (no spam)
- [ ] Add Blueprint binding for keyword chips display
- [ ] Add Blueprint binding for Counter badge (if Counter > 0)
- [ ] Add Blueprint binding for Trigger icon (if HasTrigger)
- [ ] Test with both valid and invalid CardIDs (requires Editor)

#### Definition of Done ✅
- [ ] If CardDatabase row exists, all fields display from row (needs Blueprint testing)
- [x] If row missing, fallback text/power render correctly (implemented in C++)
- [x] Only one "Row not found" log per missing card (uses TSet<FName> tracking)
- [ ] Keywords, Counter, and Trigger display correctly (needs Blueprint work)

#### C++ Changes Complete ✅
- Added `UDataTable* CardDatabase` property to TCGHandWidget.h
- Implemented `LookupCardDefinition()` helper function with smart logging
- Modified `SpawnCardWidget()` to prioritize DataTable lookup with FCardData fallback
- Added `#include "Engine/DataTable.h"` to TCGHandWidget.cpp
- Created PHASE2_CPP_CHANGES.md documentation

#### Blueprint Work Remaining
- WBP_TCG_Card keyword chip bindings
- Counter badge visibility logic
- Trigger icon visibility logic
- Testing in Unreal Editor

#### Files Modified
- `Source/OnePieceTCG_V2/TCGHandWidget.cpp` ✅
- `Source/OnePieceTCG_V2/TCGHandWidget.h` ✅
- `PHASE2_CPP_CHANGES.md` ✅ (new documentation)
- `Content/WBP_TCG_Card.uasset` (pending Blueprint work)
- `Content/WBP_TCG_Hand.uasset` (pending - set CardDatabase property)

---

### **PHASE 3: Spawner Plumbing** ⚫ NOT STARTED

**Goal**: Cards consistently created with CardID and DataTable reference

#### Tasks
- [ ] Update TCGPlayerState.cpp spawning functions:
  - Pass CardID (FName) to all spawn calls
  - Set CardDatabase reference to DT_Cards_Test by default
- [ ] Ensure Hand, Deck, Field spawners pass CardID correctly
- [ ] Optional: Server-side FCardInstance lookup from DataTable
- [ ] Test mixed CardID hand spawning

#### Definition of Done ✅
- [ ] Creating hand with mixed CardIDs renders without manual widget config
- [ ] Spawned cards automatically load correct art/stats from table
- [ ] No hardcoded card data in spawning code

#### Files Modified
- `Source/OnePieceTCG_V2/TCGPlayerState.cpp`
- `Source/OnePieceTCG_V2/TCGPlayerState.h`
- `Source/OnePieceTCG_V2/TCGPlayerController.cpp`

---

### **PHASE 4: Game Event Bus** ⚫ NOT STARTED

**Goal**: Centralized timing system for effect triggers

#### Tasks
- [ ] Create TCGEventBus.h with delegate declarations:
  - `FOnPhaseChanged(EGamePhase NewPhase)`
  - `FOnTurnPhaseStarted(EGamePhase Phase, int32 PlayerID)`
  - `FOnAttackDeclared(FAttackData AttackData)`
  - `FOnBlockWindowOpen(FAttackData AttackData)`
  - `FOnCounterWindowOpen(FAttackData AttackData)`
  - `FOnDamageResolved(int32 Damage, int32 TargetPlayerID)`
  - `FOnBattleEnd(FAttackData FinalState)`
  - `FOnCardMoved(FCardInstance Card, ECardZone FromZone, ECardZone ToZone)`
  - `FOnCardPlayed(FCardInstance Card)`
  - `FOnCardRested(FCardInstance Card)`
  - `FOnDonAttached(FCardInstance Card, int32 DonCount)`
- [ ] Add event bus to TCGGameMode
- [ ] Hook existing phase transitions to broadcast events
- [ ] Add no-op test subscribers that log events
- [ ] Create debug UI overlay to show event stream

#### Definition of Done ✅
- [ ] Test match cycles through all phases
- [ ] Each phase transition broadcasts correct event
- [ ] Events log in correct order (Refresh → Draw → DON → Main → Battle → End)
- [ ] No gameplay logic attached yet (just logging)

#### Files Modified
- `Source/OnePieceTCG_V2/TCGEventBus.h` (new)
- `Source/OnePieceTCG_V2/TCGGameMode.cpp`
- `Source/OnePieceTCG_V2/TCGGameMode.h`

---

### **PHASE 5: Minimal Battle Loop** ⚫ NOT STARTED

**Goal**: Attack-to-damage without effects

#### Tasks
- [ ] Implement legal attack declaration validation:
  - Character must be active (not rested)
  - Must be Battle Phase
  - Must be attacker's turn
- [ ] Implement optional blocking:
  - Defender selects blocker with [Blocker] keyword
  - Server validates blocker eligibility
- [ ] Implement damage resolution:
  - Compare attacker power vs defender/leader power
  - Apply damage to Life zone if attacking leader
  - Move defeated character to Trash
  - Move Life card to Hand (no Trigger handling yet)
- [ ] Enforce character area limits (5 characters max)
- [ ] Enforce stage slot limit (1 stage max)

#### Definition of Done ✅
- [ ] Can declare attacks on characters and leader
- [ ] Can assign blocker to intercept attack
- [ ] Damage resolves correctly (life to hand, KO to trash)
- [ ] Character/stage limits enforced
- [ ] No effect triggers or counters (pure combat math)

#### Files Modified
- `Source/OnePieceTCG_V2/TCGGameMode.cpp`
- `Source/OnePieceTCG_V2/TCGPlayerController.cpp`
- `Source/OnePieceTCG_V2/TCGPlayerState.cpp`

---

### **PHASE 6: Counter Window** ⚫ NOT STARTED

**Goal**: Defensive interaction functional

#### Tasks
- [ ] Add CounterWindow UI:
  - Show attacker power, defender power
  - Display available counter options (hand Events, Characters with Counter)
  - "Play Counter" and "Pass" buttons
- [ ] Implement Event [Counter] playing:
  - Pay cost, grant power to defender
  - Apply "until end of battle" modifier
- [ ] Implement Character Counter trashing:
  - Trash character from hand, grant Counter value to defender
- [ ] Allow multiple counter actions in sequence
- [ ] Expire power buffs at EndOfBattle event

#### Definition of Done ✅
- [ ] Counter actions change battle outcome
- [ ] Power buffs display correctly during battle
- [ ] Buffs expire cleanly at battle end
- [ ] Can play multiple counters in one battle
- [ ] Server validates all counter actions

#### Files Modified
- `Source/OnePieceTCG_V2/TCGGameMode.cpp`
- `Content/WBP_TCG_HUD.uasset`
- `Content/WBP_CounterWindow.uasset` (new)

---

### **PHASE 7: Trigger Window** ⚫ NOT STARTED

**Goal**: Life reveal decisions correct

#### Tasks
- [ ] Detect damage instances that move Life to Hand
- [ ] Check if Life card has HasTrigger flag
- [ ] Present modal choice to damaged player:
  - "Reveal and resolve Trigger effect"
  - "Add to hand without revealing"
- [ ] Execute chosen path:
  - If revealed: Broadcast TriggerRevealed event, queue effect resolution
  - If not: Silently add to hand
- [ ] Continue damage processing after choice
- [ ] Ensure exactly one Trigger decision per damage instance

#### Definition of Done ✅
- [ ] Trigger cards offer choice modal correctly
- [ ] Resolving Trigger executes effect (logged for now)
- [ ] Not revealing adds card to hand silently
- [ ] Multiple damage instances = multiple Trigger checks
- [ ] No double-triggering bugs

#### Files Modified
- `Source/OnePieceTCG_V2/TCGGameMode.cpp`
- `Content/WBP_TriggerModal.uasset` (new)

---

### **PHASE 8: Effect Executor MVP** ⚫ NOT STARTED

**Goal**: Run simple effects from data (BIGGEST PHASE)

#### Scope (Start Small)
**Timings Supported**:
- OnPlay (when card enters play)
- ActivateMain (manual activation in Main Phase)
- Trigger (when revealed from Life)
- Counter (when used as counter)

**Conditions Supported**:
- `YourTurn` - Only on your turn
- `DonRequirement:N` - Need N active DON
- `TargetType:Character/Leader/Stage` - Filter targets
- `TargetColor:Red/Green/Blue` - Filter by color
- `TargetAttribute:X` - Filter by attribute

**Costs Supported**:
- `DonRest:N` - Rest N active DON
- `DonReturn:N` - Return N DON to DON deck
- `TrashSelf` - Trash this card
- `TrashCard:N:Zone` - Trash N cards from zone

**Operations Supported**:
- `Draw:N` - Draw N cards
- `GivePower:Target:Amount:Duration` - Power buff (UntilEndOfTurn, UntilEndOfBattle)
- `Rest:Target` - Rest target card
- `KO:Target` - Move target to Trash
- `SetActive:Target` - Un-rest target card
- `SearchDeck:Count:Filter:Destination` - Search and move cards
- `MoveCard:Target:Destination` - Move card to zone

#### Tasks
- [ ] Create effect parsing system (FName string → actions)
- [ ] Implement effect resolution queue (FIFO)
- [ ] Build targeting system with filter support
- [ ] Implement cost payment validation
- [ ] Implement condition checking
- [ ] Create modifier tracking system (power buffs with durations)
- [ ] Hook effect triggers to event bus
- [ ] Add effect resolution to test cards
- [ ] Build effect debug UI (shows active modifiers, pending queue)

#### Definition of Done ✅
- [ ] All 8 test cards execute their effects from DataTable
- [ ] No card-specific hardcoded logic
- [ ] Effects trigger at correct timing
- [ ] Costs and conditions validated correctly
- [ ] Power modifiers apply and expire correctly
- [ ] Effect queue processes in order

#### Files Modified
- `Source/OnePieceTCG_V2/TCGEffectExecutor.h` (new)
- `Source/OnePieceTCG_V2/TCGEffectExecutor.cpp` (new)
- `Source/OnePieceTCG_V2/TCGGameMode.cpp`
- `Source/OnePieceTCG_V2/TCGTypes.h`
- `Content/WBP_EffectDebugOverlay.uasset` (new)

---

### **PHASE 9: Replacement Effects & Permanent Auras** ⚫ NOT STARTED

**Goal**: Expand effect coverage gradually

#### Tasks
- [ ] Add Replacement effect timing:
  - "Instead" hooks that modify game actions before they occur
  - One-time application flag
- [ ] Add Permanent effect timing:
  - Continuous auras that apply while card is in play
  - State-watching modifiers (e.g., "All your Red characters get +1000 power")
- [ ] Implement cost modifiers (reduce/increase costs)
- [ ] Implement stacking rules for multiple modifiers
- [ ] Add expiration tracking (EndOfTurn, WhileInPlay, etc.)

#### Definition of Done ✅
- [ ] At least one Replacement effect card works (e.g., damage prevention)
- [ ] At least one Permanent aura card works (e.g., buff all allies)
- [ ] Multiple modifiers stack correctly
- [ ] Modifiers expire at correct timing

#### Files Modified
- `Source/OnePieceTCG_V2/TCGEffectExecutor.cpp`
- `Source/OnePieceTCG_V2/TCGTypes.h`

---

### **PHASE 10: Server Authority & Replication Polish** ⚫ NOT STARTED

**Goal**: No desyncs, clean reveals, multiplayer-ready

#### Tasks
- [ ] Audit all gameplay actions for server authority
- [ ] Ensure effects execute server-side only
- [ ] Replicate state deltas to clients (not full state)
- [ ] Implement hidden information rules:
  - Opponent's hand is hidden
  - Deck order is hidden
  - Life cards hidden until revealed
- [ ] Add deterministic RNG seeding for server
- [ ] Validate client selections server-side (no trust)
- [ ] Add anti-cheat validation for card plays

#### Definition of Done ✅
- [ ] Two-client PIE test runs full match
- [ ] Both clients see identical game state
- [ ] Hidden info respected (can't peek at opponent hand/deck)
- [ ] No desync errors through attacks, counters, triggers
- [ ] Server logs validate all client actions

#### Files Modified
- `Source/OnePieceTCG_V2/TCGGameMode.cpp`
- `Source/OnePieceTCG_V2/TCGPlayerState.cpp`
- `Source/OnePieceTCG_V2/TCGPlayerController.cpp`

---

### **PHASE 11: QA Fixtures & Tools** ⚫ NOT STARTED

**Goal**: Fast iteration and regression testing

#### Tasks
- [ ] Create "Gold" scripted test system:
  - JSON/CSV format for turn-by-turn actions
  - Expected state assertions after each action
  - Automated playback in PIE
- [ ] Build debug overlay UI:
  - Shows active modifiers per card
  - Displays pending effect queue
  - Event log per card (what happened this turn)
  - Game state inspector (DON counts, life totals, etc.)
- [ ] Create 5 gold test scenarios:
  1. Basic combat (no effects)
  2. Counter window usage
  3. Trigger reveal and resolution
  4. OnPlay effect chain
  5. Full game to victory
- [ ] Add performance profiling hooks

#### Definition of Done ✅
- [ ] Gold tests run automatically and pass
- [ ] Debug overlay available in PIE via hotkey
- [ ] Can inspect any card's modifier stack
- [ ] Can replay any gold test scenario

#### Files Modified
- `Source/OnePieceTCG_V2/Testing/TCGTestRunner.h` (new)
- `Source/OnePieceTCG_V2/Testing/TCGTestRunner.cpp` (new)
- `Content/Testing/GoldTests/*.json` (new)
- `Content/WBP_DebugOverlay.uasset` (new)

---

## **MILESTONE 1 COMPLETE** ✅ (When Phases 1-11 Done)

**Checkpoint**: You now have a playable 2-player One Piece TCG with:
- 50+ cards with working effects
- Full combat system (attack, block, counter, trigger)
- Data-driven card definitions
- Networked multiplayer
- Automated testing

**Next**: Move to single-player features

---

### **PHASE 12: Tutorial System** ⚫ NOT STARTED

**Goal**: Teach new players the game rules

#### Tasks
- [ ] Design tutorial scenario scripting system:
  - JSON format for tutorial steps
  - Locked phases until objectives complete
  - UI hints/tooltips triggered by events
  - Force specific actions for teaching
- [ ] Create TutorialGameMode (extends TCGGameMode)
- [ ] Build 5 tutorial scenarios:
  1. Basic turn structure (phases)
  2. Playing characters and DON system
  3. Attacking and blocking
  4. Using counters defensively
  5. Trigger effects and victory conditions
- [ ] Create tutorial UI overlays:
  - Highlight specific UI elements
  - Show instruction text
  - "Next" button when objective complete
- [ ] Add tutorial progress tracking (save which completed)

#### Definition of Done ✅
- [ ] All 5 tutorials playable start to finish
- [ ] Tutorials block invalid actions appropriately
- [ ] Hints appear at correct moments
- [ ] New player can learn rules without external docs
- [ ] Tutorial progress saves between sessions

#### Files Modified
- `Source/OnePieceTCG_V2/Tutorial/TutorialGameMode.h` (new)
- `Source/OnePieceTCG_V2/Tutorial/TutorialGameMode.cpp` (new)
- `Content/Tutorial/Scenarios/*.json` (new)
- `Content/WBP_TutorialOverlay.uasset` (new)

---

### **PHASE 13: AI Opponent** ⚫ NOT STARTED

**Goal**: Single-player vs computer

#### Implementation Approach
**Phase 13A: Random AI** (1 week)
- AI makes random legal moves
- Good for basic testing

**Phase 13B: Heuristic AI** (2-3 weeks)
- Attack when power advantage
- Block when losing/low life
- Play high-value cards first
- Use counters when critical

**Phase 13C: Advanced AI** (Future - MCTS/ML)
- Monte Carlo Tree Search
- Deep learning (way later)

#### Tasks (Phase 13A - Random AI)
- [ ] Create AIPlayerController (extends TCGPlayerController)
- [ ] Implement decision loop:
  - Detect when AI needs to make choice
  - Query valid actions from game state
  - Select random valid action
  - Submit action to server
- [ ] Handle all decision points:
  - Which cards to play in Main Phase
  - Which DON to activate
  - Attack declarations
  - Block selection
  - Counter usage
  - Trigger reveal choice
- [ ] Add AI delay (so moves aren't instant)
- [ ] Create AI vs Player game mode

#### Definition of Done ✅
- [ ] Can start match vs AI opponent
- [ ] AI makes legal moves (never illegal actions)
- [ ] AI completes full match to victory/defeat
- [ ] AI responds to all decision prompts
- [ ] AI behavior is visible/understandable to player

#### Files Modified
- `Source/OnePieceTCG_V2/AI/AIPlayerController.h` (new)
- `Source/OnePieceTCG_V2/AI/AIPlayerController.cpp` (new)
- `Source/OnePieceTCG_V2/TCGGameMode.cpp`

---

### **PHASE 14: Deck Builder UI** ⚫ NOT STARTED

**Goal**: Players can customize their decks

#### Tasks
- [ ] Design deck data structure:
  - Leader card (1)
  - Main deck (50 cards exactly)
  - DON deck (10 cards)
  - Deck name, metadata
- [ ] Create deck save/load system (JSON files)
- [ ] Build deck builder UI:
  - Card collection browser (left pane)
  - Current deck list (right pane)
  - Add/remove cards with click
  - Filter/search by name, type, color, cost
  - Deck validation (50 card limit, color rules, etc.)
  - Save/load deck buttons
- [ ] Create starter deck templates:
  - Red Straw Hat deck
  - Green Supernovas deck
  - Blue Navy deck
  - Purple Big Mom deck
  - 4-5 total starter decks
- [ ] Hook deck selection to match start
- [ ] Show deck legality errors before match

#### Definition of Done ✅
- [ ] Can browse all available cards
- [ ] Can build custom 50-card deck with leader
- [ ] Deck validation prevents illegal decks (too many cards, wrong colors, etc.)
- [ ] Can save/load custom decks
- [ ] Starter decks provided for quick play
- [ ] Selected deck used when starting match

#### Files Modified
- `Source/OnePieceTCG_V2/DeckSystem/DeckManager.h` (new)
- `Source/OnePieceTCG_V2/DeckSystem/DeckManager.cpp` (new)
- `Content/WBP_DeckBuilder.uasset` (new)
- `Content/StarterDecks/*.json` (new)

---

## **MILESTONE 2 COMPLETE** ✅ (When Phases 12-14 Done)

**Checkpoint**: Players can now:
- Learn the game via tutorial
- Play vs AI opponent
- Build custom decks

**Next**: Add collection/progression systems

---

### **PHASE 15: Collection & Pack Opening** ⚫ NOT STARTED

**Goal**: Card acquisition and ownership

#### Tasks
- [ ] Design collection system:
  - Player inventory (which cards owned)
  - Card quantities (can own multiples)
  - Rarity tiers (Common, Uncommon, Rare, Secret Rare)
  - Set completion tracking
- [ ] Create pack definition system:
  - Pack types (Starter, Booster, Premium)
  - Card distribution (rarities per pack)
  - Set association (OP01, OP02, etc.)
- [ ] Implement pack opening logic:
  - Random card selection weighted by rarity
  - Add cards to collection
  - Prevent/allow duplicates based on rules
- [ ] Build pack opening UI:
  - Animated card reveal (flip animation)
  - Show rarity with visual effects
  - "Open Another" or "Done" buttons
  - Collection progress display
- [ ] Add starting collection (give tutorial/starter cards)
- [ ] Persistence: Save/load collection to disk

#### Definition of Done ✅
- [ ] Players start with starter collection
- [ ] Can open packs and receive random cards
- [ ] Cards added to collection inventory
- [ ] Pack opening UI is polished and satisfying
- [ ] Collection persists between sessions
- [ ] Deck builder shows only owned cards

#### Files Modified
- `Source/OnePieceTCG_V2/Collection/CollectionManager.h` (new)
- `Source/OnePieceTCG_V2/Collection/CollectionManager.cpp` (new)
- `Source/OnePieceTCG_V2/Collection/PackDefinitions.h` (new)
- `Content/WBP_PackOpening.uasset` (new)
- `Content/Collection/PackDefinitions/*.json` (new)

---

### **PHASE 16: Full Card Import** ⚫ NOT STARTED

**Goal**: All 1000+ One Piece cards available

#### Tasks
- [ ] Source card data from official database/API
- [ ] Create import tool:
  - Parse external card data format
  - Map to FCardDefinition struct
  - Generate DataTable CSV
  - Download card art assets
- [ ] Import all sets:
  - OP01-OP13 (Booster sets)
  - ST01-ST28 (Starter decks)
  - EB01-EB02 (Extra boosters)
  - Promo cards
- [ ] Map card effects to effect system:
  - Auto-map simple effects (draw, power, etc.)
  - Flag complex effects for manual implementation
- [ ] Add missing effect operations as needed
- [ ] QA test sample cards from each set

#### Definition of Done ✅
- [ ] 1000+ cards imported with correct stats
- [ ] Card art loads for all imported cards
- [ ] At least 200 cards have working effects
- [ ] Complex effects flagged with TODO notes
- [ ] All cards visible in deck builder

#### Files Modified
- `Content/Cards/Data/DT_AllCards.uasset` (new)
- `Tools/CardImporter/` (new tool)
- Card art assets in `Content/Cards/`

---

## **MILESTONE 3 COMPLETE** ✅ (When Phases 15-16 Done)

**Checkpoint**: Full-featured card game with:
- Complete card library
- Collection/progression system
- Pack opening

**Next**: Polish and competitive features

---

### **PHASE 17: Matchmaking & Online Play** ⚫ NOT STARTED

**Goal**: Find opponents online

#### Tasks
- [ ] Integrate Unreal Online Subsystem (Steam, EOS, or custom)
- [ ] Implement matchmaking queue
- [ ] Add ranked/unranked modes
- [ ] Build lobby system
- [ ] Add friend list and invites
- [ ] Implement reconnection on disconnect

#### Definition of Done ✅
- [ ] Can queue for match and find opponent
- [ ] Stable networked gameplay
- [ ] Ranked ladder functional

---

### **PHASE 18: Polish, SFX, VFX** ⚫ NOT STARTED

**Goal**: Production-quality presentation

#### Tasks
- [ ] Add sound effects for all actions
- [ ] Create VFX for card plays, attacks, etc.
- [ ] Polish all UI animations
- [ ] Add background music
- [ ] Optimize performance

---

## **MILESTONE 4 COMPLETE** ✅

**Checkpoint**: Shippable commercial-quality game

---

## 📊 Progress Tracking

### Completion Status

| Phase | Name | Status | Completion | ETA |
|-------|------|--------|------------|-----|
| 1 | Canonical Data Model | 🟡 In Progress | 75% | Requires Editor |
| 2 | WBP_TCG_Card Reads Table | ⚫ Not Started | 0% | - |
| 3 | Spawner Plumbing | ⚫ Not Started | 0% | - |
| 4 | Game Event Bus | ⚫ Not Started | 0% | - |
| 5 | Minimal Battle Loop | ⚫ Not Started | 0% | - |
| 6 | Counter Window | ⚫ Not Started | 0% | - |
| 7 | Trigger Window | ⚫ Not Started | 0% | - |
| 8 | Effect Executor MVP | ⚫ Not Started | 0% | - |
| 9 | Replacement/Permanent Effects | ⚫ Not Started | 0% | - |
| 10 | Server Authority & Replication | ⚫ Not Started | 0% | - |
| 11 | QA Fixtures & Tools | ⚫ Not Started | 0% | - |
| 12 | Tutorial System | ⚫ Not Started | 0% | - |
| 13 | AI Opponent | ⚫ Not Started | 0% | - |
| 14 | Deck Builder UI | ⚫ Not Started | 0% | - |
| 15 | Collection & Pack Opening | ⚫ Not Started | 0% | - |
| 16 | Full Card Import | ⚫ Not Started | 0% | - |

### Milestone Status

| Milestone | Phases | Status | Completion |
|-----------|--------|--------|------------|
| 1: Core Engine | 1-11 | 🟡 In Progress | 7% |
| 2: Single-Player | 12-14 | ⚫ Blocked | 0% |
| 3: Collection | 15-16 | ⚫ Blocked | 0% |
| 4: Polish | 17-18 | ⚫ Blocked | 0% |

---

## 🔄 Update Process

### After Each Work Session

1. **Update CHANGELOG.md** with changes made
2. **Mark todos complete** in this document
3. **Update phase completion %** in progress table
4. **Commit with descriptive message**
5. **Push to feature branch**

### Phase Completion Checklist

When finishing a phase:
- [ ] All tasks checked off
- [ ] All DoD criteria met and tested
- [ ] Code reviewed for quality
- [ ] Documentation updated
- [ ] Committed and pushed to branch
- [ ] Phase marked complete in progress table
- [ ] Entry added to CHANGELOG.md

---

## 📝 Notes & Decisions

### Design Decisions

**2025-11-13**:
- Decided to split FCardData into FCardDefinition (static) and FCardInstance (runtime)
- Using FName arrays for effects initially (parse strings in C++)
- Multi-color support via TArray<ECardColor> instead of bitflags

### Technical Debt

*None yet - track as phases complete*

### Known Issues

*None yet - track as discovered*

---

## 🎯 Current Focus

**Active Phase**: Phase 1 - Canonical Data Model (75% complete)
**Status**: C++ foundation complete, awaiting Unreal Editor setup

**Completed This Session**:
- ✅ FCardDefinition struct (static card data)
- ✅ FCardInstance struct (runtime game state)
- ✅ FEffectRow struct (data-driven effects)
- ✅ FActiveModifier struct (buffs/debuffs system)
- ✅ Test card CSV with 8 real One Piece cards

**Pending (Requires Unreal Editor)**:
- Create S_CardDefinition struct asset
- Import Cards_Test.csv as DataTable
- Verify card art loading

**Next Up**: Complete Phase 1 editor setup, then Phase 2 - WBP_TCG_Card table integration

---

## 📞 Resources

- **Official Rules**: `/Content/ref docs/rule_comprehensive.pdf`
- **Existing Docs**:
  - `NEXT_STEPS.md` (original roadmap)
  - `HAND_WIDGET_SETUP.md` (hand widget guide)
- **Card Database**: [One Piece Card Game Official Site](https://en.onepiece-cardgame.com/)

---

**END OF IMPLEMENTATION PLAN**
