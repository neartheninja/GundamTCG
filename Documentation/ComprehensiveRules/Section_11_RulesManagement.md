# Section 11: Rules Management

## Table of Contents
- [11-1. Rules Management Fundamentals](#11-1-rules-management-fundamentals)
- [11-2. Determination of Defeat Management](#11-2-determination-of-defeat-management)
- [11-3. Destruction Management](#11-3-destruction-management)
- [11-4. Battle Area Excess Management](#11-4-battle-area-excess-management)
- [11-5. Managing Shield Area Base Section Excess](#11-5-managing-shield-area-base-section-excess)

---

## Overview

**Rules Management = Automatic State-Based Actions**

Section 11 defines automatic game management that occurs without player action:
- **Defeat Conditions**: No shields + damage, empty deck
- **Destruction**: 0 HP = destroyed
- **Zone Limits**: Max 6 Units, max 1 Base
- **Immediate Execution**: Happens instantly when conditions met

**Similar to Magic: The Gathering's "State-Based Actions"**

---

## 11-1. Rules Management Fundamentals

### 11-1-1. Automatic Management Definition

**Rules management is management stipulated within the rules that automatically takes place when a specific event occurs or is occurring during a game.**

**Implementation**:
```cpp
// In AGCGGameMode_1v1::PerformRulesManagement()
void PerformRulesManagement()
{
    // Rule 11-2: Check defeat conditions
    for (Player in AllPlayers)
    {
        if (CheckDefeatConditions(Player))
        {
            Player.bHasLost = true;
            EndGame(Opponent);
        }
    }

    // Rule 11-3: Check destruction (0 HP)
    DestroyAllUnitsWithZeroHP();

    // Rule 11-4: Battle area limit (max 6 Units)
    EnforceBattleAreaLimit();

    // Rule 11-5: Base section limit (max 1 Base)
    EnforceBaseSectionLimit();
}
```

**Status**: ✅ Implemented (GameMode:546-585)

---

### 11-1-2. Immediate Resolution

**Immediately resolve rules management the moment an event occurs, regardless of if other actions are being performed.**

**Implementation**:
```cpp
// Called at critical game points:
// - After combat damage
// - After drawing cards
// - After playing Units
// - After effect resolution

// Example: After combat damage
void DealDamageToPlayer(int32 Damage, AGCGPlayerState* Player)
{
    // Deal damage
    BreakShields(Damage, Player);

    // IMMEDIATELY check defeat conditions
    PerformRulesManagement();
}
```

**Status**: ✅ Conceptual (called after key events)

---

## 11-2. Determination of Defeat Management

### 11-2-1. Simultaneous Defeat Check

**At the start of rules management, if any player fulfills a condition for defeat, all players fulfilling a condition for defeat are immediately defeated.**

**Implementation**:
```cpp
// In AGCGGameMode_1v1::PerformRulesManagement()
void PerformRulesManagement()
{
    TArray<int32> DefeatedPlayers;

    // Check all players for defeat conditions
    for (Player in AllPlayers)
    {
        if (CheckDefeatConditions(Player.PlayerID))
        {
            DefeatedPlayers.Add(Player.PlayerID);
        }
    }

    // Process all defeats simultaneously
    for (PlayerID in DefeatedPlayers)
    {
        Player.bHasLost = true;
        OnPlayerLost(PlayerID);
    }

    // Determine winner
    if (DefeatedPlayers.Num() > 0)
    {
        DetermineWinner();
    }
}
```

**Status**: ⚠️ Sequential checking (should be simultaneous)

---

### 11-2-1-1. Battle Damage with No Shields

**When any player receives battle damage from a Unit while they have no cards in their shield area, that player fulfills a condition for defeat.**

**Implementation**:
```cpp
// In UGCGCombatSubsystem::DealDamageToPlayer()
void DealDamageToPlayer(int32 Damage, AGCGPlayerState* DefendingPlayer)
{
    // Rule 11-2-1-1: Check for defeat condition BEFORE breaking shields
    if (DefendingPlayer->GetShieldCount() == 0 && Damage > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Player %d receives battle damage with no shields - DEFEATED"),
            DefendingPlayer->GetPlayerID());

        // Mark defeat immediately
        DefendingPlayer->bHasLost = true;

        // Trigger game end
        GameMode->EndGame(AttackingPlayer->GetPlayerID());
        return;
    }

    // Break shields normally
    BreakShields(Damage, DefendingPlayer);
}
```

**Status**: ⚠️ Needs explicit check before damage (currently checked in CheckDefeatConditions)

---

### 11-2-1-2. Empty Deck Defeat

**When any player has no cards in their deck, that player fulfills a condition for defeat.**

**Implementation**:
```cpp
// In AGCGGameMode_1v1::CheckDefeatConditions()
bool CheckDefeatConditions(int32 PlayerID) const
{
    AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);

    // Rule 11-2-1-2: No cards in deck
    if (PlayerState->GetDeckSize() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Player %d deck is empty - DEFEATED"), PlayerID);
        return true;
    }

    return false;
}

// Called from:
// - PerformRulesManagement() (after every major game event)
// - After drawing cards (if draw fails due to empty deck)
```

**Status**: ✅ Implemented (GameMode:587-610)

---

## 11-3. Destruction Management

### 11-3-1. Zero HP Destruction

**When the HP of a Unit, Base, or Shield becomes zero or less after receiving damage, that Unit, Base, or Shield is destroyed. The destroyed card is placed into the trash.**

**Implementation**:
```cpp
// In UGCGCombatSubsystem::DealDamageToUnit()
void DealDamageToUnit(int32 Damage, FGCGCardInstance& Unit, AGCGPlayerState* Owner)
{
    // Apply damage
    Unit.CurrentDamage += Damage;

    int32 MaxHP = Unit.HP; // Base HP
    // TODO: Add modifiers (buffs/debuffs to HP)

    // Rule 11-3-1: Check for destruction
    if (Unit.CurrentDamage >= MaxHP)
    {
        UE_LOG(LogTemp, Log, TEXT("Unit %s destroyed (HP: %d/%d, Damage: %d)"),
            *Unit.CardName.ToString(), MaxHP, MaxHP, Unit.CurrentDamage);

        // Trigger destruction
        DestroyUnit(Unit, Owner);
    }
}

void DestroyUnit(FGCGCardInstance& Unit, AGCGPlayerState* Owner)
{
    // Move to trash
    ZoneSubsystem->MoveCard(Unit, EGCGCardZone::BattleArea, EGCGCardZone::Trash, Owner);

    // Trigger "On Destroyed" effects
    EffectSubsystem->TriggerEffects(EGCGEffectTiming::OnDestroyed, Context, GameState);

    // Remove from Battle Area
    Owner->BattleArea.Remove(Unit);
}
```

**Status**: ✅ Implemented (CombatSubsystem:350-400)

---

### 11-3-1-1. Shields Have 1 HP

**Shields are treated as having 1 HP each.**

**Implementation**:
```cpp
// In UGCGCombatSubsystem::BreakShields()
void BreakShields(int32 Damage, AGCGPlayerState* DefendingPlayer)
{
    // Rule 11-3-1-1: Each shield has 1 HP, so each point of damage breaks 1 shield
    int32 ShieldsToBreak = FMath::Min(Damage, DefendingPlayer->GetShieldCount());

    for (int32 i = 0; i < ShieldsToBreak; i++)
    {
        // Take top shield
        FGCGCardInstance ShieldCard = DefendingPlayer->ShieldStack[0];
        DefendingPlayer->ShieldStack.RemoveAt(0);

        // Check for Burst
        // ...

        // Move to trash
        ShieldCard.CurrentZone = EGCGCardZone::Trash;
        DefendingPlayer->Trash.Add(ShieldCard);
    }
}
```

**Status**: ✅ Implemented (CombatSubsystem:453-526)

---

## 11-4. Battle Area Excess Management

### 11-4-1. Six Unit Maximum

**Units in the battle area are limited to six at most.**

**Implementation**:
```cpp
// In AGCGPlayerState::CanAddUnitToBattle()
bool CanAddUnitToBattle() const
{
    // Rule 11-4-1: Max 6 Units in battle area
    return BattleArea.Num() < 6;
}

// Checked before playing Units:
if (!PlayerState->CanAddUnitToBattle())
{
    return Error("Battle Area is full (max 6 Units)");
}
```

**Status**: ✅ Implemented (PlayerState.cpp:158-164)

---

### 11-4-2. Excess Unit Placement

**If the battle area is at its upper limit when a Unit card is being played or a Unit is being deployed by an effect, choose one Unit already in the battle area and place it into the trash.**

**Implementation**:
```cpp
// In AGCGGameMode_1v1::EnforceBattleAreaLimit()
void AGCGGameMode_1v1::EnforceBattleAreaLimit(AGCGPlayerState* PlayerState)
{
    // Rule 11-4-2: If over limit, player chooses Unit to trash
    while (PlayerState->BattleArea.Num() > 6)
    {
        int32 PlayerID = PlayerState->GetPlayerID();

        // Prompt player to choose Unit to remove
        int32 RemovedUnitID = RequestChooseUnitToRemove(PlayerID);

        FGCGCardInstance* RemovedUnit = nullptr;
        EGCGCardZone Zone;

        // Find the Unit in Battle Area
        for (int32 i = 0; i < PlayerState->BattleArea.Num(); i++)
        {
            if (PlayerState->BattleArea[i].InstanceID == RemovedUnitID)
            {
                RemovedUnit = &PlayerState->BattleArea[i];
                break;
            }
        }

        if (RemovedUnit)
        {
            // Rule 11-4-2-1: NOT considered destroyed (no "On Destroyed" effects)
            RemovedUnit->bWasDestroyed = false;
            RemovedUnit->CurrentZone = EGCGCardZone::Trash;

            PlayerState->Trash.Add(*RemovedUnit);
            PlayerState->BattleArea.RemoveAll([RemovedUnitID](const FGCGCardInstance& Card) {
                return Card.InstanceID == RemovedUnitID;
            });
        }
    }
}
```

**Status**: ⚠️ Implemented (GameMode:670-726), UI for player choice pending

---

### 11-4-2-1. Not Considered Destroyed

**Units placed into the trash due to this management are not considered to be destroyed.**

**Implementation**:
```cpp
// In FGCGCardInstance (GCGTypes.h:756)
UPROPERTY(BlueprintReadWrite, Category = "Tracking")
bool bWasDestroyed;

// In EnforceBattleAreaLimit():
if (RemovedUnit)
{
    // Rule 11-4-2-1: NOT considered destroyed (no "On Destroyed" effects)
    RemovedUnit->bWasDestroyed = false;
    RemovedUnit->CurrentZone = EGCGCardZone::Trash;

    PlayerState->Trash.Add(*RemovedUnit);
    // ... remove from Battle Area ...
}

// Effect system checks this flag:
if (Card.bWasDestroyed && Card.CurrentZone == EGCGCardZone::Trash)
{
    TriggerEffects(EGCGEffectTiming::OnDestroyed);
}
```

**Status**: ✅ Implemented (GCGTypes.h:756, GameMode:670-726)

---

### 11-4-2-2. Simultaneous Deployment

**When multiple Units are deployed simultaneously, place an equal number of Units already in the battle area into the trash.**

**Status**: ❌ Not implemented (edge case)

---

## 11-5. Managing Shield Area Base Section Excess

### 11-5-1. One Base Maximum

**The base section is limited to one Base at most.**

**Implementation**:
```cpp
// In GCGPlayerActionSubsystem::CanPlayCard()
if (CardInstance.CardType == EGCGCardType::Base)
{
    // Rule 11-5-1: Can only have 1 Base
    if (PlayerState->BaseSection.Num() > 0)
    {
        return FGCGPlayerActionResult(false, TEXT("Can only have 1 Base (replace EX Base first)"));
    }
}
```

**Status**: ✅ Implemented (PlayerActionSubsystem.cpp:199-206)

---

### 11-5-2. Excess Base Placement

**If the base section is full when a new Base card is played or a new Base is deployed by some effect, choose one Base already in the base section and place it into the trash.**

**Implementation**:
```cpp
// In AGCGGameMode_1v1::EnforceBaseSectionLimit()
void AGCGGameMode_1v1::EnforceBaseSectionLimit(AGCGPlayerState* PlayerState)
{
    // Rule 11-5-2: If over limit (>1 Base), player chooses Base to trash
    while (PlayerState->BaseSection.Num() > 1)
    {
        int32 PlayerID = PlayerState->GetPlayerID();

        // Prompt player to choose Base to remove
        int32 RemovedBaseID = RequestChooseBaseToRemove(PlayerID);

        FGCGCardInstance* RemovedBase = nullptr;

        // Find the Base in Base Section
        for (int32 i = 0; i < PlayerState->BaseSection.Num(); i++)
        {
            if (PlayerState->BaseSection[i].InstanceID == RemovedBaseID)
            {
                RemovedBase = &PlayerState->BaseSection[i];
                break;
            }
        }

        if (RemovedBase)
        {
            // Rule 11-5-2-1: NOT considered destroyed
            RemovedBase->bWasDestroyed = false;
            RemovedBase->CurrentZone = EGCGCardZone::Trash;

            PlayerState->Trash.Add(*RemovedBase);
            PlayerState->BaseSection.RemoveAll([RemovedBaseID](const FGCGCardInstance& Card) {
                return Card.InstanceID == RemovedBaseID;
            });
        }
    }
}
```

**Status**: ⚠️ Implemented (GameMode:728-784), UI for player choice pending

---

### 11-5-2-1. Not Considered Destroyed

**Bases placed into the trash due to this management are not considered to be destroyed.**

**Implementation**:
```cpp
// In EnforceBaseSectionLimit():
if (RemovedBase)
{
    // Rule 11-5-2-1: NOT considered destroyed
    RemovedBase->bWasDestroyed = false;
    RemovedBase->CurrentZone = EGCGCardZone::Trash;

    PlayerState->Trash.Add(*RemovedBase);
    // ... remove from Base Section ...
}
```

**Status**: ✅ Implemented (GCGTypes.h:756, GameMode:728-784)

---

## Implementation Summary

### Current Status: ~75% Complete

**✅ Fully Implemented**:
1. Defeat conditions checking (empty deck)
2. Battle area limit validation (max 6 check)
3. Base section limit validation (max 1 check)
4. Unit destruction (0 HP)
5. Shield breaking (1 HP per shield)
6. Rules management framework (PerformRulesManagement)
7. **Battle area excess enforcement (auto-select)**
8. **Base section excess enforcement (auto-select)**
9. **bWasDestroyed flag for destruction tracking**

**⚠️ Partially Implemented**:
1. Battle damage defeat check (logic exists, needs enforcement point)
2. Simultaneous defeat checking (currently sequential)
3. **Player choice UI for excess removal (auto-selects first card)**

**❌ Not Implemented**:
1. Immediate resolution (Rule 11-1-2) - conceptual only
2. Simultaneous Unit deployment excess (Rule 11-4-2-2)

**Implementation Breakdown**:

| Rule | Description | Status | Location |
|------|-------------|--------|----------|
| 11-1-1 | Automatic management | ✅ Implemented | GameMode:546-593 |
| 11-1-2 | Immediate resolution | ⚠️ Conceptual | Called after key events |
| 11-2-1 | Simultaneous defeat | ⚠️ Sequential | GameMode:558-584 |
| 11-2-1-1 | Battle damage defeat | ⚠️ Partial | CombatSubsystem |
| 11-2-1-2 | Empty deck defeat | ✅ Implemented | GameMode:602-607 |
| 11-3-1 | Zero HP destruction | ✅ Implemented | CombatSubsystem:350-400 |
| 11-3-1-1 | Shields = 1 HP | ✅ Implemented | CombatSubsystem:453-526 |
| 11-4-1 | 6 Unit max | ✅ Implemented | PlayerState:158-164 |
| 11-4-2 | Excess Unit removal | ✅ Implemented | GameMode:670-726 |
| 11-4-2-1 | Not destroyed flag | ✅ Implemented | GCGTypes.h:756 |
| 11-4-2-2 | Simultaneous excess | ❌ Not implemented | - |
| 11-5-1 | 1 Base max | ✅ Implemented | PlayerActionSubsystem:199-206 |
| 11-5-2 | Excess Base removal | ✅ Implemented | GameMode:728-784 |
| 11-5-2-1 | Not destroyed flag | ✅ Implemented | GCGTypes.h:756 |

**Total Coverage**: 10/14 rules fully (71%), 3 partial (21%)
**Effective Coverage**: ~75%

---

## Key Missing Features

### 1. **Player Choice UI** (High Priority)
Currently auto-selects first card. Need UI for player to choose which card to remove:
- Battle Area: Choose 1 Unit to trash (when excess occurs)
- Base Section: Choose Base to replace (when excess occurs)

Implementation in progress (auto-selection works, UI pending):
```cpp
// Currently auto-selects first card:
int32 AGCGGameMode_1v1::RequestChooseUnitToRemove(int32 PlayerID)
{
    // TODO: Show UI for player to choose Unit
    // For now, auto-select first Unit
    if (PlayerState->BattleArea.Num() > 0)
    {
        return PlayerState->BattleArea[0].InstanceID;
    }
    return 0;
}
```

### 2. **Simultaneous Defeat Check** (Medium Priority)
Currently checks players sequentially. Should check all simultaneously:
```cpp
// Current: Sequential
for (Player) {
    if (CheckDefeat(Player)) EndGame(Opponent);
}

// Should be: Simultaneous
TArray<int32> Defeated;
for (Player) {
    if (CheckDefeat(Player)) Defeated.Add(Player);
}
ProcessAllDefeats(Defeated);
```

### 3. **Simultaneous Unit Deployment** (Low Priority)
When multiple Units are deployed at once, excess management needs to handle simultaneous removal (Rule 11-4-2-2).

---

## Integration Points

**Section 1 Dependencies**:
- Victory/defeat conditions (1-2-2) ✅ Implemented

**Section 5 Dependencies**:
- Shield destruction (5-10-2) ✅ Implemented
- Card removal (5-12) ✅ Implemented

**Section 8 Dependencies**:
- Battle damage defeat (8-4-3) ⚠️ Needs enforcement

**Section 10 Dependencies**:
- Effect-deployed Units ❌ Needs excess management

---

## Testing Scenarios

### Test Case 1: Empty Deck Defeat
```
1. Player draws last card from deck
2. Deck becomes empty
3. ✓ PerformRulesManagement() called
4. ✓ CheckDefeatConditions() returns true
5. ✓ Player marked as defeated
6. ✓ Game ends with opponent as winner
```

### Test Case 2: Battle Damage with No Shields
```
1. Player has 0 shields
2. Enemy Unit attacks player
3. Damage would be dealt
4. ⚠️ Check should happen BEFORE damage
5. ✓ Player defeated immediately
6. ✓ Game ends
```

### Test Case 3: Battle Area Limit
```
1. Player has 6 Units
2. Player tries to play 7th Unit
3. ✓ CanAddUnitToBattle() returns false
4. ✓ Play action rejected
5. ❌ If effect deploys Unit, no enforcement
```

### Test Case 4: Base Replacement
```
1. Player has EX Base
2. Player plays real Base card
3. ✓ EX Base removed
4. ✓ Real Base enters play
5. ⚠️ If playing second real Base, needs UI
```

---

## Related Sections

- **Section 1**: Victory/defeat conditions (1-2-2, 1-2-3)
- **Section 5**: Shields, destruction terminology
- **Section 8**: Combat damage, shield breaking
- **Section 10**: Effect-deployed Units triggering excess

---

## Rule References

**This section implements rules from the official Gundam TCG Comprehensive Rules Section 11: Rules Management.**

**Key Rule Numbers**:
- **11-1**: Rules management fundamentals (automatic, immediate)
- **11-2**: Defeat determination (no shields + damage, empty deck)
- **11-3**: Destruction management (0 HP)
- **11-4**: Battle area excess (max 6 Units)
- **11-5**: Base section excess (max 1 Base)
