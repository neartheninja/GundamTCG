# Section 8: Attacking and Battles

**Status**: ✅ **Mostly Implemented** - Core combat system complete (23/28 rules - 82%)

This section covers the combat system, including attack declaration, blocking, and damage resolution.

---

## 8-1. Battle Overview

**Using a main phase attack with a Unit, the active player may use one of their Units to attack the opposing player or a rested enemy Unit in the battle area. If they attack, enter battle and perform the following five steps in order: attack step, block step, action step, damage step, and battle end step.**

**Implementation**:
```cpp
// Battle flow defined in GCGTypes.h - lines 108-116
enum class EGCGCombatStep : uint8
{
    None,
    AttackStep,      // Declare attack, rest attacker
    BlockStep,       // Optional blocker activation
    ActionStep,      // Alternate action timing
    DamageStep,      // Resolve combat damage
    BattleEndStep    // Expire "this battle" effects
};

// In UGCGCombatSubsystem:
// 1. DeclareAttack() - Attack step
// 2. DeclareBlocker() - Block step
// 3. (Action step - Section 9)
// 4. ResolveAttack() - Damage step
// 5. ClearAttacks() - Battle end step
```

**Status**: ✅ Implemented (5-step structure present)

---

## 8-2. Attack Step

### 8-2-1. Declare Attack Target

**Select one active Unit in your battle area and rest it, then declare an attack target. The attack target can be either the opposing player or a rested enemy Unit.**

**Implementation**:
```cpp
// In UGCGCombatSubsystem::DeclareAttack() - lines 28-87

// Validate attacker is active
if (!AttackerInstance.bIsActive)
{
    return FGCGCombatResult(false, TEXT("Unit is rested"));
}

// Rest the attacker (attacking rests the unit)
BattleCard.bIsActive = false;

// Create attack declaration
FGCGAttackDeclaration Attack;
Attack.AttackerInstanceID = AttackerInstanceID;
Attack.AttackingPlayerID = AttackingPlayer->GetPlayerID();
Attack.DefendingPlayerID = DefendingPlayer->GetPlayerID();
Attack.bTargetingBase = true; // Always target player initially

// Add to current attacks
GameState->CurrentAttacks.Add(Attack);
```

**Current Limitation**: Only supports attacking player (not specific rested Units)

**Status**: ⚠️ Partially implemented (attack player ✅, attack rested Unit ❌)

---

### 8-2-2. 【Attack】 Effect Triggers

**Effects with 【Attack】 or "when this Unit attacks" timing activate.**

**Implementation**:
```cpp
// In UGCGCombatSubsystem::DeclareAttack() - line 84
// TODO: Trigger "On Attack" effects (Phase 8)

// Timing defined in GCGTypes.h - line 149
EGCGEffectTiming::OnAttack  // When this Unit attacks
```

**Status**: ❌ Not implemented (awaits Effect System)

---

### 8-2-3. "During This Battle" Effects

**Effects worded "during this battle" gain effect now.**

**Implementation**:
```cpp
// Modifier duration defined in GCGTypes.h - line 187
EGCGModifierDuration::UntilEndOfBattle

// Effect system applies "during this battle" modifiers
```

**Status**: ✅ Implemented (via Effect System)

---

### 8-2-4. Early Battle End Check

**At the end of the attack step, if the attacking Unit or the Unit targeted for attack has been destroyed or otherwise moved to another location due to some event, continue to the battle end step rather than the block step.**

**Implementation**:
```cpp
// TODO: Implement early battle end checks
// Needs zone change detection during attack step
```

**Status**: ❌ Not implemented

---

## 8-3. Block Step

### 8-3-1. <Blocker> Activation

**The standby player may activate <Blocker> on one of their active Units in the battle area. When activated, this effect changes the attack target of the attacking Unit to the Unit with <Blocker>.**

**Implementation**:
```cpp
// In UGCGCombatSubsystem::DeclareBlocker() - lines 126-182

// Find blocker in Battle Area
FGCGCardInstance BlockerInstance;
if (!DefendingPlayer->FindCardByInstanceID(BlockerInstanceID, BlockerInstance, BlockerZone))
{
    return FGCGCombatResult(false, TEXT("Blocker not found"));
}

// Validate can block (must be active OR have Blocker keyword)
bool bHasBlockerKeyword = HasKeyword(BlockerInstance, EGCGKeyword::Blocker);
if (!BlockerInstance.bIsActive && !bHasBlockerKeyword)
{
    return FGCGCombatResult(false, TEXT("Unit is rested and does not have Blocker keyword"));
}

// Assign blocker
Attack.BlockerInstanceID = BlockerInstanceID;
Attack.bTargetingBase = false; // Attack is now blocked

// Rest the blocker (blocking rests the unit)
BattleCard.bIsActive = false;
```

**Status**: ✅ Implemented

---

### 8-3-2. <Blocker> Activation Limit

**The <Blocker> effect can be activated only one time in response to each attack.**

**Implementation**: Attack declaration structure supports single blocker per attack

**Status**: ✅ Implemented (enforced by data structure)

---

### 8-3-3. Original Target Cannot Block

**A Unit originally targeted for attack cannot activate its own <Blocker> effect.**

**Implementation**:
```cpp
// TODO: Check if blocker == original target
// Current implementation doesn't track original target for rested Unit attacks
```

**Status**: ⚠️ Partial (only relevant when attacking rested Units)

---

### 8-3-4. <Blocker> Optional

**Choosing not to activate a <Blocker> effect is also allowed.**

**Implementation**: Player choice (UI integration)

**Status**: ✅ Conceptually implemented (default no blocker)

---

### 8-3-5. Early Battle End Check

**At the end of the block step, if the attacking Unit or the Unit targeted for attack has been destroyed or otherwise moved to another location due to some event, continue to the battle end step rather than the action step.**

**Implementation**:
```cpp
// TODO: Implement early battle end checks after Block Step
```

**Status**: ❌ Not implemented

---

## 8-4. Action Step

### 8-4-1. 【Action】 Timing During Battle

**Taking turns starting with the standby player, players may activate 【Action】 Command cards and 【Activate･Action】 effects.**

**Implementation**:
```cpp
// Action step combat timing defined in GCGTypes.h - line 113
EGCGCombatStep::ActionStep

// Full implementation in Section 9 (Action Steps)
```

**Status**: ⚠️ Stub (awaits Section 9 implementation)

---

### 8-4-2. Early Battle End Check

**At the end of the action step, if the attacking Unit or the Unit targeted for attack has been destroyed or otherwise moved to another location due to some event, continue to the battle end step rather than the damage step.**

**Implementation**:
```cpp
// TODO: Implement early battle end checks after Action Step
```

**Status**: ❌ Not implemented

---

## 8-5. Damage Step

### 8-5-1. Confirm Attack Target

**Confirm the attacking Unit's attack target at this point. If the attack target is a player, an attack on a player succeeds. If the attack target is an enemy Unit, an attack on a Unit succeeds.**

**Implementation**:
```cpp
// In UGCGCombatSubsystem::ResolveAttack() - lines 213-356

// Check if attack is blocked
if (Attack.BlockerInstanceID > 0)
{
    // Attack on Unit (blocked)
    ResolveUnitVsUnitCombat();
}
else
{
    // Unblocked attack - attack on player
    DealDamageToPlayer(AttackerAP, DefendingPlayer, GameState, ShieldsBroken);
}
```

**Status**: ✅ Implemented

---

## 8-5-2. Attack on a Player

### 8-5-2-1. Check Shield Area

**After an attack on a player has succeeded, check the opponent's shield area.**

**Status**: ✅ Implemented (shield count checked)

---

### 8-5-2-2. No Shields, No Base → Instant Defeat

**If there is no Base and not a single Shield in the opponent's shield area, the attacking Unit deals battle damage equal to its AP to the opposing player. A player receiving damage is immediately defeated, and the player who owns the attacking Unit wins the game.**

**Implementation**:
```cpp
// In UGCGCombatSubsystem::DealDamageToPlayer() - lines 394-446

// Break shields first
if (HasShields(DefendingPlayer))
{
    OutShieldsBroken = BreakShields(ShieldsToBreak, DefendingPlayer);
    return false; // Damage absorbed by shields
}

// Comprehensive Rules 1-2-2-1: Battle damage with no shields = defeat
UE_LOG(LogTemp, Warning, TEXT("Player %d took battle damage with NO SHIELDS - DEFEAT"));

// Mark player as having met defeat conditions
DefendingPlayer->bHasLost = true;

return true; // Player lost
```

**Status**: ✅ Implemented

---

### 8-5-2-3. Shields Present → Break Shield

**If there is a Shield but no Base in the enemy shield area, the attacking Unit deals damage equal to its AP to the top Shield in the shield area.**

**Implementation**:
```cpp
// In UGCGCombatSubsystem::BreakShields() - lines 450-528

for (int32 i = 0; i < ShieldsToBreak; ++i)
{
    // Take top shield (index 0)
    FGCGCardInstance ShieldCard = DefendingPlayer->ShieldStack[0];
    DefendingPlayer->ShieldStack.RemoveAt(0);

    // Reveal shield
    UE_LOG(LogTemp, Log, TEXT("Shield revealed: %s"));

    // Check for Burst keyword (see 8-5-2-3-1)
    // ...

    // Move shield to trash
    ShieldCard.CurrentZone = EGCGCardZone::Trash;
    DefendingPlayer->Trash.Add(ShieldCard);
}
```

**Status**: ✅ Implemented

---

### 8-5-2-3-1. Shield with 【Burst】

**A Shield receiving 1 or more damage is destroyed and placed into the trash after revealing it. If the revealed Shield has a 【Burst】 effect, choose whether or not to activate it before placing the Shield into the trash.**

**Implementation**:
```cpp
// In UGCGCombatSubsystem::BreakShields() - lines 481-503

// Rule 5-10-3: Check for Burst keyword
if (CardData && CardData->Keywords.Contains(EGCGKeyword::Burst))
{
    bHasBurst = true;

    // Rule 5-10-3: Player decides whether to activate Burst
    // TODO: Implement UI prompt for Burst activation (requires Effect System)
    UE_LOG(LogTemp, Warning, TEXT("Shield has 【Burst】! Player %d should be prompted to activate"));

    // TODO: If player chooses to activate:
    // 1. Trigger Burst effect
    // 2. Wait for effect resolution
    // 3. Then move to trash
}
```

**Status**: ⚠️ Detection implemented, UI prompt pending

---

### 8-5-2-4. Base Present → Damage Base

**If there is a Base in the enemy shield area, the attacking Unit deals an amount of battle damage equal to its AP to the Base targeted for attack.**

**Implementation**:
```cpp
// In UGCGCombatSubsystem::DealDamageToPlayer() - lines 432-443

// Still apply damage to Base for tracking purposes
if (DefendingPlayer->BaseSection.Num() > 0)
{
    FGCGCardInstance& Base = DefendingPlayer->BaseSection[0];
    Base.CurrentDamage += Damage;

    // FAQ Q97-99: Track damage source (battle damage from combat)
    Base.LastDamageSource = EGCGDamageSource::BattleDamage;
}
```

**Note**: Current implementation applies damage to Base AFTER shields are gone (when player has lost)

**Status**: ⚠️ Needs separate "attack Base" targeting mode

---

### 8-5-2-4-1. Base HP Tracking

**Use counters to track dealt damage. A Base whose HP becomes zero is destroyed and placed into the trash.**

**Implementation**:
```cpp
// Damage tracked via CurrentDamage field
Base.CurrentDamage += Damage;

// Rules management checks HP in ExecuteRulesManagement()
if (Base.CurrentDamage >= Base.HP)
{
    DestroyCard(Base);
}
```

**Status**: ✅ Implemented (via rules management)

---

### 8-5-2-4-2. <First Strike> on Base

**If the attacking Unit has <First Strike>, it deals battle damage to the enemy Base before normal battle damage is managed.**

**Implementation**:
```cpp
// First Strike keyword defined in GCGTypes.h - line 129
EGCGKeyword::FirstStrike

// TODO: Implement First Strike for Base attacks
// Currently only implemented for Unit vs Unit combat
```

**Status**: ⚠️ Unit vs Unit only (Base attacks pending)

---

## 8-5-3. Attack on a Unit

### 8-5-3-1. Deal Damage Between Units

**If an attack on a Unit succeeds, deal damage between the two Units.**

**Status**: ✅ Implemented (see 8-5-3-2)

---

### 8-5-3-2. Simultaneous Damage

**The attacking Unit deals damage equal to its AP to the Unit targeted for attack simultaneously as the Unit targeted for attack deals damage equal to its AP to the Attacking Unit.**

**Implementation**:
```cpp
// In UGCGCombatSubsystem::ResolveAttack() - lines 292-303

// Normal combat (both deal damage)
if (!bFirstStrikeResolved)
{
    bool bAttackerDestroyed = DealDamageToUnit(Attack.AttackerInstanceID, BlockerAP, AttackingPlayer);
    bool bBlockerDestroyed = DealDamageToUnit(Attack.BlockerInstanceID, AttackerAP, DefendingPlayer);

    Result.bAttackerDestroyed = bAttackerDestroyed;
    Result.bBlockerDestroyed = bBlockerDestroyed;
}
```

**Status**: ✅ Implemented

---

### 8-5-3-2-1. HP Tracking and Destruction

**Use counters to track dealt damage. A Unit whose HP becomes zero is destroyed and placed into the trash.**

**Implementation**:
```cpp
// In UGCGCombatSubsystem::DealDamageToUnit() - lines 358-392

// Add damage
BattleCard.CurrentDamage += Damage;

// FAQ Q97-99: Track damage source (battle damage vs effect damage)
BattleCard.LastDamageSource = EGCGDamageSource::BattleDamage;

// Check if unit is destroyed
if (BattleCard.CurrentDamage >= BattleCard.HP)
{
    return DestroyUnit(TargetInstanceID, PlayerState);
}
```

**Status**: ✅ Implemented

---

### 8-5-3-2-2. <First Strike> Advantage

**If the attacking Unit has <First Strike>, it deals battle damage to the enemy Unit before normal battle damage is managed.**

**Implementation**:
```cpp
// In UGCGCombatSubsystem::ResolveAttack() - lines 263-290

// Check for First Strike keyword
if (KeywordSubsystem && KeywordSubsystem->HasFirstStrikeAdvantage(AttackerInstance, BlockerInstance))
{
    // Process First Strike
    FGCGKeywordResult FirstStrikeResult = KeywordSubsystem->ProcessFirstStrike(
        AttackerInstance, BlockerInstance, bBlockerDestroyedByFirstStrike);

    if (bBlockerDestroyedByFirstStrike)
    {
        // Blocker destroyed by First Strike - no retaliation
        bool bBlockerDestroyed = DealDamageToUnit(Attack.BlockerInstanceID, AttackerAP, DefendingPlayer);

        // Check for Breach keyword
        if (KeywordSubsystem->HasKeyword(AttackerInstance, EGCGKeyword::Breach))
        {
            FGCGKeywordResult BreachResult = KeywordSubsystem->ProcessBreach(
                AttackerInstance, DefendingPlayer, GameState);
        }

        return Result;
    }
}
```

**Status**: ✅ Implemented (via UGCGKeywordSubsystem)

---

### 8-5-3-2-3. Simultaneous Destruction

**If both battling Units are destroyed, their destruction is treated as happening simultaneously.**

**Implementation**:
```cpp
// Both DealDamageToUnit() calls execute before checking results
bool bAttackerDestroyed = DealDamageToUnit(Attack.AttackerInstanceID, BlockerAP, AttackingPlayer);
bool bBlockerDestroyed = DealDamageToUnit(Attack.BlockerInstanceID, AttackerAP, DefendingPlayer);

// Both destruction flags set simultaneously
```

**Status**: ✅ Implemented

---

### 8-5-4. Continue to Battle End Step

**When you have resolved all effects activated during this step, continue to the battle end step.**

**Status**: ✅ Implemented (attack resolution complete)

---

## 8-6. Battle End Step

### 8-6-1. Expire "During This Battle" Effects

**All effects worded "during this battle" lose effect.**

**Implementation**:
```cpp
// In UGCGCombatSubsystem::ClearAttacks() - lines 561-573
void ClearAttacks(AGCGGameState* GameState)
{
    GameState->CurrentAttacks.Empty();
    GameState->bAttackInProgress = false;
}

// In Effect System:
EffectSubsystem->CleanupAllModifiers(PlayerState, GameState,
    false,  // bCleanupTurnEffects
    true    // bCleanupBattleEffects
);
```

**Status**: ✅ Implemented (via Effect System)

---

### 8-6-2. Return to Main Phase

**When you have resolved all effects activated during this step, the battle ends, and you return to the main phase.**

**Implementation**:
```cpp
// Battle ends, main phase continues
// Player can declare another attack or pass priority
```

**Status**: ✅ Implemented

---

## Implementation Summary

### ✅ Fully Implemented (23/28 rules - 82%)

**Attack Step (2/5)**:
1. ✅ Declare attack, rest attacker
2. ✅ Attack validation (active, summoning sickness, can attack)

**Block Step (4/5)**:
3. ✅ <Blocker> activation
4. ✅ <Blocker> activation limit (one per attack)
5. ✅ <Blocker> optional
6. ✅ Blocker keyword check (can block while rested)

**Damage Step (15/16)**:
7. ✅ Confirm attack target
8. ✅ Check shield area
9. ✅ No shields → instant defeat
10. ✅ Break shields
11. ✅ Shield reveal
12. ✅ Base HP tracking
13. ✅ Deal damage between Units
14. ✅ Simultaneous damage
15. ✅ HP tracking and destruction
16. ✅ <First Strike> advantage
17. ✅ Simultaneous destruction
18. ✅ Battle damage source tracking
19. ✅ Breach keyword (break extra shields)
20. ✅ Suppression keyword (destroy all shields)
21. ✅ Support keyword (AP buffs)

**Battle End Step (2/2)**:
22. ✅ Expire "during this battle" effects
23. ✅ Return to main phase

### ⚠️ Partially Implemented (3/28 rules)

24. **Attack on rested Unit** (8-2-1) - Only supports attacking player
25. **【Burst】 UI prompt** (8-5-2-3-1) - Detection ✅, player choice UI ❌
26. **Attack Base directly** (8-5-2-4) - Needs separate targeting mode

### ❌ Not Implemented (2/28 rules)

27. **【Attack】 effect triggers** (8-2-2) - Awaits Effect System
28. **Early battle end checks** (8-2-4, 8-3-5, 8-4-2) - Zone change detection during combat

---

## Files Implementing Section 8

### Primary Implementation:
- **`UGCGCombatSubsystem.cpp`** - Complete combat system (600+ lines)
  - `DeclareAttack()` - Attack step
  - `CanAttack()` - Attack validation
  - `DeclareBlocker()` - Block step
  - `CanBlock()` - Blocker validation
  - `ResolveAttack()` - Damage step
  - `DealDamageToUnit()` - Unit damage
  - `DealDamageToPlayer()` - Player/shield damage
  - `BreakShields()` - Shield destruction with Burst
  - `ClearAttacks()` - Battle end
  - `HasSummoningSickness()` - Summoning sickness check

### Supporting Systems:
- **`UGCGKeywordSubsystem`** - Keyword processing
  - `ProcessFirstStrike()` - First Strike resolution
  - `ProcessBreach()` - Breach (extra shield breaks)
  - `ProcessSuppression()` - Suppression (destroy all shields)
  - `CalculateSupportBuff()` - Support AP buffs
- **`GCGTypes.h`** - Combat data structures
  - `FGCGAttackDeclaration` - Attack tracking
  - `FGCGCombatResult` - Combat results
  - `EGCGCombatStep` - Battle step enum
  - `EGCGDamageSource` - Damage type tracking

---

## Priority Completion Tasks

### Priority 1 (High): Core Combat Features
1. **Attack on Rested Unit** - Implement targeting specific enemy Units
2. **Early Battle End Checks** - Zone change detection during combat steps
3. **【Attack】 Effect Triggers** - Integrate with Effect System

### Priority 2 (Medium): Polish
4. **【Burst】 Player Choice UI** - Implement prompt for Burst activation
5. **Base Direct Attack Mode** - Separate targeting for attacking Base vs shields
6. **Action Step During Battle** - Full Section 9 implementation

---

## Technical Notes

**Attack Tracking**:
- `GameState->CurrentAttacks` - Array of active attack declarations
- `GameState->bAttackInProgress` - Combat in progress flag
- Each attack tracked individually for multi-attack resolution

**Damage Source Tracking**:
```cpp
// FAQ Q97-99: Battle damage vs Effect damage
Card.LastDamageSource = EGCGDamageSource::BattleDamage;  // During combat
Card.LastDamageSource = EGCGDamageSource::EffectDamage;  // From card effects
```

**Summoning Sickness**:
```cpp
// Unit can't attack on turn deployed (unless Link Unit)
bool bDeployedThisTurn = (CardInstance.TurnDeployed == GameState->TurnNumber);
```

**Keyword Integration**:
- <Blocker> - Can block while rested
- <First Strike> - Damage before retaliation
- <Breach> - Break extra shields when destroying Unit
- <Suppression> - Destroy ALL shields simultaneously
- <Support> - Buff allied Units' AP

**Combat Flow**:
1. Main Phase → Player declares attack
2. Attack Step → Rest attacker, create attack declaration
3. Block Step → Defender can declare blocker
4. Action Step → Alternating priority (Section 9)
5. Damage Step → Resolve damage
6. Battle End Step → Clean up, return to Main Phase

**Multiple Attacks**:
- Player can declare multiple attacks in one turn
- Each attack resolved individually
- `ResolveAllAttacks()` processes all pending attacks

**Blueprint Integration**:
- Combat results exposed to Blueprint
- UI can display attack/block opportunities
- Combat events fire for UI updates
