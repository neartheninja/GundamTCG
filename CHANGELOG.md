# Changelog

All notable changes to OnePieceTCG_V2 will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### In Progress - Phase 2: DataTable Integration (50% Complete)

#### Added (C++ Implementation) ✅
- **DataTable lookup system** in TCGHandWidget:
  - `UDataTable* CardDatabase` property for referencing DT_Cards_Test
  - `LookupCardDefinition()` helper function with graceful error handling
  - Smart logging system (one warning per missing card, avoids spam)
  - `LoggedMissingCards` TSet for tracking logged warnings
- **Modified `SpawnCardWidget()`** to prioritize DataTable:
  - Attempts DataTable lookup by CardID first
  - Converts FCardDefinition → FCardData when found
  - Falls back to legacy FCardData when lookup fails
  - Logs data source used (DataTable vs fallback)
- **Documentation**: PHASE2_CPP_CHANGES.md with technical details

#### Changed
- TCGHandWidget.h: Added DataTable support (+4 lines)
- TCGHandWidget.cpp: Implemented DataTable lookup with fallback (+60 lines)

#### Blueprint Work Remaining
- [ ] Add keyword chip bindings to WBP_TCG_Card
- [ ] Add Counter badge visibility logic
- [ ] Add Trigger icon visibility logic
- [ ] Set CardDatabase property in WBP_TCG_Hand to reference DT_Cards_Test
- [ ] Test with valid/invalid CardIDs in Unreal Editor

#### Technical Notes
- **Backward Compatible**: 100% compatible with existing FCardData code
- **Graceful Degradation**: Widget works even without DataTable assigned
- **Performance**: O(log n) DataTable lookup, minimal overhead
- **Multi-color Limitation**: Currently uses primary color only (FCardData limitation)

---

## [0.2.0-alpha] - 2025-11-13

### ✅ Phase 1 COMPLETE - Canonical Data Model

**Achievement**: Established complete data-driven foundation for One Piece TCG game

#### What Was Completed

1. **C++ Data Structures** (TCGTypes.h):
   - FCardDefinition: Static card data (280 lines)
   - FCardInstance: Runtime game state
   - FEffectRow: Data-driven effect system
   - FActiveModifier: Buff/debuff tracking
   - EModifierDuration: Effect expiration timing

2. **Test Card Database**:
   - 11 real One Piece TCG cards with accurate stats
   - All 4 card types: CHARACTER (8), LEADER (1), EVENT (1), STAGE (1)
   - Comprehensive keyword coverage:
     - DON!! conditionals: DonX1, DonX2
     - Timing: OnPlay, ActivateMain, WhenAttacking, YourTurn
     - Combat: Rush, Blocker, Banish, DoubleAttack
     - Dual-mode: Counter + Trigger (Radical Beam!!)

3. **Unreal Editor Integration**:
   - DT_Cards_Test DataTable created successfully
   - CSV import working with enum-compatible keywords
   - Card art loading correctly (TSoftObjectPtr system)
   - All multi-value arrays functional (Colors, Types, Keywords)
   - Row Name = CardID for direct lookup

4. **System Validation** ✅:
   - Multi-keyword cards working (Law: Blocker + OnPlay)
   - Multi-type cards working (Moria, Crocodile: 2 types each)
   - Conditional effects (DON!! x1, DON!! x2, hand size checks)
   - Dual-mode effects (Radical Beam: Counter + Trigger)
   - Leader stats (Zoro Leader has 5 Life)
   - Stage mechanics (Mini-Merry)

#### Test Card Coverage

| Card | Test Case |
|------|-----------|
| ST01-013 Zoro | DON!! conditional power boost |
| ST01-007 Nami | Activate Main with Once Per Turn |
| OP01-068 Gecko Moria | Conditional keyword grant (Double Attack) |
| OP01-029 Radical Beam!! | Dual-mode (Counter + Trigger) |
| OP01-047 Law | Multi-keyword (Blocker + OnPlay) |
| OP01-067 Crocodile | Banish + DON!! cost reduction |
| OP03-123 Katakuri | Zone manipulation (Life zone) |
| ST01-012 Luffy | Rush + Blocker negation |
| OP01-001 Zoro Leader | Leader card with Life stat |
| EB01-011 Mini-Merry | Stage card mechanics |
| EB01-012 Cavendish | Deck search effect |

#### Documentation Created
- IMPLEMENTATION_PLAN.md: 18-phase roadmap (833 lines)
- CHANGELOG.md: Version tracking system
- DATATABLE_SETUP_GUIDE.md: Editor import instructions
- DATATABLE_FIELD_REFERENCE.md: Field documentation
- CARD_DATA_CORRECTIONS.md: Card change log

#### Files Modified/Created
- Source/OnePieceTCG_V2/TCGTypes.h (+280 lines)
- Content/Cards/Data/Cards_Test.csv (11 cards)
- Content/Cards/Data/DT_Cards_Test.uasset (DataTable)
- IMPLEMENTATION_PLAN.md (Phase 1: 100% ✅)
- CHANGELOG.md (this file)

#### Technical Achievement
This phase established:
- ✅ Separation of static/runtime data (memory efficient)
- ✅ Multi-color/multi-type support (flexible arrays)
- ✅ Keyword system (scalable with TSet)
- ✅ Effect framework (ready for Phase 8 implementation)
- ✅ Modifier tracking (full buff/debuff system)
- ✅ Blueprint accessibility (USTRUCT(BlueprintType))

#### Impact
- Cards can now be added without code changes (pure data)
- Effect system ready for systematic implementation
- All modifiers tracked in one place (no hidden state)
- UI can read from single source of truth
- Test coverage for Phase 8 effect executor

#### Next Phase
Phase 2: Update WBP_TCG_Card widget to read from DT_Cards_Test with fallback handling

**Version**: 0.2.0-alpha (Phase 1 complete)
**Milestone 1 Progress**: 1/11 phases complete (9%)

---

## [0.1.1-alpha] - 2025-11-13

### Added - Phase 1 Foundation
- **FCardDefinition struct**: Complete static card data structure with:
  - Identity fields: CardID, CardName, CardType, Colors (TArray), Attributes, Types
  - Stats fields: Cost, Power, Life, Counter
  - Presentation: CardText (FText), CardArt (TSoftObjectPtr)
  - Keywords: TSet of FName for Blocker, Rush, DoubleAttack, etc.
  - Effects: EffectIDs array and InlineEffects array
  - Helper methods: HasKeyword(), GetPrimaryColor(), IsMulticolor()

- **FCardInstance struct**: Runtime game state for card instances with:
  - InstanceID, CardDefinitionID reference
  - Game state: CurrentZone, bIsRested, AttachedDonCount, OwnerPlayerID
  - ActiveModifiers array for buffs/debuffs
  - Helper methods: GetTotalPower(), GetTotalCost()

- **FEffectRow struct**: Minimal effect system for data-driven card effects with:
  - Timing: When effect triggers (OnPlay, WhenAttacking, etc.)
  - Conditions: Requirements to activate (YourTurn, DonRequirement, etc.)
  - Costs: Activation costs (DonRest, TrashSelf, etc.)
  - Operations: What the effect does (Draw, GivePower, KO, etc.)
  - Description: Human-readable text for UI

- **FActiveModifier struct**: Runtime modifier system with:
  - ModifierType (Power, Cost, Counter)
  - Amount (can be negative)
  - Duration (UntilEndOfTurn, UntilEndOfBattle, WhileInPlay, Permanent)
  - SourceInstanceID for tracking

- **EModifierDuration enum**: Timing for effect expiration

- **Test card data CSV**: 8 real One Piece TCG cards for testing:
  1. ST01-013: Roronoa Zoro (Vanilla Character)
  2. ST01-007: Nami (Counter Character)
  3. OP01-068: Jinbe (Blocker keyword)
  4. OP01-029: Gum-Gum Red Roc (Event with [Counter])
  5. OP01-047: Trafalgar Law (OnPlay effect)
  6. OP01-067: Nico Robin (Activate Main effect)
  7. OP03-123: Charlotte Katakuri (Trigger card)
  8. ST01-012: Monkey.D.Luffy (Leader card)

### Changed
- Marked FCardData as deprecated/legacy (kept for backward compatibility)
- Updated TCGTypes.h with comprehensive data model architecture

### Technical Notes
- All C++ structures compile successfully
- Blueprint/Editor setup required for Phase 1 completion (must open in Unreal Editor)
- DataTable creation pending in editor
- Multi-color support via TArray<ECardColor> (flexible for any combination)
- Effect system uses FName string parsing approach for MVP flexibility

### Next Steps (Phase 1 DoD Remaining)
- Open project in Unreal Editor
- Create S_CardDefinition struct asset in Content/
- Create DT_Cards_Test DataTable from Cards_Test.csv
- Verify all 8 test cards load correctly with art assets
- Confirm Row Name = CardID for lookups

---

## [0.1.0-alpha] - 2025-11-13

### Added
- Created comprehensive IMPLEMENTATION_PLAN.md with 18 phases across 4 milestones
- Created CHANGELOG.md for tracking project updates
- Established version control and progression tracking system
- Defined clear Definition of Done criteria for all phases
- Moved Deck Builder to Milestone 2 (Single-Player Features)

### Project Structure
- **Milestone 1** (Phases 1-11): Core Game Engine
- **Milestone 2** (Phases 12-14): Single-Player Features (Tutorial, AI, Deck Builder)
- **Milestone 3** (Phases 15-16): Collection System
- **Milestone 4** (Phases 17-18): Polish & Online Features

### Technical Decisions
- Splitting FCardData into FCardDefinition (static) and FCardInstance (runtime)
- Using FName arrays for initial effect system (string parsing approach)
- Multi-color support via TArray<ECardColor> instead of bitflags
- Event bus pattern for centralized game timing
- Server-authoritative gameplay with replication

---

## [0.0.1] - 2025-11-11 (Pre-Planning)

### Completed
- Hand Display Widget implementation (WBP_TCG_Hand)
- Fixed card art display in hand widget
- Data-driven card spawning in hand
- Added official rules PDF files
- Basic multiplayer networking setup
- Core game classes (GameMode, PlayerState, PlayerController)

### Existing Features
- Turn-based phase system (Refresh, Draw, DON, Main, Battle, End)
- Card zones (Deck, Hand, Life, DON, Character, Stage, Trash)
- DON resource system
- Basic combat mechanics
- Card data structures (FCardData, FAttackData)
- Card image library (Sets OP01-OP13, ST01-ST28, EB01-EB02, Promos)
- Playmat backgrounds (all colors)

---

## Template for Future Updates

### [Version] - YYYY-MM-DD

#### Added
- New features

#### Changed
- Changes to existing functionality

#### Deprecated
- Soon-to-be removed features

#### Removed
- Removed features

#### Fixed
- Bug fixes

#### Security
- Security improvements

---

**Note**: This changelog will be updated after each work session. Each phase completion will warrant a version bump.

Versioning scheme:
- **Major** (X.0.0): Milestone completions
- **Minor** (0.X.0): Phase completions
- **Patch** (0.0.X): Bug fixes and small improvements
