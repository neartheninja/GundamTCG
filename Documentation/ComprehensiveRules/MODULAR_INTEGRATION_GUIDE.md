# Modular Comprehensive Rules Integration Guide

This document explains the new modular approach for integrating Gundam TCG Comprehensive Rules Sections 3-13.

---

## The Problem

**Previous approach (Sections 1-2):**
- Modified core type enums in `GCGTypes.h` (e.g., changed color enum)
- Created breaking changes that require data migration
- Mixed rules documentation into code comments
- Scattered validation logic across multiple files

**Issues:**
1. ⚠️ Breaking changes affect existing card data
2. Hard to maintain - rules text duplicated in code
3. Difficult to track which rules are implemented
4. Changes require recompilation of many files

---

## The Solution: Modular Architecture

**New approach (Sections 3-13 and beyond):**

### 1. Documentation Layer
- **Location**: `Documentation/ComprehensiveRules/Section_XX_Name.md`
- **Purpose**: Official rules text in markdown format
- **Benefits**: Easy to read, update, and reference

### 2. Validation Layer
- **Location**: `UGCGComprehensiveRulesSubsystem` class
- **Purpose**: Centralized rules validation methods
- **Benefits**: No changes to core types, single source of truth

### 3. Code References
- **Location**: Inline comments in relevant subsystems
- **Format**: `// Rule X-Y-Z: Brief description`
- **Purpose**: Link code to official rules

---

## How to Integrate New Comprehensive Rules Sections

### Step 1: Create Documentation

Use the template: `Documentation/ComprehensiveRules/TEMPLATE_Section.md`

```bash
# Copy template
cp Documentation/ComprehensiveRules/TEMPLATE_Section.md \
   Documentation/ComprehensiveRules/Section_03_CardTypes.md

# Edit the file with official rules text
```

**What to include:**
- Full official rules text from Gundam TCG rulebook
- Rule numbers (e.g., 3-1, 3-1-1, 3-1-2)
- Implementation notes showing WHERE each rule is enforced
- Testing notes for each rule
- Implementation checklist

### Step 2: Add Validation Methods

Add methods to `GCGComprehensiveRulesSubsystem.h/.cpp`

**Header (.h):**
```cpp
// ===== SECTION 3: CARD TYPES =====

/**
 * Rule 3-2-1: Validate Unit card can be placed in Battle Area
 * @param Card Card instance to validate
 * @param PlayerState Player attempting to place the card
 * @return Validation result with rule reference
 */
UFUNCTION(BlueprintCallable, Category = "Comprehensive Rules|Section 3")
FGCGRulesValidationResult ValidateRule_3_2_1_UnitPlacement(
    const FGCGCardInstance& Card,
    const AGCGPlayerState* PlayerState) const;
```

**Implementation (.cpp):**
```cpp
FGCGRulesValidationResult UGCGComprehensiveRulesSubsystem::ValidateRule_3_2_1_UnitPlacement(
    const FGCGCardInstance& Card,
    const AGCGPlayerState* PlayerState) const
{
    // Rule 3-2-1: Unit cards are placed in the Battle Area

    UGCGCardDatabase* CardDB = GetCardDatabase();
    const FGCGCardData* CardData = CardDB->GetCardData(Card.CardNumber);

    if (CardData->CardType != EGCGCardType::Unit)
    {
        return FGCGRulesValidationResult(
            false,
            TEXT("3-2-1"),
            TEXT("Only Unit cards can be placed in Battle Area")
        );
    }

    // Check battle area has space (max 6 units)
    if (PlayerState->BattleArea.Num() >= 6)
    {
        return FGCGRulesValidationResult(
            false,
            TEXT("3-2-1"),
            TEXT("Battle Area is full (max 6 units)")
        );
    }

    return FGCGRulesValidationResult(true, TEXT("3-2-1"));
}
```

### Step 3: Use Validation in Relevant Subsystems

**Example in `GCGPlayerActionSubsystem::PlayCard()`:**

```cpp
// Rule 3-2-1: Validate card can be placed
UGCGComprehensiveRulesSubsystem* RulesSubsystem =
    GetWorld()->GetGameInstance()->GetSubsystem<UGCGComprehensiveRulesSubsystem>();

FGCGRulesValidationResult ValidationResult =
    RulesSubsystem->ValidateRule_3_2_1_UnitPlacement(CardInstance, PlayerState);

if (!ValidationResult.bIsValid)
{
    return FGCGPlayerActionResult(false,
        FString::Printf(TEXT("[Rule %s] %s"),
            *ValidationResult.RuleNumber,
            *ValidationResult.ErrorMessage));
}
```

### Step 4: Update Documentation

Mark the rule as implemented in:
1. `Section_XX_Name.md` - Change status to ✅ Implemented
2. `README.md` - Update integration status table
3. Add commit reference

---

## Benefits of This Approach

### ✅ No Breaking Changes
- Core types (`GCGTypes.h`) remain unchanged
- Existing card data remains valid
- No migration required

### ✅ Maintainability
- Rules documentation in one place (markdown files)
- Validation logic in one subsystem
- Easy to find and update

### ✅ Traceability
- Every validation method references specific rule number
- Error messages include rule references
- Clear link between code and official rules

### ✅ Testability
- Each rule has dedicated validation method
- Easy to unit test individual rules
- Can test all rules comprehensively

### ✅ Modularity
- Can implement rules section-by-section
- No dependencies between sections
- Can update one section without affecting others

---

## Examples from Existing Code

### Good Example: Validation Subsystem Approach

**Current implementation in `GCGValidationSubsystem::ValidateDeck()`:**
```cpp
// FAQ Q2 / Rule 2-4-3: Deck color restriction
TSet<EGCGCardColor> DeckColorsSet;
for (const EGCGCardColor& Color : DeckList.DeckColors)
{
    if (Color != EGCGCardColor::Colorless)
    {
        DeckColorsSet.Add(Color);
    }
}

if (DeckColorsSet.Num() > 2)
{
    Result.AddError(FString::Printf(
        TEXT("[Rule 2-4-3] Deck has too many colors: %d (max 2)"),
        DeckColorsSet.Num()));
}
```

**Better modular approach:**
```cpp
// Use ComprehensiveRulesSubsystem for validation
UGCGComprehensiveRulesSubsystem* RulesSubsystem = GetGameInstance()->GetSubsystem<UGCGComprehensiveRulesSubsystem>();

FGCGRulesValidationResult ColorValidation =
    RulesSubsystem->ValidateRule_2_4_3_DeckColors(DeckList.DeckColors);

if (!ColorValidation.bIsValid)
{
    Result.AddError(FString::Printf(TEXT("[Rule %s] %s"),
        *ColorValidation.RuleNumber,
        *ColorValidation.ErrorMessage));
}
```

---

## What NOT to Do

### ❌ Don't Modify Core Enums

**Bad:**
```cpp
// In GCGTypes.h
enum class EGCGCardType : uint8
{
    // ...
    NewType  // Adding new type - BREAKING CHANGE!
};
```

**Good:**
```cpp
// In ComprehensiveRulesSubsystem
// Add validation method instead - no enum change needed
FGCGRulesValidationResult ValidateRule_X_Y_NewTypeRule(const FGCGCardInstance& Card) const
{
    // Validate using existing card data
}
```

### ❌ Don't Duplicate Rules Text in Code

**Bad:**
```cpp
/**
 * Rule 3-2-1: When a Unit card is played, it is placed in the Battle Area.
 * The Battle Area can hold a maximum of 6 Unit cards at a time.
 * If the Battle Area is full, players cannot play additional Unit cards
 * until space becomes available through cards being destroyed or moved.
 */
bool CanPlayUnit() { ... }
```

**Good:**
```cpp
/**
 * Rule 3-2-1: Unit card placement validation
 * See Documentation/ComprehensiveRules/Section_03_CardTypes.md
 */
bool CanPlayUnit() { ... }
```

### ❌ Don't Scatter Validation Logic

**Bad:**
```cpp
// In 5 different files:
// File1.cpp: Check battle area size
// File2.cpp: Check card type
// File3.cpp: Check player Lv
// File4.cpp: Check cost
// File5.cpp: Check color
```

**Good:**
```cpp
// In ComprehensiveRulesSubsystem.cpp:
// All validation methods in one place
FGCGRulesValidationResult ValidateRule_X_Y_Z(...) { ... }
FGCGRulesValidationResult ValidateRule_X_Y_W(...) { ... }
```

---

## Migration Plan for Existing Code

The existing validation in `GCGValidationSubsystem` can be gradually migrated to use `GCGComprehensiveRulesSubsystem`:

1. **Phase 1** (Current): Both systems coexist
   - `GCGValidationSubsystem`: Handles deck validation
   - `GCGComprehensiveRulesSubsystem`: Handles comprehensive rules

2. **Phase 2** (Future): Gradual migration
   - Move validation methods to `GCGComprehensiveRulesSubsystem`
   - Keep `GCGValidationSubsystem` for backward compatibility
   - Update callers to use new subsystem

3. **Phase 3** (Long-term): Consolidation
   - Deprecate old validation methods
   - All rules validation through comprehensive rules subsystem
   - Single source of truth

---

## Quick Reference

| Task | Location | Tool |
|------|----------|------|
| Official rules text | `Documentation/ComprehensiveRules/Section_XX_Name.md` | Markdown |
| Validation methods | `GCGComprehensiveRulesSubsystem.h/.cpp` | C++ |
| Rule references | Inline comments: `// Rule X-Y-Z` | Comments |
| Integration status | `Documentation/ComprehensiveRules/README.md` | Markdown |
| Template | `Documentation/ComprehensiveRules/TEMPLATE_Section.md` | Markdown |

---

## Summary

**Old Approach (Sections 1-2):**
- Modify core types ⚠️
- Breaking changes ⚠️
- Scattered validation ⚠️

**New Approach (Sections 3-13):**
- Document in markdown ✅
- Validate in subsystem ✅
- Reference in comments ✅
- No breaking changes ✅

**Result:**
- Easier to maintain
- Easier to test
- Easier to extend
- No data migration needed
