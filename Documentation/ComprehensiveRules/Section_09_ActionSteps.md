# Section 9: Action Steps

## Table of Contents
- [9-1. Action Step Timing](#9-1-action-step-timing)
- [9-2. Alternating Priority System](#9-2-alternating-priority-system)
- [9-3. Standby Player Actions](#9-3-standby-player-actions)
- [9-4. Active Player Actions](#9-4-active-player-actions)
- [9-5. Action Step End Condition](#9-5-action-step-end-condition)

---

## Overview

**Action Steps provide timing windows where players can alternate activating instant-speed effects:**
- Occurs **after the block step** during combat (see Section 8-3)
- Occurs during the **end phase** (see Section 7-4-1)
- Priority alternates starting with the **standby player** (non-active player)
- Continues until **both players pass consecutively**

**Key Concepts**:
- **Standby Player**: The player who is NOT the active player (opponent)
- **Active Player**: The player whose turn it is
- **Alternating Priority**: Players take turns choosing to act or pass
- **Both Pass**: Action Step ends only when both pass in succession

---

## 9-1. Action Step Timing

**An action step occurs after the block step and during the end phase.**

**Rule**: Action Steps happen in two contexts:
1. **During Combat**: After blockers are declared (EGCGCombatStep::ActionStep)
2. **During End Phase**: First step of End Phase (EGCGEndPhaseStep::ActionStep)

**Implementation**:
```cpp
// In GCGTypes.h - lines 109-116
enum class EGCGCombatStep : uint8
{
    None,
    AttackStep,          // Declare attack, rest attacker
    BlockStep,           // Optional blocker activation
    ActionStep,          // Alternate action timing ← SECTION 9 APPLIES HERE
    DamageStep,          // Resolve damage
    BattleEndStep        // Cleanup
};

// In GCGTypes.h - lines 96-102
enum class EGCGEndPhaseStep : uint8
{
    None,
    ActionStep,          // Action timing ← SECTION 9 APPLIES HERE
    EndStep,             // "At end of turn" triggers
    HandStep,            // Discard to 10
    CleanupStep          // Expire "this turn" effects
};
```

**Status**: ✅ Enums defined, ⚠️ execution pending

---

## 9-2. Alternating Priority System

**During an action step, taking turns starting with the standby player, players may activate 【Action】 Command cards and effects with 【Activate･Action】 timing.**

**Rule**: Priority alternates between players:
1. **Standby player** (opponent) gets first opportunity
2. Then **active player** gets opportunity
3. Repeat until both pass consecutively

**Standby Player Definition**:
- The player who is NOT currently the active player
- In combat: Defending player is usually standby player
- In end phase: Opponent of turn player

**Implementation**:
```cpp
// Currently MISSING - needs to be implemented

// Proposed implementation in GCGGameState.h:
UPROPERTY(Replicated, BlueprintReadOnly, Category = "Priority")
int32 PriorityPlayerID;  // Who currently has priority

UPROPERTY(Replicated, BlueprintReadOnly, Category = "Priority")
int32 LastPassedPlayerID;  // Who passed last (for consecutive pass detection)

// Helper function needed:
int32 GetStandbyPlayerID() const
{
    // Return the opponent of active player
    return (ActivePlayerID == 1) ? 2 : 1;
}

// In AGCGGameMode_1v1::ExecuteActionStep():
void AGCGGameMode_1v1::ExecuteActionStep()
{
    // Rule 9-2: Standby player goes first
    int32 StandbyPlayerID = GCGGameState->GetStandbyPlayerID();
    GCGGameState->PriorityPlayerID = StandbyPlayerID;
    GCGGameState->LastPassedPlayerID = -1;  // Reset

    // Enter Action Step state
    bInActionStep = true;

    // Wait for player actions (UI-driven)
    // Players can activate 【Action】 Commands or 【Activate･Action】 abilities
    // Or pass priority
}
```

**Status**: ❌ Not implemented (TODO comment at GCGGameMode_1v1.cpp:461)

**Dependencies**:
- Effect System (for activating 【Activate･Action】 abilities)
- UI system (for player action prompts)

---

## 9-3. Standby Player Actions

**The standby player may either:**
1. **Activate a 【Action】 Command card from hand**
2. **Activate a 【Activate･Action】 effect on a card in play**
3. **Pass priority to the active player**

**Rule 9-3-1**: If standby player passes, priority moves to active player

**Implementation**:
```cpp
// Effect timing already defined
// In GCGTypes.h - line 168
EGCGEffectTiming::ActivateAction

// Action Command cards marked with:
// FGCGCardData::ActivationTiming = EGCGEffectTiming::ActivateAction

// When standby player acts:
void AGCGGameMode_1v1::ProcessStandbyPlayerAction(int32 PlayerID, EGCGPlayerActionType ActionType)
{
    // Validate it's standby player's priority
    if (PlayerID != GCGGameState->GetStandbyPlayerID())
    {
        UE_LOG(LogTemp, Warning, TEXT("Not standby player's priority"));
        return;
    }

    if (ActionType == EGCGPlayerActionType::PassPriority)
    {
        // Rule 9-3-1: Pass to active player
        GCGGameState->PriorityPlayerID = GCGGameState->ActivePlayerID;
        GCGGameState->LastPassedPlayerID = PlayerID;
    }
    else if (ActionType == EGCGPlayerActionType::ActivateAbility)
    {
        // Process 【Action】 activation
        // Reset pass tracking (someone acted)
        GCGGameState->LastPassedPlayerID = -1;

        // Priority stays with standby player (they can act again)
        // OR moves to active player (depending on rules interpretation)
        // TODO: Clarify rules for priority after action
    }
}
```

**Status**: ❌ Not implemented

---

## 9-4. Active Player Actions

**The active player may either:**
1. **Activate a 【Action】 Command card from hand**
2. **Activate a 【Activate･Action】 effect on a card in play**
3. **Pass priority to the standby player**

**Rule 9-4-1**: After active player acts or passes, priority returns to standby player

**Rule 9-4-2**: This alternation continues until both players pass consecutively

**Implementation**:
```cpp
// When active player acts:
void AGCGGameMode_1v1::ProcessActivePlayerAction(int32 PlayerID, EGCGPlayerActionType ActionType)
{
    // Validate it's active player's priority
    if (PlayerID != GCGGameState->ActivePlayerID)
    {
        UE_LOG(LogTemp, Warning, TEXT("Not active player's priority"));
        return;
    }

    if (ActionType == EGCGPlayerActionType::PassPriority)
    {
        // Check if standby player also passed
        if (GCGGameState->LastPassedPlayerID == GCGGameState->GetStandbyPlayerID())
        {
            // Rule 9-4-2: Both passed consecutively
            // Rule 9-5: End Action Step
            EndActionStep();
        }
        else
        {
            // Rule 9-4-1: Return priority to standby player
            GCGGameState->PriorityPlayerID = GCGGameState->GetStandbyPlayerID();
            GCGGameState->LastPassedPlayerID = PlayerID;
        }
    }
    else if (ActionType == EGCGPlayerActionType::ActivateAbility)
    {
        // Process 【Action】 activation
        // Reset pass tracking (someone acted)
        GCGGameState->LastPassedPlayerID = -1;

        // Priority returns to standby player (Rule 9-4-1)
        GCGGameState->PriorityPlayerID = GCGGameState->GetStandbyPlayerID();
    }
}
```

**Status**: ❌ Not implemented

**Key Logic**:
- **Someone acts**: Reset LastPassedPlayerID = -1 (pass counter resets)
- **First pass**: LastPassedPlayerID = that player
- **Second pass**: Check if LastPassedPlayerID = other player → end Action Step

---

## 9-5. Action Step End Condition

**After both players pass priority in succession, the action step ends.**

**Rule**: Action Step continues indefinitely until:
1. Standby player passes
2. THEN active player passes (without anyone acting in between)

**Implementation**:
```cpp
void AGCGGameMode_1v1::EndActionStep()
{
    UE_LOG(LogTemp, Log, TEXT("Action Step ended - both players passed"));

    // Clear priority state
    GCGGameState->PriorityPlayerID = -1;
    GCGGameState->LastPassedPlayerID = -1;
    bInActionStep = false;

    // Determine where to continue based on context
    if (GCGGameState->CurrentPhase == EGCGTurnPhase::MainPhase)
    {
        // Combat Action Step → continue to Damage Step
        UGCGCombatSubsystem* CombatSubsystem = GetGameInstance()->GetSubsystem<UGCGCombatSubsystem>();
        if (CombatSubsystem)
        {
            // Rule 8-4: Proceed to Damage Step
            CombatSubsystem->ExecuteDamageStep(CurrentAttack, GCGGameState);
        }
    }
    else if (GCGGameState->CurrentPhase == EGCGTurnPhase::EndPhase)
    {
        // End Phase Action Step → continue to End Step
        // Rule 7-4-2: Proceed to End Step
        GCGGameState->CurrentEndPhaseStep = EGCGEndPhaseStep::EndStep;
        ExecuteEndStep();
    }
}
```

**Status**: ❌ Not implemented

---

## Action Step Flow Diagram

```
┌─────────────────────────────────────────┐
│       Enter Action Step                 │
│  (from Block Step or End Phase start)   │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│  Priority → Standby Player              │
│  LastPassedPlayerID = -1                │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│  Standby Player's Turn                  │
│  Options:                               │
│  • Activate 【Action】 Command         │
│  • Activate 【Activate･Action】       │
│  • Pass Priority                        │
└──────────────┬──────────────────────────┘
               │
         ┌─────┴─────┐
         │           │
    [Activate]   [Pass]
         │           │
         │           ▼
         │  ┌─────────────────────┐
         │  │ LastPassed = Standby│
         │  │ Priority → Active   │
         │  └──────┬──────────────┘
         │         │
         └────────►│
                   ▼
         ┌─────────────────────┐
         │ Active Player's Turn│
         │ Options:            │
         │ • Activate 【Action】│
         │ • Activate 【Activate･Action】│
         │ • Pass Priority     │
         └──────┬──────────────┘
                │
          ┌─────┴─────┐
          │           │
     [Activate]   [Pass]
          │           │
          │           ▼
          │  ┌──────────────────────┐
          │  │ LastPassed = Active? │
          │  └───┬──────────────┬───┘
          │      │ YES          │ NO
          │      │              │
          │      ▼              ▼
          │  ┌────────┐  ┌─────────────┐
          │  │  END   │  │ Priority →  │
          │  │ ACTION │  │   Standby   │
          │  │  STEP  │  └──────┬──────┘
          │  └────────┘         │
          │                     │
          └────────────────────►│
                                │
                         [Loop continues]
```

---

## Implementation Summary

### Current Status: ~20% Complete

**✅ What Exists**:
1. `EGCGCombatStep::ActionStep` - Combat action timing enum (GCGTypes.h:114)
2. `EGCGEndPhaseStep::ActionStep` - End phase action timing enum (GCGTypes.h:99)
3. `EGCGEffectTiming::ActivateAction` - Effect timing for Action abilities (GCGTypes.h:168)
4. `EGCGPlayerActionType::PassPriority` - Pass priority action type (PlayerActionSubsystem.h:28)
5. `RequestPassPriority()` - Basic pass method (only works in Main Phase) (GameMode:689)

**❌ What's Missing**:
1. **Standby Player Identification** - No GetStandbyPlayerID() method
2. **Priority State Tracking** - No PriorityPlayerID or LastPassedPlayerID fields
3. **Alternating Priority Logic** - No standby-first, then active alternation
4. **Both Pass Detection** - No consecutive pass tracking
5. **Action Step Execution** - Currently just TODO comment (GameMode:461)
6. **Action Step State Machine** - No bInActionStep flag or state management

**Implementation Breakdown**:

| Rule | Description | Status | Location |
|------|-------------|--------|----------|
| 9-1 | Action Step timing (Combat + End Phase) | ✅ Enums defined | GCGTypes.h:99, 114 |
| 9-2 | Alternating priority (standby first) | ❌ Not implemented | - |
| 9-3 | Standby player actions | ❌ Not implemented | - |
| 9-3-1 | Pass to active player | ❌ Not implemented | - |
| 9-4 | Active player actions | ❌ Not implemented | - |
| 9-4-1 | Return priority to standby | ❌ Not implemented | - |
| 9-4-2 | Continue until both pass | ❌ Not implemented | - |
| 9-5 | Action Step ends on both pass | ❌ Not implemented | - |

**Coverage**: 1/8 rules (12.5%)

---

## Dependencies

**Section 9 requires**:
1. **Effect System (Section 10)** - To execute 【Activate･Action】 abilities
2. **UI System** - To prompt players for actions during Action Step
3. **Game State Extensions** - PriorityPlayerID, LastPassedPlayerID tracking

**Section 9 is required by**:
1. **Combat System (Section 8)** - Block Step → Action Step → Damage Step
2. **End Phase (Section 7)** - Action Step → End Step → Hand Step → Cleanup

---

## Implementation Notes

### Priority System Design

The Action Step introduces the game's **priority system**:

**Key Principles**:
1. **Non-active player first**: Standby player always gets first opportunity
2. **Alternating**: Priority bounces between players
3. **Reset on action**: Any activation resets the "pass counter"
4. **Both pass to end**: Requires two consecutive passes

**Similar to Magic: The Gathering's priority system**, but simpler:
- Only applies during Action Steps (not all game actions)
- No stack/effect resolution (effects resolve immediately)
- Always standby player first (no "active player first" exceptions)

### Implementation Phases

**Phase 1: State Tracking** (Foundation)
- Add PriorityPlayerID to GCGGameState
- Add LastPassedPlayerID to GCGGameState
- Add GetStandbyPlayerID() helper
- Add bInActionStep flag

**Phase 2: Action Step Execution** (Core Logic)
- Implement ExecuteActionStep() in GameMode
- Implement ProcessStandbyPlayerAction()
- Implement ProcessActivePlayerAction()
- Implement EndActionStep()

**Phase 3: UI Integration** (Player Interaction)
- Create Action Step UI overlay
- Add "Pass Priority" button
- Add action activation buttons (when available)
- Show "Waiting for opponent" indicator

**Phase 4: Effect System Integration** (Functionality)
- Filter effects by EGCGEffectTiming::ActivateAction
- Allow activation during Action Step only
- Process Command cards with 【Action】 timing
- Process abilities with 【Activate･Action】 timing

---

## Testing Scenarios

### Test Case 1: Both Pass Immediately
```
1. Enter Action Step (Combat or End Phase)
2. Standby player passes
3. Active player passes
4. ✓ Action Step ends
5. ✓ Game continues to next step (Damage or End Step)
```

### Test Case 2: Standby Acts, Then Both Pass
```
1. Enter Action Step
2. Standby player activates 【Action】 Command
3. Standby player passes (or Active player passes first)
4. Other player passes
5. ✓ Action Step ends
6. ✓ Command effect was applied
```

### Test Case 3: Multiple Activations
```
1. Enter Action Step
2. Standby player activates ability
3. Active player activates ability
4. Standby player activates another ability
5. Standby player passes
6. Active player passes
7. ✓ Action Step ends
8. ✓ All effects were applied in order
```

### Test Case 4: Pass-Act-Pass Sequence
```
1. Enter Action Step
2. Standby player passes
3. Active player activates ability (resets pass counter)
4. Standby player passes (first pass of new sequence)
5. Active player passes (second pass)
6. ✓ Action Step ends
7. ✓ Pass counter reset correctly after action
```

---

## Related Sections

- **Section 7-4-1**: End Phase Action Step timing
- **Section 8-3**: Combat Action Step (after Block Step)
- **Section 10**: Effect Activation (will define 【Activate･Action】 mechanics)
- **Section 13**: Keywords that may have 【Action】 timing

---

## Rule References

**This section implements rules from the official Gundam TCG Comprehensive Rules Section 9: Action Steps.**

**Key Rule Numbers**:
- **9-1**: Action Step occurs after block step and during end phase
- **9-2**: Alternating priority starting with standby player
- **9-3**: Standby player may activate or pass
- **9-3-1**: Pass moves priority to active player
- **9-4**: Active player may activate or pass
- **9-4-1**: Priority returns to standby player after active player acts/passes
- **9-4-2**: Alternation continues until both pass consecutively
- **9-5**: Action Step ends when both players pass in succession
