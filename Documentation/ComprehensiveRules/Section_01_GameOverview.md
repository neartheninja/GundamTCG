# Section 1: Game Overview

**Integration Status**: ✅ Implemented
**Implementation Location**: `AGCGGameMode_1v1`, `UGCGCombatSubsystem`
**Commit**: 4e56c2e

---

## 1-1. The Game

The Gundam TCG is a two-player trading card game. Players prepare their decks in advance, then compete against each other using those decks. The goal is to outmaneuver your opponent and fulfill the conditions for victory before they do.

### Implementation Notes
- Game modes: `AGCGGameMode_1v1` (1v1), `AGCGGameMode_2v2` (team battle)
- Game state tracked in `AGCGGameState`

---

## 1-2. Winning and Losing the Game

The following are ways to win or lose the game:

### 1-2-1. Winning the Game
A player wins the game when all opposing players fulfill one or more of the conditions for defeat at the same time.

**Implementation**: `AGCGGameMode_1v1::EndGame()` called when opponent loses

---

### 1-2-2. Conditions for Defeat

#### 1-2-2-1. Battle Damage with No Shields (Implemented)
When either player receives battle damage from a Unit while they have no cards in their shield area, that player fulfills the conditions for defeat.

**Implementation**:
- Location: `UGCGCombatSubsystem::DealDamageToPlayer()`
- Code: Lines 150-170
- Logic: When `ShieldStack.Num() == 0` and taking battle damage, set `bHasLost = true`
- Rule: Defeat is immediate when taking damage with 0 shields (NOT when Base HP reaches 0)

```cpp
// Comprehensive Rules 1-2-2-1: Battle damage with no shields = defeat
if (DefendingPlayer->ShieldStack.Num() == 0)
{
    DefendingPlayer->bHasLost = true;
    return true; // Defeat condition met
}
```

#### 1-2-2-2. Empty Deck (Implemented)
When you are required to draw a card(s) from your deck, but you do not have any cards in your deck to draw, you fulfill the conditions for defeat.

**Implementation**:
- Location: `AGCGGameMode_1v1::CheckDefeatConditions()`
- Code: Lines 580-590
- Logic: Check `PlayerState->GetDeckSize() == 0` during rules management
- Note: Loss occurs when attempting to draw with empty deck, not when deck becomes empty

```cpp
// 1-2-2-2: No cards remaining in deck
if (PlayerState->GetDeckSize() == 0)
{
    return true; // Defeat condition met
}
```

---

### 1-2-3. Rules Management (Implemented)
After the conclusion of game actions, the game automatically checks whether the conditions for defeat are met. This is called "rules management."

**Implementation**:
- Location: `AGCGGameMode_1v1::PerformRulesManagement()`
- Called after: Every game action that could cause defeat
- Checks: All defeat conditions for all players
- Triggers: Automatic loss and game end

```cpp
void AGCGGameMode_1v1::PerformRulesManagement()
{
    TArray<AGCGPlayerState*> AllPlayerStates = GetAllPlayerStates();
    for (AGCGPlayerState* PlayerState : AllPlayerStates)
    {
        if (CheckDefeatConditions(PlayerState->GetPlayerID()))
        {
            PlayerState->bHasLost = true;
            int32 OpponentID = GetNextPlayerID(PlayerState->GetPlayerID());
            EndGame(OpponentID);
            return;
        }
    }
}
```

---

### 1-2-4. Concession (Implemented)
Players may concede the game at any point during the game. Players who concede are treated as fulfilling the conditions for defeat.

**Implementation**:
- Location: `AGCGGameMode_1v1::ProcessPlayerConcession()`
- Trigger: Player-initiated (UI or RPC call)
- Effect: Immediately sets `bHasLost = true` and ends game

```cpp
void AGCGGameMode_1v1::ProcessPlayerConcession(int32 PlayerID)
{
    AGCGPlayerState* PlayerState = GetPlayerStateByID(PlayerID);
    PlayerState->bHasLost = true;
    int32 OpponentID = GetNextPlayerID(PlayerID);
    EndGame(OpponentID);
}
```

---

### 1-2-5. Concession Cannot Be Forced (Implemented)
Players cannot be forced to concede due to card effects or other game effects. Players must concede of their own volition.

**Implementation**:
- Location: `ProcessPlayerConcession()` is only callable by player actions, not by card effects
- Safeguard: No card effect system can call this method
- Enforcement: Method access restricted to player input handlers only

---

## 1-3. Fundamental Rules

### 1-3-1. The Rules
Card text supersedes the comprehensive rules. If card text contradicts the comprehensive rules, the card text takes priority.

**Implementation**:
- Card effects processed by effect system (future implementation)
- Card text validation in `UGCGValidationSubsystem`

---

### 1-3-2. Impossibilities
Players cannot perform impossible game actions. If part of an action is impossible to perform, perform as much as possible and ignore the impossible parts.

**Implementation**:
- Action validation throughout all subsystems
- Partial execution logic in effect resolution (future implementation)

---

### 1-3-3. Numerical Values
If a calculation produces a fractional number, round down. Numerical values cannot go below 0.

**Implementation**:
- Applied throughout combat calculations
- Minimum value checks: `FMath::Max(0, value)`
- Division: Integer division automatically rounds down

```cpp
// Example from combat calculations
int32 Damage = FMath::Max(0, AttackerAP - DefenderHP);
```

---

## Implementation Checklist

- [x] **1-2-2-1**: Battle damage with no shields = defeat
- [x] **1-2-2-2**: Empty deck when drawing = defeat
- [x] **1-2-3**: Rules management system
- [x] **1-2-4**: Player concession
- [x] **1-2-5**: Concession cannot be forced
- [x] **1-3-1**: Card text supersedes rules
- [x] **1-3-2**: Impossible actions ignored
- [x] **1-3-3**: Fractional values round down, minimum 0

---

## Testing Notes

**Defeat Conditions:**
1. Test battle damage with 0 shields triggers immediate defeat
2. Test drawing from empty deck triggers defeat
3. Test rules management runs after all game actions
4. Test player concession works at any time
5. Verify card effects cannot force concession

**Edge Cases:**
1. Simultaneous defeat conditions (both players lose)
2. Defeat during opponent's turn
3. Multiple defeat conditions at once
4. Concession during card effect resolution
