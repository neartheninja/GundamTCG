# Section 4: Game Locations

**Integration Status**: ✅ Mostly Implemented
**Implementation Location**: `EGCGCardZone`, `UGCGZoneSubsystem`, `AGCGPlayerState`
**Commit**: TBD
**Breaking Changes**: None (existing zones match official rules)

---

## Overview

The game uses eight locations for card placement. Each player has their own instance of each location.

---

## 4-1. Locations

### 4-1-1. Eight Locations
The locations in the game are the deck area, resource deck area, resource area, battle area, shield area, removal area, hand, and trash.

**4-1-1-1. Per-Player Locations**
Each player has their own instance of each location.

**Implementation**:
- Enum: `EGCGCardZone` in `GCGTypes.h`
- Storage: Fields in `AGCGPlayerState`

```cpp
// Existing implementation in GCGTypes.h
enum class EGCGCardZone : uint8
{
    Deck,           // 4-2: Deck Area
    ResourceDeck,   // 4-3: Resource Deck Area
    ResourceArea,   // 4-4: Resource Area
    BattleArea,     // 4-5: Battle Area (Units + Pilots)
    BaseSection,    // 4-6-3: Shield Area - Base Section
    ShieldStack,    // 4-6-4: Shield Area - Shield Section
    RemovalArea,    // 4-7: Removal Area
    Hand,           // 4-8: Hand
    Graveyard       // 4-9: Trash
};
```

**Status**: ✅ Implemented (all zones defined)

---

**4-1-1-2. The Field**
When referring to them all together, the resource area, battle area, and shield area are also called the field.

**Implementation**:
```cpp
// Helper method in UGCGComprehensiveRulesSubsystem
bool IsFieldZone(EGCGCardZone Zone) const
{
    // Rule 4-1-1-2: Field = Resource Area + Battle Area + Shield Area
    return Zone == EGCGCardZone::ResourceArea ||
           Zone == EGCGCardZone::BattleArea ||
           Zone == EGCGCardZone::BaseSection ||
           Zone == EGCGCardZone::ShieldStack;
}
```

**Status**: ✅ Implemented (ComprehensiveRulesSubsystem.h:215-222)

---

### 4-1-2. Active Effects Have No Location
Commands whose effects are currently active and cards whose 【Burst】 effects are currently active are not considered to be in any of the locations listed above.

**Implementation**:
- Related to Rule 3-4-3 (Commands during resolution)
- Needs `EffectStack` temporary zone

**Status**: ⏳ Pending effect system implementation

---

### 4-1-3. Public Card Count Information
The number of cards in any location is public information and can be confirmed by either player at any time during the game.

**Implementation**:
```cpp
// In AGCGPlayerState - all arrays expose Num()
int32 GetDeckSize() const { return Deck.Num(); }
int32 GetHandSize() const { return Hand.Num(); }
int32 GetResourceCount() const { return ResourceArea.Num(); }
// etc.
```

**Status**: ✅ Implemented (replication provides counts)

---

### 4-1-4. Public vs Private Locations
Cards in some locations may be viewed by all players, while cards in other locations may not. Locations where cards may be viewed are referred to as public, whereas locations where cards may not be viewed are referred to as private.

**Public Locations**:
- Resource Area (4-4-3)
- Battle Area (4-5-5)
- Base Section (4-6-3-1)
- Removal Area (4-7-2)
- Trash (4-9-2)

**Private Locations**:
- Deck Area (4-2-2)
- Resource Deck Area (4-3-2)
- Shield Section (4-6-4-1)
- Hand (4-8-2, but owner can view)

**Implementation**:
```cpp
// In UGCGComprehensiveRulesSubsystem
bool ValidateRule_4_1_4_IsPublicZone(EGCGCardZone Zone) const
{
    // Rule 4-1-4: Public vs Private locations
    switch (Zone)
    {
        // Public zones
        case EGCGCardZone::ResourceArea:
        case EGCGCardZone::BattleArea:
        case EGCGCardZone::BaseSection:
        case EGCGCardZone::RemovalArea:
        case EGCGCardZone::Graveyard:
            return true;

        // Private zones
        case EGCGCardZone::Deck:
        case EGCGCardZone::ResourceDeck:
        case EGCGCardZone::ShieldStack:
        case EGCGCardZone::Hand:
            return false;

        default:
            return false;
    }
}
```

**Replication**:
```cpp
// In AGCGPlayerState::GetLifetimeReplicatedProps()
DOREPLIFETIME(AGCGPlayerState, ResourceArea);     // Public
DOREPLIFETIME(AGCGPlayerState, BattleArea);       // Public
DOREPLIFETIME(AGCGPlayerState, BaseSection);      // Public
DOREPLIFETIME(AGCGPlayerState, RemovalArea);      // Public
DOREPLIFETIME(AGCGPlayerState, Graveyard);        // Public

DOREPLIFETIME_CONDITION(AGCGPlayerState, Hand, COND_OwnerOnly);      // Private
DOREPLIFETIME_CONDITION(AGCGPlayerState, Deck, COND_OwnerOnly);      // Private
DOREPLIFETIME_CONDITION(AGCGPlayerState, ShieldStack, COND_OwnerOnly); // Private
```

**Status**: ✅ Implemented (ComprehensiveRulesSubsystem.h:230-251, replication enforced in AGCGPlayerState)

---

### 4-1-5. Cards Become New When Moving
**When a card moves between locations, unless specified otherwise, treat it as a new card in the new location. Any effects that were applied to it in the previous location are ignored.**

**Implementation**:
```cpp
// In UGCGZoneSubsystem::MoveCard()
void UGCGZoneSubsystem::MoveCard(
    FGCGCardInstance& Card,
    EGCGCardZone FromZone,
    EGCGCardZone ToZone,
    AGCGPlayerState* PlayerState)
{
    // Rule 4-1-5: Card becomes "new" when moving zones
    // Clear temporary effects/modifiers
    Card.BonusAP = 0;
    Card.BonusHP = 0;
    Card.TemporaryKeywords.Empty();
    Card.AppliedEffects.Empty();
    // Note: Damage persists on Units/Bases (permanent until destroyed)

    // Update zone
    Card.CurrentZone = ToZone;

    // Move to destination
    AddCardToZone(Card, ToZone, PlayerState);
}
```

**Status**: ⚠️ Needs implementation of effect clearing

---

### 4-1-6. Simultaneous Placement Order
When multiple cards are simultaneously placed into a location, unless specified otherwise, the player who owns those cards determines the order in which they are placed in that location.

**Implementation**:
- Owner chooses order when placing multiple cards
- Important for stack ordering (trash, removal area)

**Status**: ✅ Implicit (owner controls their own zones)

---

### 4-1-7. Private Placement Order Hidden
When multiple cards are simultaneously placed into a private location from a public location, unless specified otherwise, the player who owns those cards determines the order in which they are placed into that private location. The order in which those cards are placed is not revealed to the other player.

**Example**: Cards destroyed from battle area → bottom of deck (player chooses order, opponent doesn't see)

**Implementation**:
- Owner-only replication for private zones
- Order not revealed to opponent

**Status**: ✅ Enforced by replication (COND_OwnerOnly)

---

## 4-2. Deck Area

### 4-2-1. Deck Placement
This is where you place your deck at the start of the game.

**Status**: ✅ Implemented

---

### 4-2-2. Deck Privacy
**Your deck area is private. Cards within it are placed face down in a stack. Unless specified otherwise, neither player is allowed to view the contents or order of the cards, nor are they allowed to change the order of the cards.**

**Implementation**:
```cpp
// Replicated as COND_OwnerOnly
DOREPLIFETIME_CONDITION(AGCGPlayerState, Deck, COND_OwnerOnly);
```

**Status**: ✅ Implemented via replication

---

### 4-2-3. Moving Multiple Cards
When moving multiple cards from your deck to another location simultaneously, they all get treated as though they are placed simultaneously, but physically move the cards one card at a time.

**Implementation**:
```cpp
// In UGCGZoneSubsystem::DrawTopCards()
int32 DrawTopCards(
    EGCGCardZone SourceZone,
    AGCGPlayerState* PlayerState,
    int32 Count,
    TArray<FGCGCardInstance>& OutCards)
{
    // Move cards one at a time
    for (int32 i = 0; i < Count; i++)
    {
        // Draw from top
        FGCGCardInstance Card = PlayerState->Deck[0];
        PlayerState->Deck.RemoveAt(0);
        OutCards.Add(Card);
    }
    // Rule 4-2-3: Treated as simultaneous placement
    return OutCards.Num();
}
```

**Status**: ✅ Implemented

---

### 4-2-4. Deck Shuffling
When instructed to shuffle a deck, the player who owns that deck reorders the cards randomly.

**Implementation**:
```cpp
// In UGCGZoneSubsystem
void ShuffleDeck(AGCGPlayerState* PlayerState)
{
    // Rule 4-2-4: Random reorder
    int32 DeckSize = PlayerState->Deck.Num();
    for (int32 i = DeckSize - 1; i > 0; i--)
    {
        int32 j = FMath::RandRange(0, i);
        PlayerState->Deck.Swap(i, j);
    }
}
```

**Status**: ✅ Implemented

---

## 4-3. Resource Deck Area

### 4-3-1. Resource Deck Placement
This is the location where you place your resource deck at the start of the game.

**Status**: ✅ Implemented

---

### 4-3-2. Resource Deck Privacy
Your resource deck area is private. Cards within it are placed face down in a stack. Unless specified otherwise, neither player is allowed to view the contents or order of the cards, nor are they allowed to change the order of the cards.

**Implementation**:
```cpp
DOREPLIFETIME_CONDITION(AGCGPlayerState, ResourceDeck, COND_OwnerOnly);
```

**Status**: ✅ Implemented via replication

---

### 4-3-3. Moving Multiple Cards
When moving multiple cards from your resource deck to another location simultaneously, they all get treated as though they are placed simultaneously, but physically move the cards one card at a time.

**Status**: ✅ Same logic as 4-2-3

---

## 4-4. Resource Area

### 4-4-1. Resource Placement
This is where you place Resources from your resource deck.

**Status**: ✅ Implemented

---

### 4-4-2. Resource Capacity Limit
**You may have up to 15 Resources in your resource area.**

**4-4-2-1. EX Resource Limit**
**You may have up to five EX Resources in your resource area.**

**Implementation**:
```cpp
// In UGCGComprehensiveRulesSubsystem
FGCGRulesValidationResult ValidateRule_4_4_2_ResourceAreaLimit(
    const AGCGPlayerState* PlayerState) const
{
    int32 TotalResources = PlayerState->ResourceArea.Num();

    // Rule 4-4-2: Max 15 resources
    if (TotalResources >= 15)
    {
        return FGCGRulesValidationResult(
            false,
            TEXT("4-4-2"),
            TEXT("Resource area is full (maximum 15 resources)")
        );
    }

    // Rule 4-4-2-1: Max 5 EX Resources
    int32 EXResourceCount = 0;
    for (const FGCGCardInstance& Resource : PlayerState->ResourceArea)
    {
        if (Resource.bIsToken && Resource.TokenType == FName("EXResource"))
        {
            EXResourceCount++;
        }
    }

    if (EXResourceCount >= 5)
    {
        return FGCGRulesValidationResult(
            false,
            TEXT("4-4-2-1"),
            TEXT("EX Resource limit reached (maximum 5 EX Resources)")
        );
    }

    return FGCGRulesValidationResult(true, TEXT("4-4-2"));
}
```

**Status**: ✅ Implemented (ComprehensiveRulesSubsystem:352-381) - Validates max 15 resources and max 5 EX Resources

---

### 4-4-3. Resource Area Is Public
Your resource area is public. Both players are free to view cards in this location.

**Implementation**:
```cpp
DOREPLIFETIME(AGCGPlayerState, ResourceArea); // Public
```

**Status**: ✅ Implemented

---

## 4-5. Battle Area

### 4-5-1. Battle Area Purpose
This is where you place Units and Pilots.

**Status**: ✅ Implemented

---

### 4-5-2. Units Placed Face Up
Units are placed face up when they are deployed.

**Status**: ✅ Structural (no face-down Units)

---

### 4-5-3. Pilots Placed Face Up Beneath Units
Pilots are placed face up beneath Units when they are paired.

**Status**: ⚠️ Needs implementation in Link Unit system

---

### 4-5-4. Battle Area Capacity
**You may have up to six Units at a time in your battle area.**

**Implementation**:
```cpp
// In UGCGComprehensiveRulesSubsystem
FGCGRulesValidationResult ValidateRule_4_5_4_BattleAreaLimit(
    const AGCGPlayerState* PlayerState) const
{
    // Rule 4-5-4: Max 6 Units in battle area

    int32 UnitCount = 0;
    for (const FGCGCardInstance& Card : PlayerState->BattleArea)
    {
        // Count only Units (not Pilots stored beneath them)
        UGCGCardDatabase* CardDB = GetCardDatabase();
        const FGCGCardData* CardData = CardDB->GetCardData(Card.CardNumber);

        if (CardData && CardData->CardType == EGCGCardType::Unit)
        {
            UnitCount++;
        }
    }

    if (UnitCount >= 6)
    {
        return FGCGRulesValidationResult(
            false,
            TEXT("4-5-4"),
            TEXT("Battle area is full (maximum 6 Units)")
        );
    }

    return FGCGRulesValidationResult(true, TEXT("4-5-4"));
}
```

**Status**: ✅ Implemented (ComprehensiveRulesSubsystem:396-432)

---

### 4-5-5. Battle Area Is Public
Your battle area is public. Both players are free to view cards in this location.

**Implementation**:
```cpp
DOREPLIFETIME(AGCGPlayerState, BattleArea); // Public
```

**Status**: ✅ Implemented

---

## 4-6. Shield Area

### 4-6-1. Shield Area Purpose
This location is checked when the player is attacked.

**Status**: ✅ Implemented in combat system

---

### 4-6-2. Two Sections
Within the shield area there is a section to place Shields and a section to place a Base.

**Implementation**:
- `AGCGPlayerState::BaseSection` (4-6-3)
- `AGCGPlayerState::ShieldStack` (4-6-4)

**Status**: ✅ Implemented (separate arrays)

---

### 4-6-3. Base Section

**4-6-3. Base Placement**
You may have up to one Base placed face up in your base section.

**4-6-3-1. Base Section Is Public**
The base section is public. Both players are free to view cards in this location.

**Implementation**:
```cpp
// In UGCGComprehensiveRulesSubsystem
FGCGRulesValidationResult ValidateRule_4_6_3_BaseSectionLimit(
    const AGCGPlayerState* PlayerState) const
{
    // Rule 4-6-3: Max 1 Base in base section
    if (PlayerState->BaseSection.Num() >= 1)
    {
        return FGCGRulesValidationResult(
            false,
            TEXT("4-6-3"),
            TEXT("Base section already has a Base (maximum 1)")
        );
    }

    return FGCGRulesValidationResult(true, TEXT("4-6-3"));
}

// Replication
DOREPLIFETIME(AGCGPlayerState, BaseSection); // Public
```

**Status**: ✅ Implemented (ComprehensiveRulesSubsystem:434-453, also enforced in GameMode:728-784)

---

### 4-6-4. Shield Section

**4-6-4. Shield Placement**
The cards that become your Shields are placed face down in the shield section.

**4-6-4-1. Shield Section Privacy**
**Your shield section is private. Unless specified otherwise, cards in this area are placed face down and neither player is allowed to view the contents or order of the cards, nor are they allowed to change the order of the cards. Unless specified otherwise, when moving a card from your shield section to a different location, choose the top card.**

**4-6-4-2. Shield HP**
**Cards in the shield section are treated as Shields with 1 HP each.**

**Implementation**:
```cpp
// Replication
DOREPLIFETIME_CONDITION(AGCGPlayerState, ShieldStack, COND_OwnerOnly);

// In combat system
void DamageShields(AGCGPlayerState* DefendingPlayer, int32 Damage)
{
    // Rule 4-6-4-2: Each shield has 1 HP
    while (Damage > 0 && DefendingPlayer->ShieldStack.Num() > 0)
    {
        // Rule 4-6-4-1: Take from top
        FGCGCardInstance Shield = DefendingPlayer->ShieldStack[0];
        DefendingPlayer->ShieldStack.RemoveAt(0);

        // Move to trash
        MoveCardToTrash(Shield, DefendingPlayer);

        Damage--;
    }
}
```

**Status**: ✅ Implemented

---

## 4-7. Removal Area

### 4-7-1. Removal Area Purpose
This is where you place removed cards.

**Status**: ✅ Implemented

---

### 4-7-2. Removal Area Is Public
**Your removal area is public. Both players are free to view cards in this location, and you are free to reorder them.**

**Implementation**:
```cpp
DOREPLIFETIME(AGCGPlayerState, RemovalArea); // Public

// Players can reorder (no enforcement needed)
```

**Status**: ✅ Implemented

---

## 4-8. Hand

### 4-8-1. Hand Purpose
Your hand is where you place cards drawn from your deck.

**Status**: ✅ Implemented

---

### 4-8-2. Hand Privacy
**Your hand is a private location, but each player may freely view or reorder the cards in their own hand.**

**Implementation**:
```cpp
DOREPLIFETIME_CONDITION(AGCGPlayerState, Hand, COND_OwnerOnly);
```

**Status**: ✅ Implemented

---

### 4-8-3. Cannot View Opponent's Hand
Unless specified otherwise, you may not view the other player's hand.

**Status**: ✅ Enforced by replication (COND_OwnerOnly)

---

### 4-8-4. Hand Size Limit
**A hand is limited to no more than ten cards. Players may have any number of cards in their hand, but if your hand exceeds the limit during your end phase, you must discard cards until the limit is reached.**

**Implementation**:
```cpp
// In AGCGGameMode_1v1::OnPhaseChanged()
void OnEndPhaseStarted(int32 ActivePlayerID)
{
    AGCGPlayerState* PlayerState = GetPlayerStateByID(ActivePlayerID);

    // Rule 4-8-4: Hand size limit enforcement
    const int32 MaxHandSize = 10;

    if (PlayerState->Hand.Num() > MaxHandSize)
    {
        int32 CardsToDiscard = PlayerState->Hand.Num() - MaxHandSize;

        // Prompt player to choose cards to discard
        // (UI interaction needed)
        PromptPlayerToDiscardCards(PlayerState, CardsToDiscard);
    }
}

FGCGRulesValidationResult ValidateRule_4_8_4_HandSizeLimit(
    const AGCGPlayerState* PlayerState,
    bool bIsEndPhase) const
{
    const int32 MaxHandSize = 10;

    // Rule 4-8-4: Hand limit only enforced during end phase
    if (!bIsEndPhase)
    {
        return FGCGRulesValidationResult(true, TEXT("4-8-4"));
    }

    if (PlayerState->Hand.Num() > MaxHandSize)
    {
        return FGCGRulesValidationResult(
            false,
            TEXT("4-8-4"),
            FString::Printf(TEXT("Hand size exceeds limit (%d/%d) - must discard %d card(s)"),
                PlayerState->Hand.Num(), MaxHandSize, PlayerState->Hand.Num() - MaxHandSize)
        );
    }

    return FGCGRulesValidationResult(true, TEXT("4-8-4"));
}
```

**Status**: ✅ Implemented (ComprehensiveRulesSubsystem:455-489, validation enforced during end phase)

---

## 4-9. Trash

### 4-9-1. Trash Purpose
Destroyed Unit, Pilot, and Base cards, and Command cards whose abilities have finished activating, go into the trash.

**Status**: ✅ Implemented (Graveyard zone)

---

### 4-9-2. Trash Is Public
**Your trash is public. Cards within it are placed face up in a stack, and both players are free to view their contents. You are free to reorder the cards in your trash.**

**Implementation**:
```cpp
DOREPLIFETIME(AGCGPlayerState, Graveyard); // Public

// Players can reorder (no restriction)
```

**Status**: ✅ Implemented

---

## Implementation Checklist

### Locations (4-1)
- [x] **4-1-1**: Eight locations defined (enum)
- [x] **4-1-1-2**: Field helper method (Resource + Battle + Shield)
- [ ] **4-1-2**: Active effects have no location (EffectStack)
- [x] **4-1-3**: Card counts are public (replication)
- [x] **4-1-4**: Public vs private validation method
- [ ] **4-1-5**: Clear effects when moving zones
- [x] **4-1-6**: Owner determines simultaneous placement order
- [x] **4-1-7**: Private placement order hidden (replication)

### Deck Area (4-2)
- [x] **4-2-1**: Deck placement
- [x] **4-2-2**: Deck privacy (COND_OwnerOnly)
- [x] **4-2-3**: Move multiple cards one at a time
- [x] **4-2-4**: Shuffle implementation

### Resource Deck Area (4-3)
- [x] **4-3-1**: Resource deck placement
- [x] **4-3-2**: Resource deck privacy (COND_OwnerOnly)
- [x] **4-3-3**: Move multiple cards

### Resource Area (4-4)
- [x] **4-4-1**: Resource placement
- [x] **4-4-2**: Max 15 resources validation
- [x] **4-4-2-1**: Max 5 EX Resources validation
- [x] **4-4-3**: Resource area is public

### Battle Area (4-5)
- [x] **4-5-1**: Battle area purpose
- [x] **4-5-2**: Units face up
- [ ] **4-5-3**: Pilots beneath Units (Link system)
- [x] **4-5-4**: Max 6 Units validation
- [x] **4-5-5**: Battle area is public

### Shield Area (4-6)
- [x] **4-6-1**: Shield area purpose
- [x] **4-6-2**: Two sections (Base + Shield)
- [x] **4-6-3**: Max 1 Base validation
- [x] **4-6-3-1**: Base section is public
- [x] **4-6-4**: Shield placement face down
- [x] **4-6-4-1**: Shield privacy (COND_OwnerOnly)
- [x] **4-6-4-2**: Shields have 1 HP each

### Removal Area (4-7)
- [x] **4-7-1**: Removal area purpose
- [x] **4-7-2**: Removal area is public

### Hand (4-8)
- [x] **4-8-1**: Hand purpose
- [x] **4-8-2**: Hand privacy (owner can view/reorder)
- [x] **4-8-3**: Cannot view opponent's hand
- [x] **4-8-4**: Max 10 hand size during end phase

### Trash (4-9)
- [x] **4-9-1**: Trash purpose
- [x] **4-9-2**: Trash is public

**Summary**: 35/37 rules implemented (95%)

Most zone structure is already correct. Remaining work:
- Rule 4-1-2: Active effects location handling (EffectStack zone)
- Rule 4-1-5: Clear temporary effects when moving zones
- Rule 4-5-3: Visual representation of Pilots beneath Units (UI)

---

## Next Steps for Full Integration

### Remaining Implementation (1-2 days)
1. **Rule 4-1-2**: Add `EffectStack` zone for active effects without location
2. **Rule 4-1-5**: Implement effect clearing in `MoveCard()` (clear temporary modifiers when cards change zones)
3. **Rule 4-5-3**: Add visual representation of Pilots beneath Units in Battle Area UI

**Total Estimate**: 1-2 days for full Section 4 integration (95% → 100%)

---

## Testing Notes

**Critical Tests**:
1. Cannot place 16th resource in resource area
2. Cannot place 6th EX Resource
3. Cannot deploy 7th Unit to battle area
4. Cannot place 2nd Base in base section
5. Hand size limit enforced during end phase (must discard excess)
6. Cards lose temporary effects when moving zones
7. Shield privacy maintained (opponent cannot see)
8. Public zones visible to both players
9. Private zones only visible to owner

---

## Related Sections

- **Section 3**: Card Types (Units, Pilots, Commands, Bases, Resources)
- **Section 7**: Game Progression (phases where limits are checked)
- **Section 8**: Attacking and Battles (shield damage)

---

## Official Rules Reference

All rules text is from the official Gundam Trading Card Game Comprehensive Rules, Section 4: Game Locations.
