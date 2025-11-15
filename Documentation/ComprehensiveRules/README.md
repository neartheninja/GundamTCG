# Gundam TCG Comprehensive Rules Documentation

This folder contains modular documentation for the official Gundam Trading Card Game Comprehensive Rules.

## Purpose

To provide a clean separation between rules documentation and code implementation:
- **Documentation**: Official rules text stored in markdown files
- **Implementation**: Code references rules by section number (e.g., "See Rule 2-4-2-1")
- **Validation**: Rules validation logic in `UGCGComprehensiveRulesSubsystem`

## Structure

Each comprehensive rules section has its own markdown file:

- `Section_01_GameOverview.md` - Win/loss conditions, fundamental rules
- `Section_02_CardInformation.md` - Card attributes and terminology
- `Section_03_CardTypes.md` - Unit, Pilot, Command, Base, Resource types
- `Section_04_GameLocations.md` - Zones and areas
- `Section_05_Terminology.md` - Essential game terms
- `Section_06_PreparingToPlay.md` - Game setup procedures
- `Section_07_GameProgression.md` - Turn structure and phases
- `Section_08_AttacksAndBattles.md` - Combat rules
- `Section_09_ActionSteps.md` - Player action timing
- `Section_10_EffectActivation.md` - Effect resolution rules
- `Section_11_RulesManagement.md` - Rules processing and state-based actions
- `Section_12_MultiplayerBattle.md` - 2v2 team battle rules
- `Section_13_Keywords.md` - Keyword abilities and effects

## Integration Status

| Section | Status | Implementation Notes |
|---------|--------|---------------------|
| Section 1 | ✅ Integrated | Rules management in `AGCGGameMode_1v1` |
| Section 2 | ✅ Integrated | Documentation in `GCGTypes.h`, ⚠️ Breaking change (color enum) |
| Section 3-13 | ⏳ Pending | Will use modular approach (subsystem + docs only) |

## Modular Integration Approach

**For future sections (3-13):**

1. **Documentation First**: Add official rules text to appropriate markdown file
2. **Validation Logic**: Add validation methods to `UGCGComprehensiveRulesSubsystem`
3. **Minimal Code Changes**: Only modify core types if absolutely necessary
4. **Comments Reference Rules**: Use rule numbers in comments (e.g., `// Rule 3-2-1: Unit cards...`)

**Avoid:**
- Modifying core enums in `GCGTypes.h` (breaking changes)
- Changing existing data structures
- Duplicating rules text in code comments

**Prefer:**
- External subsystem validation
- Rule references by number
- Centralized documentation

## Breaking Changes Log

### Section 2 Integration (Commit f1e24c6)
- **Changed**: `EGCGCardColor` enum
  - Removed: `Black`, `Yellow`
  - Added: `Purple`
  - Reason: Match official rules (5 colors: White, Blue, Green, Red, Purple)
- **Impact**: Existing card data using Black/Yellow colors will need migration

## Reference

All rules documentation is based on the official Gundam Trading Card Game Comprehensive Rules.
