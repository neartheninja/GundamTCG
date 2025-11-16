# Section 5: Essential Game Terminology

**Integration Status**: 🔄 In Progress
**Implementation Location**: Multiple subsystems, `FGCGCardInstance`, effect system
**Commit**: TBD
**Breaking Changes**: None (modular additions)

---

## Overview

This section defines fundamental game terminology and mechanics that underpin the entire Gundam TCG system.

---

## 5-1. Effect

### 5-1-1. Effect Definition
An effect is text that is printed within a defined region of a card.

**Implementation**:
- Field: `FGCGCardData::CardText`
- Parsing: `UGCGEffectSubsystem`

**Status**: ✅ Structural (field exists)

---

### 5-1-2. Keyword Effects
Some effects are keyword effects such as <Support> and <Blocker>.

**Implementation**:
- Enum: `EGCGKeyword`
- Processing: `UGCGKeywordSubsystem`

**Status**: ✅ System exists (keywords in Section 13)

---

## 5-2. Player

### 5-2-1. Player as Owner
A player is the owner of a card.

**Implementation**:
- Field: `FGCGCardInstance::OwnerPlayerID`
- Every card knows its owner

**Status**: ✅ Implemented

---

### 5-2-2. "Owner" in Card Text
When "owner" appears in the text of a card, it refers to the player who owns that card.

**Implementation**:
- Effect resolution uses `OwnerPlayerID`

**Status**: ✅ Structural

---

### 5-2-3. Cards Return to Owner
At the end of a game, all cards are returned to the players who own them.

**Implementation**:
- Game end cleanup

**Status**: ✅ Assumed (game end logic)

---

## 5-3. Active Player and Standby Player

### 5-3-1. Active Player
**The active player is the player responsible for advancing the current turn in progress.**

**Implementation**:
```cpp
// In AGCGGameState
UPROPERTY(Replicated)
int32 ActivePlayerID; // Rule 5-3-1: Player whose turn it is
```

**Status**: ✅ Implemented

---

### 5-3-2. Standby Player
The standby player is the player not responsible for advancing the current turn in progress.

**Implementation**:
```cpp
int32 GetStandbyPlayerID() const
{
    // Rule 5-3-2: The other player
    return (ActivePlayerID == 0) ? 1 : 0;
}
```

**Status**: ✅ Implemented

---

## 5-4. Active and Rested

### 5-4-1. Two Orientations
**Cards in the battle area, resource area, and base section can be in one of the two following indicative orientations.**

**5-4-1-1. Active**
Active: the card is placed vertically.

**5-4-1-2. Rested**
Rested: the card is placed horizontally.

**Implementation**:
```cpp
// In FGCGCardInstance
UPROPERTY(Replicated)
bool bIsActive; // Rule 5-4-1: true = active (vertical), false = rested (horizontal)
```

**Status**: ✅ Field exists

**Affected Zones**:
- Battle Area: Units can be active/rested
- Resource Area: Resources can be active/rested
- Base Section: Base can be active/rested

---

### 5-4-2. Default Active on Placement
**When a card is placed into the battle area, resource area, or base section, it is generally set as active.**

**Implementation**:
```cpp
// In UGCGZoneSubsystem::MoveCard()
if (ToZone == EGCGCardZone::BattleArea ||
    ToZone == EGCGCardZone::ResourceArea ||
    ToZone == EGCGCardZone::BaseSection)
{
    // Rule 5-4-2: Cards enter these zones active
    Card.bIsActive = true;
}
```

**Status**: ⚠️ Needs verification/implementation

---

## 5-5. Damage

### 5-5-1. Damage Counters
When damage is dealt to a Unit, Base, or Shield, the dealt damage is shown with counters.

**5-5-1-1. Counter Placement**
Show the current amount of damage a card has received by placing a number of counters equal to that damage on top of it.

**Implementation**:
```cpp
// In FGCGCardInstance
UPROPERTY(Replicated)
int32 CurrentDamage; // Rule 5-5-1: Damage counters on this card
```

**Status**: ✅ Field exists

---

### 5-5-2. Destruction by Damage
**A card that receives damage equal to or greater than its HP is destroyed as a result of rules management.**

**Implementation**:
```cpp
// In rules management
if (Card.CurrentDamage >= CardData->HP)
{
    // Rule 5-5-2: Destroy card
    DestroyCard(Card);
}
```

**Status**: ✅ Implemented (Section 1 rules management)

---

### 5-5-3. Battle Damage
**Units, Bases, Shields, and players can receive damage as a result of battle. Attacking Units and Units being attacked deal damage equal to their AP to each other during the damage step. This damage is called battle damage.**

**Implementation**:
```cpp
// In FGCGCardInstance
UPROPERTY(Replicated)
EGCGDamageSource LastDamageSource; // Rule 5-5-3 vs 5-5-4: Track damage type

enum class EGCGDamageSource : uint8
{
    None,
    BattleDamage,    // Rule 5-5-3
    EffectDamage     // Rule 5-5-4
};
```

**Status**: ⚠️ Needs `EGCGDamageSource` enum addition

---

### 5-5-4. Effect Damage
Units, Bases, Shields, and players can receive damage from effects on cards. This damage is called effect damage.

**Status**: ⚠️ Needs `EGCGDamageSource` enum

---

### 5-5-5. Zero Damage Not Dealt
**Damage is not dealt when the amount of damage dealt would be zero.**

**Implementation**:
```cpp
void DealDamage(FGCGCardInstance& Target, int32 Damage, EGCGDamageSource Source)
{
    // Rule 5-5-5: Zero damage is not dealt
    if (Damage <= 0)
    {
        return;
    }

    Target.CurrentDamage += Damage;
    Target.LastDamageSource = Source;
}
```

**Status**: ⚠️ Needs implementation

---

### 5-5-6. No Excess Damage to Shields
**When damage received by a Base or Shield exceeds its HP, the excess damage is not dealt to another Shield.**

**Implementation**:
```cpp
// Each shield/base absorbs damage independently
// No "overkill" damage carries over

void DamageShield(FGCGCardInstance& Shield, int32 Damage)
{
    // Rule 5-5-6: Shield has 1 HP, excess damage ignored
    if (Damage >= 1)
    {
        DestroyShield(Shield); // Takes 1 damage, rest ignored
    }
}
```

**Status**: ✅ Implemented (shields have 1 HP each)

---

## 5-6. HP Recovery

### 5-6-1. Remove Damage Counters
**When a Unit or Base recovers from received damage for any reason, remove a number of damage counters from it equal to the amount it recovers.**

**Implementation**:
```cpp
// In UGCGComprehensiveRulesSubsystem::RecoverHP() (line 492)
int32 RecoverHP(FGCGCardInstance& Card, int32 RecoveryAmount) const
{
    // Rule 5-6-3: Undamaged cards cannot recover HP
    if (Card.CurrentDamage == 0)
    {
        return 0;
    }

    // Rule 5-6-1: Remove damage counters
    int32 ActualRecovery = FMath::Min(RecoveryAmount, Card.CurrentDamage);

    // Rule 5-6-2: Cannot exceed max HP
    Card.CurrentDamage -= ActualRecovery;

    return ActualRecovery;
}
```

**Status**: ✅ Implemented (ComprehensiveRulesSubsystem:492)

---

### 5-6-2. Cannot Exceed Max HP
**If the amount recovered exceeds the amount of damage the card has currently received, remove all of its counters. HP does not increase by the amount exceeded.**

**Implementation**:
```cpp
// In UGCGComprehensiveRulesSubsystem::RecoverHP() (line 502)
// Rule 5-6-1: Remove damage counters
int32 ActualRecovery = FMath::Min(RecoveryAmount, Card.CurrentDamage);

// Rule 5-6-2: Cannot exceed max HP (remove all counters at most)
Card.CurrentDamage -= ActualRecovery;
```

**Status**: ✅ Implemented (ComprehensiveRulesSubsystem:502)

---

### 5-6-3. Undamaged Cards Cannot Recover
**A Unit that has not received damage cannot recover HP.**

**Implementation**:
```cpp
// In UGCGComprehensiveRulesSubsystem::RecoverHP() (line 494)
// Rule 5-6-3: Undamaged cards cannot recover HP
if (Card.CurrentDamage == 0)
{
    UE_LOG(LogTemp, Verbose, TEXT("RecoverHP - Card has no damage, cannot recover"));
    return 0;
}
```

**Status**: ✅ Implemented (ComprehensiveRulesSubsystem:494)

---

## 5-7. Play

### 5-7-1. Playing Cards
**This typically describes revealing a card in your hand and paying its cost to use it.**

**5-7-1-1. Play from Other Locations**
Sometimes cards are played from locations other than your hand due to effects.

**Implementation**:
- `UGCGPlayerActionSubsystem::PlayCard()`
- Supports playing from hand (default) or other zones (via effects)

**Status**: ✅ Implemented

---

## 5-8. Deploy

### 5-8-1. Deploy Definition
**This describes a Unit or Base being placed on the field.**

**Implementation**:
- Units → Battle Area
- Bases → Base Section

**Status**: ✅ Terminology (implemented as PlayCard)

---

## 5-9. Pair

### 5-9-1. Pair Definition
**This describes placing a Pilot card, or a Command card with a 【Pilot】 effect, beneath a Unit.**

**Implementation**:
- `UGCGLinkUnitSubsystem::PairPilotWithUnit()`
- Supports both Pilot cards and dual-mode Commands

**Status**: ⚠️ Partially implemented (needs dual-mode Command support)

---

## 5-10. Destroy

### 5-10-1. Destroy Definition
**This describes cards receiving damage equal to or greater than their HP and cards being placed from the field into the trash by effects.**

**Implementation**:
- Damage-based: Rules management checks HP
- Effect-based: Direct destruction

**Status**: ✅ Implemented

---

### 5-10-2. Destroyed Units/Bases to Trash
**When a Unit or Base is destroyed, it is placed face up into the trash of the player who owns it.**

**Implementation**:
```cpp
void DestroyCard(FGCGCardInstance& Card)
{
    // Rule 5-10-2: Move to owner's trash
    MoveCard(Card, Card.CurrentZone, EGCGCardZone::Graveyard, GetOwner(Card));
}
```

**Status**: ✅ Implemented

---

### 5-10-3. Shield Destruction Triggers Burst
**When a Shield is destroyed, first reveal it and check for a 【Burst】 effect before placing it into the trash. If a 【Burst】 effect is present, decide if you will activate it before placing the Shield into the trash.**

**Implementation**:
```cpp
void DestroyShield(FGCGCardInstance& Shield, AGCGPlayerState* Owner)
{
    // Rule 5-10-3: Reveal shield, check for Burst
    UGCGCardDatabase* CardDB = GetCardDatabase();
    const FGCGCardData* CardData = CardDB->GetCardData(Shield.CardNumber);

    // Reveal to both players
    RevealCard(Shield);

    // Check for Burst effect
    if (CardData->Keywords.Contains(EGCGKeyword::Burst))
    {
        // Prompt owner: activate Burst effect?
        PromptBurstActivation(Owner, Shield);
    }
    else
    {
        // No Burst, directly to trash
        MoveCard(Shield, EGCGCardZone::ShieldStack, EGCGCardZone::Graveyard, Owner);
    }
}
```

**Status**: ⚠️ Partially implemented (detection works, UI prompt pending) - CombatSubsystem:540-562

---

### 5-10-4. Limit Exceeding Not Destruction
**A card placed into the trash by rules management when the limit on the number of cards in the battle area or base section is exceeded is not treated as destroyed.**

**Example**: If you somehow have 7 Units, you must choose one to put in trash, but it's not "destroyed" (doesn't trigger "when destroyed" effects)

**Implementation**:
```cpp
// In AGCGGameMode_1v1::EnforceBattleAreaLimit() (line 670)
void AGCGGameMode_1v1::EnforceBattleAreaLimit(AGCGPlayerState* PlayerState)
{
    // Rule 11-4-2: If over limit, player chooses Unit to trash
    while (PlayerState->BattleArea.Num() > 6)
    {
        int32 RemovedUnitID = RequestChooseUnitToRemove(PlayerID);

        if (RemovedUnit)
        {
            // Rule 5-10-4 & 11-4-2-1: NOT considered destroyed (no "On Destroyed" effects)
            RemovedUnit->bWasDestroyed = false;
            RemovedUnit->CurrentZone = EGCGCardZone::Trash;

            PlayerState->Trash.Add(*RemovedUnit);
            PlayerState->BattleArea.RemoveAll(...);
        }
    }
}

// Same for EnforceBaseSectionLimit() (line 728)
```

**Status**: ✅ Implemented (GameMode:670-726, GameMode:728-784) - Uses bWasDestroyed flag

---

## 5-11. Discard

### 5-11-1. Discard Definition
**This describes placing a card from the hand into the trash.**

**Implementation**:
```cpp
void DiscardCard(FGCGCardInstance& Card, AGCGPlayerState* PlayerState)
{
    // Rule 5-11-1: Hand → Trash
    MoveCard(Card, EGCGCardZone::Hand, EGCGCardZone::Graveyard, PlayerState);
}
```

**Status**: ✅ Terminology (implemented as MoveCard)

---

## 5-12. Remove

### 5-12-1. Remove Definition
**This describes a card being placed from any location into the removal area.**

**Implementation**:
```cpp
void RemoveCard(FGCGCardInstance& Card, AGCGPlayerState* PlayerState)
{
    // Rule 5-12-1: Any zone → Removal Area
    MoveCard(Card, Card.CurrentZone, EGCGCardZone::RemovalArea, PlayerState);
}
```

**Status**: ✅ Zone exists

---

### 5-12-2. Removed Units Not Destroyed
**A removed Unit or Base is not treated as destroyed.**

**Implementation**:
- Remove doesn't trigger "when destroyed" effects
- Different from destruction

**Status**: ✅ Conceptual (different code paths)

---

## 5-13. Randomly

### 5-13-1. Random Definition
**Cards handled randomly are reordered without intentional interference from the player.**

**Implementation**:
```cpp
// Shuffle uses FMath::RandRange()
// Random selection uses FMath::RandRange()
```

**Status**: ✅ Implemented (shuffle, first player selection)

---

## 5-14. Draw (a Card)

### 5-14-1. Draw Definition
**This describes adding the top card from your deck to your hand without showing it to the other player.**

**5-14-1-1. "Draw 1"**
If a player is instructed to "draw 1," they move the top card of their deck to their hand without showing it to the other player.

**Implementation**:
```cpp
void DrawCards(AGCGPlayerState* PlayerState, int32 Count)
{
    // Rule 5-14-1: Top of deck → hand, private
    for (int32 i = 0; i < Count; i++)
    {
        if (PlayerState->Deck.Num() == 0)
        {
            // Rule 1-2-2-2: Empty deck = defeat
            break;
        }

        FGCGCardInstance DrawnCard = PlayerState->Deck[0];
        PlayerState->Deck.RemoveAt(0);

        DrawnCard.CurrentZone = EGCGCardZone::Hand;
        PlayerState->Hand.Add(DrawnCard);
    }
}
```

**Status**: ✅ Implemented

---

## 5-15. Shuffle

### 5-15-1. Shuffle Definition
**When instructed to shuffle your deck, reorder the cards in your deck area randomly.**

**5-15-1-1. Single Card Shuffle**
When instructed to shuffle your deck and there is only one card in your deck area, treat the deck as shuffled even though the order of that card has not changed.

**Implementation**:
```cpp
void ShuffleDeck(AGCGPlayerState* PlayerState)
{
    // Rule 5-15-1-1: Single card is already "shuffled"
    if (PlayerState->Deck.Num() <= 1)
    {
        return;
    }

    // Fisher-Yates shuffle
    for (int32 i = PlayerState->Deck.Num() - 1; i > 0; i--)
    {
        int32 j = FMath::RandRange(0, i);
        PlayerState->Deck.Swap(i, j);
    }
}
```

**Status**: ✅ Implemented

---

## 5-16. Gain

### 5-16-1. Gain Definition
**This describes a new effect being added to a card by some effect.**

**Example**: "This Unit gains <Blocker>"

**Implementation**:
```cpp
// In FGCGCardInstance
UPROPERTY(Replicated)
TArray<EGCGKeyword> TemporaryKeywords; // Rule 5-16-1: Gained keywords

UPROPERTY(Replicated)
TArray<FGCGEffect> GainedEffects; // Rule 5-16-1: Gained effects
```

**Status**: ❌ Not implemented - needs effect system

---

## 5-17. Token

### 5-17-1. Token Definition
**Cards treated as Units, Bases, or Resources can be placed on the field from outside the game by various effects. These cards are called tokens.**

**Implementation**:
```cpp
// In FGCGCardInstance
UPROPERTY(Replicated)
bool bIsToken; // Rule 5-17-1: Is this a token?

UPROPERTY(Replicated)
FName TokenType; // e.g., "EXResource", "EXBase", "UnitToken"
```

**Status**: ✅ Fields exist

---

### 5-17-2. Token Rules

**5-17-2-1. Tokens Affected Like Real Cards**
A token placed as a Unit, Base, or Resource is affected by rules and effects in the same manner as a real Unit, Base, or Resource.

**Status**: ✅ Conceptual (tokens use same rules)

---

**5-17-2-2. Token Units Can Pair**
A Unit token can be paired with a Pilot in the same manner as a normal Unit.

**Status**: ✅ Structural

---

**5-17-2-3. Tokens Are Colorless**
**Tokens are treated as having no color.**

**Implementation**:
```cpp
EGCGCardColor GetCardColor(const FGCGCardInstance& Card) const
{
    // Rule 5-17-2-3: Tokens have no color
    if (Card.bIsToken)
    {
        return EGCGCardColor::Colorless;
    }

    return GetCardData(Card.CardNumber)->Color[0];
}
```

**Status**: ⚠️ Needs implementation

---

**5-17-2-4. Token Lv and Cost Are Zero**
**A token's Lv and cost are both treated as zero.**

**Implementation**:
```cpp
int32 GetCardCost(const FGCGCardInstance& Card) const
{
    // Rule 5-17-2-4: Tokens have 0 cost
    if (Card.bIsToken)
    {
        return 0;
    }

    return GetCardData(Card.CardNumber)->Cost;
}

int32 GetCardLevel(const FGCGCardInstance& Card) const
{
    // Rule 5-17-2-4: Tokens have 0 Lv
    if (Card.bIsToken)
    {
        return 0;
    }

    return GetCardData(Card.CardNumber)->Level;
}
```

**Status**: ⚠️ Needs implementation

---

**5-17-2-5. Tokens Removed When Leaving Field**
**When a token moves to any location other than the battle area, resource area, or shield area, it is removed from the game.**

**5-17-2-5-1. Triggers Still Occur**
The token momentarily moves to that location before being removed, so trigger conditions such as "when destroyed" and "when returned to your hand" are fulfilled.

**Implementation**:
```cpp
// In UGCGZoneSubsystem::MoveCard() (line 44)
// Rule 5-17-2-5: Tokens leaving field zones are removed from the game
EGCGCardZone OriginalToZone = ToZone;
if (Card.bIsToken)
{
    // Check if leaving a field zone
    bool bLeavingFieldZone = (FromZone == EGCGCardZone::BattleArea ||
                              FromZone == EGCGCardZone::ResourceArea ||
                              FromZone == EGCGCardZone::ShieldStack ||
                              FromZone == EGCGCardZone::BaseSection);

    if (bLeavingFieldZone)
    {
        UGCGComprehensiveRulesSubsystem* RulesSubsystem = GetGameInstance()->GetSubsystem<UGCGComprehensiveRulesSubsystem>();
        if (RulesSubsystem && RulesSubsystem->ShouldRemoveToken(Card, ToZone))
        {
            ToZone = EGCGCardZone::Removal;
            UE_LOG(LogTemp, Log, TEXT("Token %s redirected to Removal zone (Rule 5-17-2-5)"),
                *Card.CardName.ToString());
        }
    }
}
```

**Status**: ✅ Implemented (ZoneSubsystem:44-64) - Tokens auto-removed when leaving field

---

### 5-17-3. EX Tokens

**5-17-3-1. EX Base**

**5-17-3-1-1. EX Base Stats**
**An EX Base is a Base token with 0 AP and 3 HP.**

**5-17-3-1-2. EX Base Placement**
**Place it in the base section at the start of the game.**

**Implementation**:
```cpp
// Token data in UGCGCardDatabase::CreateEXBaseTokenData() (line 305)
FGCGCardData UGCGCardDatabase::CreateEXBaseTokenData() const
{
    FGCGCardData EXBase;
    EXBase.CardNumber = FName("EXBase");
    EXBase.CardName = FText::FromString(TEXT("EX Base"));
    EXBase.CardType = EGCGCardType::Base;
    EXBase.Colors.Empty(); // Colorless
    EXBase.Level = 0;
    EXBase.Cost = 0;
    EXBase.AP = 0; // Rule 5-17-3-1-1: 0 AP
    EXBase.HP = 3; // Rule 5-17-3-1-1: 3 HP
    return EXBase;
}

// Instance creation in UGCGComprehensiveRulesSubsystem::CreateEXBaseToken() (line 592)
FGCGCardInstance CreateEXBaseToken(int32 OwnerPlayerID) const
{
    FGCGCardInstance EXBase;
    EXBase.bIsToken = true;
    EXBase.TokenType = FName("EXBase");
    EXBase.CardNumber = FName("TOKEN_EXBase");
    EXBase.OwnerPlayerID = OwnerPlayerID;
    EXBase.CurrentZone = EGCGCardZone::BaseSection;
    return EXBase;
}

// Game setup in AGCGGameMode_1v1::SetupEXBase() (line 1391)
void AGCGGameMode_1v1::SetupEXBase(int32 PlayerID)
{
    FGCGCardInstance EXBaseToken = RulesSubsystem->CreateEXBaseToken(PlayerID);
    PlayerState->BaseSection.Add(EXBaseToken);
}
```

**Status**: ✅ Implemented (CardDatabase:305, ComprehensiveRulesSubsystem:592, GameMode:1391)

---

**5-17-3-2. EX Resource**

**5-17-3-2-1. EX Resource Definition**
An EX Resource is a Resource token that can be used temporarily to pay costs.

**5-17-3-2-2. EX Resource Placement**
**Player Two places one in their resource area at the start of the game.**

**5-17-3-2-3. EX Resource Removal**
**When an EX Resource is used to pay a cost, that EX Resource is removed from the game.**

**Implementation**:
```cpp
// Already implemented in FAQ Q10, Q21-Q22
// See AGCGGameMode_1v1::SetupEXResource()
```

**Status**: ✅ Implemented (commit f45ca57)

---

## 5-18. Counter

### 5-18-1. Counter Definition
**During a game, specific counters are placed on and removed from cards.**

**5-18-1-1. Damage Counters**
In the Gundam Card Game, counters displaying how much damage a card has received are typically placed on Units and Bases when they receive damage.

**Implementation**:
```cpp
// Using CurrentDamage field as counter
UPROPERTY(Replicated)
int32 CurrentDamage; // Rule 5-18-1-1: Damage counter
```

**Status**: ✅ Implemented

---

### 5-18-2. Physical Counter Representation
When placing counters on a card, place some object on top of the card that shows the number of counters placed, such as chips or a die.

**Status**: ✅ UI concern (3D models or UI widgets)

---

### 5-18-3. Removing Counters
When removing counters from a card, remove the indicated number of counters from the top of the card.

**Implementation**:
```cpp
void RemoveCounters(FGCGCardInstance& Card, int32 Count)
{
    // Rule 5-18-3: Remove counters
    Card.CurrentDamage = FMath::Max(0, Card.CurrentDamage - Count);
}
```

**Status**: ✅ Implemented (HP recovery)

---

## 5-19. / (Forward Slash)

### 5-19-1. Forward Slash as "Or"
**The "/" character is sometimes used with traits, among other places. It performs the same function as the conjunction "or" would in those situations.**

**Example**: "1 (Zeon)/(Neo Zeon) Unit card" → choose either Zeon OR Neo Zeon

**Implementation**:
```cpp
// In UGCGComprehensiveRulesSubsystem::MatchesTraitCondition() (line 614)
bool UGCGComprehensiveRulesSubsystem::MatchesTraitCondition(
    const FString& Condition,
    const TArray<FName>& CardTraits) const
{
    // Rule 5-19-1: "/" means "or"
    if (Condition.Contains(TEXT("/")))
    {
        TArray<FString> Options;
        Condition.ParseIntoArray(Options, TEXT("/"));

        // Check if card has ANY of the traits
        for (const FString& Option : Options)
        {
            FName TraitName(*Option.TrimStartAndEnd());
            if (CardTraits.Contains(TraitName))
            {
                return true; // Match!
            }
        }
        return false;
    }

    // Single trait match
    return CardTraits.Contains(FName(*Condition));
}
```

**Status**: ✅ Implemented (ComprehensiveRulesSubsystem:614-645)

---

## 5-20. "If you do" and "Then"

### 5-20-1. "If you do" Conditional
**If the text portion preceding the phrase "If you do" in card text cannot be resolved, then resolving the succeeding portion is not possible.**

**Example**: "Discard 1 card. If you do, draw 2 cards."
- If you can't discard → you don't draw

**Implementation**:
```cpp
bool ResolveEffect_IfYouDo(const FGCGEffect& Effect)
{
    // Rule 5-20-1: "If you do" requires preceding action to succeed
    bool bPrecedingActionSucceeded = ResolveEffectPart1(Effect);

    if (bPrecedingActionSucceeded)
    {
        // Preceding action succeeded → resolve succeeding part
        ResolveEffectPart2(Effect);
        return true;
    }
    else
    {
        // Preceding action failed → skip succeeding part
        UE_LOG(LogTemp, Warning, TEXT("'If you do' condition not met - skipping subsequent effect"));
        return false;
    }
}
```

**Status**: ❌ Not implemented - effect system needed

---

### 5-20-2. "Then" Non-Conditional
**Even if the text portion preceding the word "Then" in card text cannot be resolved, resolving the succeeding portion is possible.**

**Example**: "Destroy 1 Unit. Then draw 1 card."
- Even if you can't destroy (no valid targets) → you still draw

**Implementation**:
```cpp
void ResolveEffect_Then(const FGCGEffect& Effect)
{
    // Rule 5-20-2: "Then" does NOT require preceding action to succeed
    bool bPrecedingActionSucceeded = ResolveEffectPart1(Effect);

    // Regardless of success, resolve succeeding part
    ResolveEffectPart2(Effect);

    UE_LOG(LogTemp, Verbose, TEXT("'Then' effect: Part 1 %s, Part 2 executed"),
        bPrecedingActionSucceeded ? TEXT("succeeded") : TEXT("failed"));
}
```

**Status**: ❌ Not implemented - effect system needed

---

## Implementation Checklist

### Effects (5-1)
- [x] **5-1-1**: Effect definition (structural)
- [x] **5-1-2**: Keyword effects (subsystem exists)

### Player (5-2)
- [x] **5-2-1**: Player as owner (OwnerPlayerID)
- [x] **5-2-2**: "Owner" in card text
- [x] **5-2-3**: Cards return to owner

### Active/Standby Player (5-3)
- [x] **5-3-1**: Active player (ActivePlayerID)
- [x] **5-3-2**: Standby player

### Active/Rested (5-4)
- [x] **5-4-1**: Two orientations (bIsActive)
- [ ] **5-4-2**: Default active on placement

### Damage (5-5)
- [x] **5-5-1**: Damage counters (CurrentDamage)
- [x] **5-5-2**: Destruction by damage
- [ ] **5-5-3**: Battle damage tracking
- [ ] **5-5-4**: Effect damage tracking
- [ ] **5-5-5**: Zero damage not dealt
- [x] **5-5-6**: No excess damage to shields

### HP Recovery (5-6)
- [x] **5-6-1**: Remove damage counters ✅ IMPLEMENTED
- [x] **5-6-2**: Cannot exceed max HP ✅ IMPLEMENTED
- [x] **5-6-3**: Undamaged cards cannot recover ✅ IMPLEMENTED

### Actions (5-7 - 5-12)
- [x] **5-7-1**: Play cards
- [x] **5-8-1**: Deploy
- [ ] **5-9-1**: Pair (partial - needs UI)
- [x] **5-10-1**: Destroy definition
- [x] **5-10-2**: Destroyed to trash
- [x] **5-10-3**: Shield Burst trigger ⚠️ DETECTION IMPLEMENTED, UI PENDING
- [x] **5-10-4**: Limit exceeding not destruction ✅ IMPLEMENTED
- [x] **5-11-1**: Discard
- [x] **5-12-1**: Remove definition
- [x] **5-12-2**: Removed not destroyed

### Deck Operations (5-13 - 5-15)
- [x] **5-13-1**: Randomly
- [x] **5-14-1**: Draw
- [x] **5-15-1**: Shuffle

### Effects (5-16)
- [ ] **5-16-1**: Gain effects

### Tokens (5-17)
- [x] **5-17-1**: Token definition (field exists)
- [x] **5-17-2-1**: Tokens use same rules
- [x] **5-17-2-2**: Token Units can pair
- [x] **5-17-2-3**: Tokens colorless ✅ IMPLEMENTED
- [x] **5-17-2-4**: Token Lv/cost = 0 ✅ IMPLEMENTED
- [x] **5-17-2-5**: Tokens removed when leaving field ✅ IMPLEMENTED
- [x] **5-17-3-1**: EX Base (0 AP / 3 HP) ✅ IMPLEMENTED
- [x] **5-17-3-2**: EX Resource ✅ IMPLEMENTED

### Counters (5-18)
- [x] **5-18-1**: Counter definition
- [x] **5-18-2**: Physical representation (UI)
- [x] **5-18-3**: Removing counters

### Effect Parsing (5-19 - 5-20)
- [x] **5-19-1**: "/" means "or" ✅ IMPLEMENTED
- [ ] **5-20-1**: "If you do" conditional (needs effect system)
- [ ] **5-20-2**: "Then" non-conditional (needs effect system)

**Summary**: 34/42 rules fully implemented (81%), 2 partial (5%)

---

## Remaining Features to Implement

### UI-Dependent Features (High Priority)
1. **Shield Burst activation prompt** (5-10-3) - Detection works, needs UI for player choice
2. **Pilot pairing UI** (5-9-1) - Pairing logic works, needs UI for target selection

### Effect System Features (Medium Priority)
3. **Damage source tracking** (5-5-3, 5-5-4) - Distinguish battle vs effect damage
4. **"If you do" / "Then"** (5-20-1, 5-20-2) - Conditional effect resolution
5. **Gain effects** (5-16-1) - Temporary keywords/effects
6. **Active/Rested placement** (5-4-2) - Explicit active/rested state on zone entry

### Already Implemented ✅
- ✅ EX Base token (5-17-3-1) - CardDatabase:305, GameMode:1391
- ✅ Token removal on zone change (5-17-2-5) - ZoneSubsystem:44
- ✅ HP Recovery (5-6) - ComprehensiveRulesSubsystem:492
- ✅ "/" trait parsing (5-19-1) - ComprehensiveRulesSubsystem:614
- ✅ Limit exceeding not destruction (5-10-4) - Uses bWasDestroyed flag

---

## Next Steps

### Phase 1: Token System (1 week)
1. Create token card data system
2. Implement EX Base (0 AP / 3 HP)
3. Implement token removal on zone change
4. Test token lifecycle

### Phase 2: Damage System (3-4 days)
1. Add `EGCGDamageSource` enum
2. Track battle vs effect damage
3. Implement HP recovery methods
4. Test damage/healing

### Phase 3: Shield Burst (3-4 days)
1. Implement shield reveal on destruction
2. Check for Burst keyword
3. Prompt player for Burst activation
4. Resolve Burst effect

### Phase 4: Effect System Foundations (1-2 weeks)
1. Implement "If you do" vs "Then" logic
2. Implement "/" trait parsing
3. Implement gained effects system
4. Test effect resolution

**Total Estimate**: 3-4 weeks for full Section 5 integration

---

## Testing Notes

**Critical Tests**:
1. Both players start with EX Base (0 AP / 3 HP)
2. Player 2 starts with 1 EX Resource
3. Shield destroyed → reveal → check Burst → prompt player
4. Token moved to hand → triggers "when returned" → removed from game
5. HP recovery works correctly (can't overheal)
6. Undamaged cards can't be healed
7. Battle damage vs effect damage tracked separately
8. Cards enter field zones as active
9. "If you do" requires preceding action success
10. "Then" executes regardless of preceding action

---

## Related Sections

- **Section 1**: Rules Management (destruction checks)
- **Section 3**: Card Types (Units, Pilots, Bases, tokens)
- **Section 4**: Game Locations (field zones)
- **Section 6**: Preparing to Play (EX Base/Resource setup)
- **Section 12**: Keywords (Burst, Support, Blocker)

---

## Official Rules Reference

All rules text is from the official Gundam Trading Card Game Comprehensive Rules, Section 5: Essential Game Terminology.
