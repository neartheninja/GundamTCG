# Section 2: Card Information

**Integration Status**: ✅ Implemented
**Implementation Location**: `GCGTypes.h` (FGCGCardData structure)
**Commit**: f1e24c6
**⚠️ Breaking Change**: Color enum modified (Black/Yellow → Purple)

---

## Overview

All cards have the following information printed on them. This section explains each piece of information.

---

## 2-1. Card Number

Each card has a card number, which is a serial number unique to that card. The card number is written in the top-right and bottom-left of the card.

### Rule 2-1-2. Deck Construction Limit
A deck can include up to four cards with the same card number.

**Implementation**:
- Location: `UGCGValidationSubsystem::ValidateDeck()`
- Validation: Enforces maximum 4 copies per card number
- Field: `FGCGCardData::CardNumber` (FName)

```cpp
// 2-1-2: Max 4 copies of any card (by card number)
TMap<FName, int32> CardCounts;
for (const FName& CardNumber : DeckList.MainDeck)
{
    CardCounts.FindOrAdd(CardNumber)++;
    if (CardCounts[CardNumber] > 4)
    {
        Result.AddError(FString::Printf(TEXT("Too many copies of card %s: %d (max 4)"),
            *CardNumber.ToString(), CardCounts[CardNumber]));
    }
}
```

---

## 2-2. Card Name

Each card has a card name.

### Rule 2-2-5. Multiple Cards with Same Name
The game does not impose a limit on the number of cards with the same card name that can exist on the field at the same time.

**Implementation**:
- Field: `FGCGCardData::CardName` (FText)
- No limit enforced on field
- Used for card search effects and text matching

---

## 2-3. Card Type

Cards are divided into the following five card types: "Unit," "Pilot," "Command," "Base," and "Resource."

**Implementation**:
- Location: `GCGTypes.h`
- Enum: `EGCGCardType`
- Field: `FGCGCardData::CardType`

```cpp
/**
 * Card Types
 * Comprehensive Rules 2-3: Card Type
 * There are five card types: Unit, Pilot, Command, Base, Resource
 */
UENUM(BlueprintType)
enum class EGCGCardType : uint8
{
    Unit        UMETA(DisplayName = "Unit"),       // 2-3: Placed in Battle Area
    Pilot       UMETA(DisplayName = "Pilot"),      // 2-3: Links to Unit cards
    Command     UMETA(DisplayName = "Command"),    // 2-3: One-time effects
    Base        UMETA(DisplayName = "Base"),       // 2-3: Placed in Base Section
    Resource    UMETA(DisplayName = "Resource")    // 2-3: Placed in Resource Area
};
```

---

## 2-4. Color

### Rule 2-4-1. Card Colors
Unit cards, Pilot cards, Command cards, and Base cards have one or more card colors.

### Rule 2-4-2-1. Five Official Colors
**There are five card colors: blue, green, red, white, and purple.**

### Rule 2-4-2. Resource Cards Have No Color
Resource cards and tokens do not have a color.

**⚠️ BREAKING CHANGE**:
- Previous implementation had 7 colors: Black, Yellow, White, Blue, Green, Red, Colorless
- Updated to match official rules: **5 colors only** (White, Blue, Green, Red, Purple)
- Removed: `EGCGCardColor::Black`, `EGCGCardColor::Yellow`
- Added: `EGCGCardColor::Purple`
- Impact: Existing card data using Black/Yellow needs migration

**Implementation**:
- Location: `GCGTypes.h`
- Enum: `EGCGCardColor`
- Field: `FGCGCardData::Color` (TArray<EGCGCardColor>)

```cpp
/**
 * Card Colors
 * Comprehensive Rules 2-4: Color
 * 2-4-2-1: There are five card colors: blue, green, red, white, and purple
 * 2-4-2: Resource cards and tokens have no color
 */
UENUM(BlueprintType)
enum class EGCGCardColor : uint8
{
    White           UMETA(DisplayName = "White"),
    Blue            UMETA(DisplayName = "Blue"),
    Green           UMETA(DisplayName = "Green"),
    Red             UMETA(DisplayName = "Red"),
    Purple          UMETA(DisplayName = "Purple"),    // Official color (replaces Black/Yellow)
    Colorless       UMETA(DisplayName = "Colorless")  // Resources and Tokens only
};
```

### Rule 2-4-3. Deck Color Restriction
A deck can only include cards of up to two colors (not counting colorless).

**Implementation**:
- Location: `UGCGValidationSubsystem::ValidateDeck()`
- Validation: FAQ Q2 implementation
- Logic: Count unique colors excluding Colorless, max 2

```cpp
// FAQ Q2 / Rule 2-4-3: Deck must have 1-2 colors maximum (not counting Colorless)
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
    Result.AddError(FString::Printf(TEXT("Deck has too many colors: %d (max 2)"),
        DeckColorsSet.Num()));
}
```

---

## 2-5. Trait

Some cards have traits.

### Rule 2-5-3. Pilot Traits Not Added to Units
When a Pilot card and a Unit card are paired, the Pilot card's traits are NOT added to the Unit card.

**Implementation**:
- Field: `FGCGCardData::Traits` (TArray<FName>)
- Used for: Card search effects, trait-specific abilities
- Pilot pairing logic: Pilot traits remain separate from Unit traits

---

## 2-6. Zone

The zone is the location on the field where a card currently is.

**Implementation**:
- Enum: `EGCGCardZone`
- All zones documented in enum definition
- Zone management: `UGCGZoneSubsystem`

---

## 2-7. AP (Attack Points)

AP is an abbreviation of attack points, and it represents the offensive strength of Unit cards and Pilot cards.

### Rule 2-7-2. Pilot AP Modifiers
When a Pilot card is paired with a Unit card, the Pilot card's AP is added to the Unit card's AP.

**Implementation**:
- Field: `FGCGCardData::AP` (int32)
- Pairing logic: Unit AP + Pilot AP when paired
- Combat calculations use combined AP

```cpp
/**
 * 2-7: AP (Attack Points)
 * Offensive strength
 * 2-7-2: When Pilot paired, Pilot's AP is ADDED to Unit's AP
 */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Stats")
int32 AP;
```

---

## 2-8. HP (Hit Points)

HP is an abbreviation of hit points, and it represents a card's defensive strength.

### Rule 2-8-2. Card Destruction
**When a card's HP becomes 0, that card is destroyed.**

### Rule 2-8-3. Pilot HP Modifiers
When a Pilot card is paired with a Unit card, the Pilot card's HP is added to the Unit card's HP.

**Implementation**:
- Field: `FGCGCardData::HP` (int32)
- Instance: `FGCGCardInstance::CurrentDamage` (tracks damage taken)
- Destruction check: `CurrentDamage >= HP`
- Pairing logic: Unit HP + Pilot HP when paired

```cpp
/**
 * 2-8: HP (Hit Points)
 * Defensive strength / health
 * 2-8-2: Card destroyed when HP reaches 0
 * 2-8-3: When Pilot paired, Pilot's HP is ADDED to Unit's HP
 */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Stats")
int32 HP;
```

---

## 2-9. Lv (Level)

Lv is an abbreviation of level, and it indicates what a player's Lv needs to be to play that card.

### Rule 2-9-2. Player Lv Calculation
**A player's Lv is equal to the number of cards in their resource area that are active, plus the number of EX Resource tokens they control.**

**Implementation**:
- Field: `FGCGCardData::Level` (int32)
- Player Lv calculation: `AGCGPlayerState::GetPlayerLv()`
- Validation: `UGCGPlayerActionSubsystem::CanPlayCard()` checks `PlayerLv >= CardLevel`

```cpp
int32 AGCGPlayerState::GetPlayerLv() const
{
    // Rule 2-9-2: Player Lv = Active Resources + EX Resources
    int32 ActiveRegularResources = 0;
    int32 EXResources = 0;

    for (const FGCGCardInstance& Resource : ResourceArea)
    {
        if (Resource.bIsToken && Resource.TokenType == FName("EXResource"))
        {
            EXResources++; // EX Resources always count
        }
        else if (Resource.bIsActive)
        {
            ActiveRegularResources++; // Regular resources only if active
        }
    }

    return ActiveRegularResources + EXResources;
}
```

---

## 2-10. Cost

The cost is the number of cards in your resource area that you need to rest in order to play a card.

**Implementation**:
- Field: `FGCGCardData::Cost` (int32)
- Payment: `UGCGPlayerActionSubsystem::PlayCard()`
- Logic: Rest regular resources first, then remove EX Resources

```cpp
// Pay cost: Rest regular active resources first
for (FGCGCardInstance& Resource : PlayerState->ResourceArea)
{
    if (!Resource.bIsToken && Resource.bIsActive && RemainingCost > 0)
    {
        Resource.bIsActive = false; // Rest the resource
        RemainingCost--;
    }
}

// Then remove EX Resources if needed
for (int32 i = EXResourcesToRemove.Num() - 1; i >= 0; i--)
{
    PlayerState->ResourceArea.RemoveAt(EXResourcesToRemove[i]);
    RemainingCost--;
}
```

---

## 2-11. Card Text

Card text is specific effects that card has.

### Rule 2-11-2. Parenthetical Text
Text written in parentheses is explanatory text used to help players understand the rules. It does not have any effect on gameplay.

**Implementation**:
- Field: `FGCGCardData::CardText` (FText)
- Effect parsing: Strip parenthetical text before processing
- Effect system: To be implemented (future)

---

## 2-12. Link Condition

Only Unit cards have link conditions.

**Implementation**:
- Field: `FGCGCardData::LinkCondition` (FText)
- Validation: Only Unit cards can have link conditions
- Pilot pairing: Checks if Pilot meets Unit's link condition

---

## 2-13. Card Art

The artwork illustrating the card's contents.

**Note**: Not stored in card data, handled by UI/asset system.

---

## 2-14. Illustrator's Name

The name of the person who drew the card art.

**Note**: Not stored in card data structure.

---

## 2-15. Copyright

Copyright information.

**Note**: Not stored in card data structure.

---

## 2-16. Rarity

An indicator of the card's rarity.

**Implementation**:
- Field: `FGCGCardData::Rarity` (EGCGCardRarity enum)
- Used for: Collection tracking, deck building UI
- No gameplay effect

---

## Implementation Checklist

- [x] **2-1**: Card Number stored and validated
- [x] **2-1-2**: Max 4 copies per card enforced
- [x] **2-2**: Card Name stored
- [x] **2-2-5**: No limit on same-name cards on field
- [x] **2-3**: Five card types defined
- [x] **2-4**: Five colors defined ⚠️ BREAKING CHANGE
- [x] **2-4-3**: 1-2 color deck restriction enforced
- [x] **2-5**: Traits stored and tracked
- [x] **2-5-3**: Pilot traits not added to Unit (documented)
- [x] **2-6**: Zones defined and managed
- [x] **2-7**: AP stored and used in combat
- [x] **2-7-2**: Pilot AP added when paired (documented)
- [x] **2-8**: HP stored and destruction at 0 HP
- [x] **2-8-3**: Pilot HP added when paired (documented)
- [x] **2-9**: Level requirement validation
- [x] **2-9-2**: Player Lv = Active Resources + EX Resources
- [x] **2-10**: Cost payment system implemented
- [x] **2-11**: Card text stored
- [x] **2-11-2**: Parenthetical text is explanatory only
- [x] **2-12**: Link condition stored (Unit cards only)
- [x] **2-16**: Rarity stored

---

## Migration Guide: Color Enum Change

If you have existing card data using `Black` or `Yellow` colors:

**Before (7 colors):**
```cpp
EGCGCardColor::Black
EGCGCardColor::Yellow
```

**After (5 official colors):**
```cpp
EGCGCardColor::Purple  // Use Purple for both Black and Yellow cards
```

**Migration Steps:**
1. Identify all cards with `Black` or `Yellow` in card database
2. Update color to `Purple` based on official card references
3. Update deck lists to use Purple instead of Black/Yellow
4. Test deck validation with new color restrictions

**Database Query Example:**
```sql
UPDATE CardData
SET Color = 'Purple'
WHERE Color IN ('Black', 'Yellow');
```
