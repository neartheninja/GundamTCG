# Gundam TCG - Unreal Engine 5.6 Architecture Document

**Version**: 1.0.0
**Last Updated**: 2025-11-14
**Engine**: Unreal Engine 5.6
**Platform**: PC (Windows/Linux)
**Language**: C++ (Core Logic) + Blueprints (UI/Glue)

---

## Table of Contents

1. [Overview](#overview)
2. [Core Systems Architecture](#core-systems-architecture)
3. [Class Hierarchy](#class-hierarchy)
4. [Data Structures](#data-structures)
5. [Game Flow](#game-flow)
6. [Zone Management](#zone-management)
7. [Effect & Keyword System](#effect--keyword-system)
8. [Networking Architecture](#networking-architecture)
9. [UI/UMG Architecture](#uiumg-architecture)
10. [Team Battle (2v2) Specifics](#team-battle-2v2-specifics)
11. [Implementation Phases](#implementation-phases)

---

## Overview

### Game Modes

**1v1 Standard Mode**
- 2 players
- Each player: 50-card Main Deck + 10-card Resource Deck
- 6 Shields per player
- 1 Base card or EX Base token per player
- Victory: Opponent has no shields and takes damage, or opponent cannot draw

**2v2 Team Battle Mode**
- 4 players (2 teams of 2)
- Each player: Own 50-card Main Deck + 10-card Resource Deck
- Shared Shield Stack: 8 shields total (4 per player, alternating)
- Shared Base: 1 Base for the team
- Max 6 Units per team total on battlefield
- Victory: Team has no shields and takes damage, or any team member cannot draw

### Core Design Principles

1. **Server-Authoritative**: All game rules execute server-side; clients are display/input only
2. **Data-Driven**: Cards defined in DataTables; no card-specific hardcoded logic
3. **Event-Based**: Trigger system using delegates for effect timing
4. **Modular**: Separation of concerns (zones, effects, combat, networking)
5. **Extensible**: Easy to add new keywords, effects, and card types

---

## Core Systems Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    AGCGGameMode (Server Authority)           │
│  - Turn/Phase Management                                    │
│  - Rules Enforcement                                        │
│  - Combat Resolution                                        │
│  - Event Broadcasting                                       │
└────────────┬────────────────────────────────────────────────┘
             │
             ├─► AGCGGameState (Replicated)
             │   - Current Phase/Turn
             │   - Active Player ID
             │   - Team Info (2v2 mode)
             │   - Shared Shield Stack (2v2)
             │   - Win/Lose Flags
             │
             ├─► AGCGPlayerState (Replicated, Per-Player)
             │   - Player Zones (Hand, Deck, Battle, Resource, etc.)
             │   - Team ID, IsTeamLeader
             │   - Deck Lists
             │
             ├─► AGCGPlayerController (Client Input)
             │   - RPC: PlayCard, DeclareAttack, ActivateEffect
             │   - UI Input Handling
             │
             ├─► UGCGZoneSubsystem (Zone Operations)
             │   - MoveCard(from, to)
             │   - Validation & Limits
             │
             ├─► UGCGEffectExecutor (Effect Resolution)
             │   - Parse & Execute Effects
             │   - Keyword Handlers
             │   - Modifier Tracking
             │
             └─► UGCGEventBus (Event System)
                 - Delegates: OnCardPlayed, OnAttack, OnDestroyed, etc.
                 - Trigger Subscription
```

---

## Class Hierarchy

### Game Framework Classes

```cpp
// Game Mode (Server-only)
AGameMode
└── AGCGGameModeBase (Menu/Lobby)
    └── AGCGGameMode_1v1 (Standard Match)
    └── AGCGGameMode_2v2 (Team Battle)

// Game State (Replicated)
AGameState
└── AGCGGameState
    - TurnNumber, CurrentPhase, ActivePlayerID
    - TeamAInfo, TeamBInfo (2v2 mode)
    - SharedShieldStack (2v2 mode)

// Player State (Replicated, Per-Player)
APlayerState
└── AGCGPlayerState
    - Zones: Hand, Deck, Battle, Resource, Shield, Base, Trash, Removal
    - TeamID, IsTeamLeader
    - DeckList, ResourceDeckList

// Player Controller (Client + Server)
APlayerController
└── AGCGPlayerController
    - Server RPCs: PlayCard, DeclareAttack, ActivateAbility
    - Client UI Input Handling
```

### Subsystems & Managers

```cpp
// Zone Management
UGameInstanceSubsystem
└── UGCGZoneSubsystem
    - MoveCard(CardInstance, FromZone, ToZone)
    - ValidateMove()
    - ApplyZoneRules()

// Effect Execution
UObject
└── UGCGEffectExecutor
    - ExecuteEffect(FEffectData)
    - ResolveKeyword(Keyword)
    - ApplyModifier(Target, Modifier)

// Event Bus
UObject
└── UGCGEventBus
    - Delegates: OnCardPlayed, OnAttack, OnDestroyed, etc.
    - Subscribe/Unsubscribe
```

---

## Data Structures

### FCardData (Static Card Definition)

**Location**: `GCGTypes.h`

```cpp
USTRUCT(BlueprintType)
struct FCardData : public FTableRowBase
{
    GENERATED_BODY()

    // Identity
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName CardNumber;           // e.g., "GCG-001"

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText CardName;             // e.g., "RX-78-2 Gundam"

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECardType CardType;         // Unit, Pilot, Command, Base

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<ECardColor> Colors;  // 1-2 colors max

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> Traits;       // e.g., "Mobile Suit", "Gundam"

    // Stats
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Level;                // Lv requirement (1-10)

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Cost;                 // Resource cost to play

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 AP;                   // Attack Power

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 HP;                   // Hit Points

    // Keywords
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSet<FName> Keywords;       // Repair, Breach, Blocker, FirstStrike, etc.

    // Effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FEffectData> Effects;

    // Presentation
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UTexture2D> CardArt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText CardText;

    // Link Requirements (for Link Units)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinkRequirement LinkReq;   // Required Pilot traits/colors
};
```

### FCardInstance (Runtime Card State)

```cpp
USTRUCT(BlueprintType)
struct FCardInstance
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 InstanceID;           // Unique runtime ID

    UPROPERTY(BlueprintReadWrite)
    FName CardNumber;           // Reference to FCardData

    UPROPERTY(BlueprintReadWrite)
    ECardZone CurrentZone;

    UPROPERTY(BlueprintReadWrite)
    bool bIsActive;             // Active (false = rested)

    UPROPERTY(BlueprintReadWrite)
    int32 DamageCounters;       // Accumulated damage

    UPROPERTY(BlueprintReadWrite)
    int32 OwnerPlayerID;

    UPROPERTY(BlueprintReadWrite)
    int32 ControllerPlayerID;   // For taking control effects

    UPROPERTY(BlueprintReadWrite)
    int32 PairedCardInstanceID; // For Units with Pilots

    UPROPERTY(BlueprintReadWrite)
    bool bIsToken;              // EX Base, EX Resource, etc.

    UPROPERTY(BlueprintReadWrite)
    TArray<FActiveModifier> Modifiers; // Temporary stat changes
};
```

### Enumerations

```cpp
UENUM(BlueprintType)
enum class ECardType : uint8
{
    Unit,
    Pilot,
    Command,
    Base,
    Resource,
    Token
};

UENUM(BlueprintType)
enum class ECardColor : uint8
{
    White,
    Blue,
    Green,
    Red,
    Black,
    Yellow,
    Colorless
};

UENUM(BlueprintType)
enum class ECardZone : uint8
{
    None,
    Deck,
    ResourceDeck,
    Hand,
    ResourceArea,      // Up to 15 resources (10 normal + 5 EX)
    BattleArea,        // Up to 6 Units
    ShieldStack,       // Face-down shields
    BaseSection,       // 1 Base card or EX Base
    Trash,
    Removal            // Removed from game
};

UENUM(BlueprintType)
enum class ETurnPhase : uint8
{
    StartPhase,        // Active Step → Start Step
    DrawPhase,
    ResourcePhase,
    MainPhase,
    EndPhase,          // Action Step → End Step → Hand Step → Cleanup Step
    GameOver
};

UENUM(BlueprintType)
enum class ECombatStep : uint8
{
    None,
    AttackStep,
    BlockStep,
    ActionStep,
    DamageStep,
    BattleEndStep
};

UENUM(BlueprintType)
enum class EKeyword : uint8
{
    None,
    Repair,            // Repair X - Recover X HP at end of turn
    Breach,            // Breach X - Deal X to shields when destroying Unit
    Support,           // Support X - Buff allies
    Blocker,           // Can block attacks
    FirstStrike,       // Damage resolves first
    HighManeuver,      // Evasion mechanic
    Suppression,       // Destroy multiple shields
    Burst              // Shield-only trigger
};
```

### FEffectData (Effect Definition)

```cpp
USTRUCT(BlueprintType)
struct FEffectData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EEffectTiming Timing;       // Deploy, Attack, Destroyed, Burst, ActivateMain, etc.

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FEffectCondition> Conditions;  // Requirements to activate

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FEffectCost> Costs;  // What to pay

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FEffectOperation> Operations;  // What happens

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Description;          // For UI display
};
```

### FActiveModifier (Temporary Stat Changes)

```cpp
USTRUCT(BlueprintType)
struct FActiveModifier
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FName ModifierType;         // "AP", "HP", "Cost"

    UPROPERTY(BlueprintReadWrite)
    int32 Amount;

    UPROPERTY(BlueprintReadWrite)
    EModifierDuration Duration; // UntilEndOfTurn, DuringThisBattle, Permanent

    UPROPERTY(BlueprintReadWrite)
    int32 SourceInstanceID;     // Who applied this
};
```

---

## Game Flow

### Turn Structure

```
┌─────────────────────────────────────────────────────────────┐
│ START PHASE                                                  │
│  - Active Step: Set all rested cards to active             │
│  - Start Step: Trigger "at start of turn" effects          │
└────────────────────────────────────────────────────┬────────┘
                                                     │
┌────────────────────────────────────────────────────▼────────┐
│ DRAW PHASE                                                   │
│  - Draw 1 card (mandatory)                                  │
│  - If deck empty after draw → lose game                    │
└────────────────────────────────────────────────────┬────────┘
                                                     │
┌────────────────────────────────────────────────────▼────────┐
│ RESOURCE PHASE                                               │
│  - Place 1 card from Resource Deck to Resource Area         │
│  - Face up, active state                                    │
│  - If Resource Deck empty, skip but don't lose             │
└────────────────────────────────────────────────────┬────────┘
                                                     │
┌────────────────────────────────────────────────────▼────────┐
│ MAIN PHASE                                                   │
│  - Repeat any order:                                        │
│    • Play Units, Pilots, Bases, Commands                   │
│    • Activate 【Activate・Main】 effects                    │
│    • Declare Unit attacks (enters Combat Flow)             │
└────────────────────────────────────────────────────┬────────┘
                                                     │
┌────────────────────────────────────────────────────▼────────┐
│ END PHASE                                                    │
│  - Action Step: Commands & 【Activate・Action】 timing      │
│  - End Step: "At end of turn" effects trigger              │
│  - Hand Step: If hand ≥ 11, discard to 10                  │
│  - Cleanup Step: "During this turn" effects expire         │
└────────────────────────────────────────────────────┬────────┘
                                                     │
                                               Pass Turn
```

### Combat Flow (During Main Phase)

```
┌─────────────────────────────────────────────────────────────┐
│ ATTACK STEP                                                  │
│  1. Attacker (active Unit) is rested                        │
│  2. Choose target: enemy player OR rested enemy Unit        │
│  3. Trigger 【Attack】 effects and "when attacks"           │
└────────────────────────────────────────────────────┬────────┘
                                                     │
┌────────────────────────────────────────────────────▼────────┐
│ BLOCK STEP                                                   │
│  - Defending player may activate <Blocker>                  │
│  - Only one Blocker per attack                              │
│  - Blocker must be active Unit                              │
│  - Changes attack target to Blocker                         │
└────────────────────────────────────────────────────┬────────┘
                                                     │
┌────────────────────────────────────────────────────▼────────┐
│ ACTION STEP                                                  │
│  - Players alternate Action timing                          │
│  - Can play Commands with 【Action】                        │
│  - Can activate 【Activate・Action】 abilities              │
│  - Defending player goes first                              │
└────────────────────────────────────────────────────┬────────┘
                                                     │
┌────────────────────────────────────────────────────▼────────┐
│ DAMAGE STEP                                                  │
│  - If target is PLAYER:                                     │
│    • Damage → Base (if present): AP to HP                   │
│    • Else damage → Shield: 1 HP each, reveal & Burst        │
│    • Else damage → Player: immediate defeat                 │
│  - If target is UNIT:                                       │
│    • Simultaneous damage: AP to HP both sides               │
│    • <First Strike>: deals damage first, no retaliation     │
│      if target destroyed                                    │
└────────────────────────────────────────────────────┬────────┘
                                                     │
┌────────────────────────────────────────────────────▼────────┐
│ BATTLE END STEP                                              │
│  - "During this battle" effects expire                      │
│  - Breach triggers (if Unit destroyed, damage shields)      │
│  - Destroyed triggers                                       │
└─────────────────────────────────────────────────────────────┘
```

---

## Zone Management

### Zone Limits (1v1 Mode)

| Zone           | Limit                          | Visibility |
|----------------|--------------------------------|------------|
| Deck           | 50 cards (start)               | Private    |
| Resource Deck  | 10 cards (start)               | Private    |
| Hand           | ∞ (discard to 10 at end step)  | Private    |
| Resource Area  | 15 max (10 normal + 5 EX)      | Public     |
| Battle Area    | 6 Units max                    | Public     |
| Shield Stack   | 6 (start), decreases           | Face-down  |
| Base Section   | 1 Base card or EX Base         | Public     |
| Trash          | ∞                              | Public     |
| Removal        | ∞                              | Public     |

### Zone Limits (2v2 Mode)

| Zone                 | Limit                          | Visibility |
|----------------------|--------------------------------|------------|
| Deck (per player)    | 50 cards (start)               | Private    |
| Resource Deck        | 10 cards (start)               | Private    |
| Hand (per player)    | ∞ (discard to 10 at end step)  | Private    |
| Resource Area        | 15 max per player              | Public     |
| Battle Area (TEAM)   | **6 Units max per team total** | Public     |
| Shared Shield Stack  | 8 (4 per player, alternating)  | Face-down  |
| Shared Base Section  | 1 Base per team                | Public     |
| Trash (per player)   | ∞                              | Public     |
| Removal (per player) | ∞                              | Public     |

### UGCGZoneSubsystem Interface

```cpp
UCLASS()
class UGCGZoneSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Move card between zones with validation
    UFUNCTION(BlueprintCallable)
    bool MoveCard(FCardInstance& Card, ECardZone FromZone, ECardZone ToZone, int32 PlayerID);

    // Validate zone capacity
    UFUNCTION(BlueprintPure)
    bool CanAddToZone(ECardZone Zone, int32 PlayerID, int32 TeamID = -1);

    // Get zone contents
    UFUNCTION(BlueprintPure)
    TArray<FCardInstance> GetZoneContents(ECardZone Zone, int32 PlayerID);

    // Shuffle zone (Deck, Resource Deck)
    UFUNCTION(BlueprintCallable)
    void ShuffleZone(ECardZone Zone, int32 PlayerID);

private:
    // Enforce zone-specific rules
    bool ValidateBattleAreaLimit(int32 PlayerID, int32 TeamID);
    bool ValidateResourceAreaLimit(int32 PlayerID);
};
```

---

## Effect & Keyword System

### Keyword Implementations

#### Repair X

```cpp
// At end of your turn, if Unit has damage, recover X HP
void UGCGEffectExecutor::HandleRepairKeyword(FCardInstance& Unit, int32 RepairAmount)
{
    if (Unit.DamageCounters > 0)
    {
        Unit.DamageCounters = FMath::Max(0, Unit.DamageCounters - RepairAmount);
        BroadcastEvent_OnDamageRecovered(Unit.InstanceID, RepairAmount);
    }
}

// Note: Multiple Repair instances stack (Repair 2 + Repair 1 = Repair 3)
```

#### Breach X

```cpp
// When attack destroys enemy Unit, deal X damage to opponent's shield/base
void UGCGEffectExecutor::HandleBreachKeyword(int32 AttackerInstanceID, int32 BreachAmount, int32 OpponentPlayerID)
{
    // Resolve Breach BEFORE Destroyed triggers
    DealDamageToShields(OpponentPlayerID, BreachAmount);
}

// Multiple Breach instances stack additively
```

#### Blocker

```cpp
// During Block Step, can redirect attack to this Unit
bool UGCGEffectExecutor::CanActivateBlocker(FCardInstance& Blocker)
{
    return Blocker.bIsActive && !Blocker.bIsRested;
}

// Only ONE Blocker activation per attack
// Cannot have multiple copies of Blocker keyword (no stacking)
```

#### First Strike

```cpp
// In Damage Step, deal damage first; no retaliation if target destroyed
void UGCGGameMode::ResolveCombatDamage_FirstStrike(FCardInstance& Attacker, FCardInstance& Defender)
{
    const FCardData* AttackerData = GetCardData(Attacker.CardNumber);

    // Attacker has First Strike
    if (AttackerData->Keywords.Contains("FirstStrike"))
    {
        // Attacker deals damage first
        Defender.DamageCounters += AttackerData->AP;

        // Check if defender is destroyed
        if (Defender.DamageCounters >= GetCardData(Defender.CardNumber)->HP)
        {
            DestroyUnit(Defender);
            return; // Attacker takes no damage
        }
    }

    // Normal simultaneous damage if defender survived
    ResolveCombatDamage_Simultaneous(Attacker, Defender);
}

// Cannot have multiple copies of First Strike (no stacking)
```

#### Suppression

```cpp
// When attacking player, can destroy multiple Shields simultaneously
void UGCGGameMode::ResolveDamageToShields_Suppression(int32 Damage, int32 PlayerID)
{
    TArray<FCardInstance> DestroyedShields;

    // Calculate shields destroyed
    int32 ShieldsToDestroy = Damage; // 1 damage = 1 shield with Suppression

    // Reveal all destroyed shields simultaneously
    for (int32 i = 0; i < ShieldsToDestroy && !Shields.IsEmpty(); ++i)
    {
        FCardInstance Shield = Shields.Pop();
        Shield.bRevealed = true;
        DestroyedShields.Add(Shield);
    }

    // Shield owner chooses Burst resolution order
    if (DestroyedShields.Num() > 1)
    {
        RequestBurstOrderChoice(PlayerID, DestroyedShields);
    }
    else
    {
        ProcessBurst(DestroyedShields[0]);
    }
}
```

#### Link Unit

```cpp
// Unit paired with Pilot satisfying link requirement can attack on deployment turn
bool UGCGGameMode::CanAttackThisTurn(FCardInstance& Unit)
{
    // Newly deployed Units normally cannot attack
    if (Unit.TurnDeployed == CurrentTurnNumber)
    {
        // Check if it's a Link Unit
        return IsLinkUnit(Unit);
    }

    return true; // Units from previous turns can attack
}

bool UGCGGameMode::IsLinkUnit(FCardInstance& Unit)
{
    // Must have a paired Pilot
    if (Unit.PairedCardInstanceID == 0)
        return false;

    FCardInstance* Pilot = FindCardInstance(Unit.PairedCardInstanceID);
    if (!Pilot)
        return false;

    // Validate link requirements (color, traits, etc.)
    const FCardData* UnitData = GetCardData(Unit.CardNumber);
    const FCardData* PilotData = GetCardData(Pilot->CardNumber);

    return ValidateLinkRequirement(UnitData->LinkReq, PilotData);
}
```

### Effect Timing Hooks

```cpp
UENUM(BlueprintType)
enum class EEffectTiming : uint8
{
    // Card Play
    OnDeploy,               // When this card enters Battle Area
    OnPlay,                 // When this card is played from hand

    // Combat
    OnAttack,               // When this Unit attacks
    OnBlock,                // When this Unit blocks
    WhenAttacked,           // When this Unit is attacked

    // Destruction
    OnDestroyed,            // When this card is destroyed
    WhenUnitDestroyed,      // When any Unit is destroyed
    WhenAttackDestroysUnit, // When this Unit's attack destroys enemy

    // Pairing
    WhenPaired,             // When Pilot is paired with Unit
    WhilePaired,            // Continuous while paired

    // Shield
    Burst,                  // When revealed from Shield and destroyed

    // Activated Abilities
    ActivateMain,           // Can activate during Main Phase
    ActivateAction,         // Can activate during Action Step

    // Turn/Phase
    StartOfTurn,
    EndOfTurn,
    StartOfBattle,
    EndOfBattle
};
```

---

## Networking Architecture

### Replication Strategy

**Server Authority**:
- All game rules execute on server
- Server validates all player actions
- Server broadcasts state changes to clients

**Replicated Properties**:

```cpp
// AGCGGameState
UPROPERTY(Replicated)
int32 TurnNumber;

UPROPERTY(Replicated)
ETurnPhase CurrentPhase;

UPROPERTY(Replicated)
int32 ActivePlayerID;

UPROPERTY(Replicated)
TArray<FCardInstance> SharedShieldStack; // 2v2 mode

// AGCGPlayerState
UPROPERTY(Replicated)
TArray<FCardInstance> BattleArea;    // Public

UPROPERTY(Replicated)
TArray<FCardInstance> ResourceArea;  // Public

UPROPERTY(Replicated)
TArray<FCardInstance> Trash;         // Public

UPROPERTY(Replicated, ReplicatedUsing=OnRep_Hand)
TArray<FCardInstance> Hand;          // Private (owner-only)

UPROPERTY(Replicated)
int32 DeckCount;                     // Public count, not contents
```

### Server RPCs

```cpp
// AGCGPlayerController

// Play a card from hand
UFUNCTION(Server, Reliable, WithValidation)
void ServerPlayCard(int32 CardInstanceID, ECardZone TargetZone);

// Declare attack
UFUNCTION(Server, Reliable, WithValidation)
void ServerDeclareAttack(int32 AttackerInstanceID, int32 TargetInstanceID, bool bTargetingPlayer);

// Activate Blocker
UFUNCTION(Server, Reliable, WithValidation)
void ServerActivateBlocker(int32 BlockerInstanceID);

// Activate ability
UFUNCTION(Server, Reliable, WithValidation)
void ServerActivateAbility(int32 SourceInstanceID, int32 AbilityIndex, TArray<int32> TargetInstanceIDs);

// Pass priority (Action Step, Main Phase)
UFUNCTION(Server, Reliable, WithValidation)
void ServerPassPriority();
```

### Hidden Information

```cpp
// AGCGPlayerState::GetLifetimeReplicatedProps
void AGCGPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Hand is owner-only
    DOREPLIFETIME_CONDITION(AGCGPlayerState, Hand, COND_OwnerOnly);

    // Deck contents are hidden (only count is public)
    DOREPLIFETIME(AGCGPlayerState, DeckCount);

    // Public zones
    DOREPLIFETIME(AGCGPlayerState, BattleArea);
    DOREPLIFETIME(AGCGPlayerState, ResourceArea);
    DOREPLIFETIME(AGCGPlayerState, Trash);
}
```

---

## UI/UMG Architecture

### Widget Hierarchy

```
WBP_TCG_Playmat (Full Screen)
├── WBP_OpponentBoard
│   ├── WBP_DeckStack
│   ├── WBP_ResourceDeckStack
│   ├── WBP_ResourceArea (15 slots)
│   ├── WBP_BattleArea (6 Unit slots)
│   ├── WBP_ShieldArea
│   │   ├── WBP_ShieldStack (face-down cards)
│   │   └── WBP_BaseSlot
│   └── WBP_TrashPile
│
├── WBP_LocalPlayerBoard
│   ├── (same structure as opponent)
│   └── WBP_HandZone
│
├── WBP_PhaseIndicator
├── WBP_TurnCounter
└── WBP_ActionLog
```

### Card Widget

**WBP_TCG_Card** (represents a single card):

```
┌──────────────────────────────┐
│ [Color Chip]     [Lv] [Cost] │
│                               │
│    ┌─────────────────────┐   │
│    │                     │   │
│    │     Card Art        │   │
│    │                     │   │
│    └─────────────────────┘   │
│                               │
│  Card Name                    │
│  ───────────────────────────  │
│  [Trait 1] [Trait 2]          │
│                               │
│  Card Text (effects)          │
│                               │
│  ┌──────┐           ┌──────┐ │
│  │  AP  │           │  HP  │ │
│  │ 7000 │           │  5   │ │
│  └──────┘           └──────┘ │
│                               │
│  [Keyword Icons: ⚔️🛡️🔧]       │
│  [Damage Counters: ●●●]       │
└──────────────────────────────┘
```

**Bindings**:
- CardArt → TSoftObjectPtr<UTexture2D>
- CardName → FText
- AP, HP → int32 (calculated with modifiers)
- Keywords → TSet<FName> → Icon visibility
- Damage Counters → int32 → visual indicator
- Rested State → bool → rotation/tilt

### 2v2 Team Battle Layout

```
┌─────────────────────────────────────────────────────────────┐
│                   TEAM B (Opponents)                         │
│  ┌─────────────────────┐   ┌─────────────────────┐          │
│  │  Player 3 Board     │   │  Player 4 Board     │          │
│  │  [Deck] [Resources] │   │  [Deck] [Resources] │          │
│  │  [Battle Area]      │   │  [Battle Area]      │          │
│  └─────────────────────┘   └─────────────────────┘          │
│                                                              │
│              ┌────────────────────────┐                      │
│              │ SHARED SHIELD STACK    │                      │
│              │ [Shield] [Shield] ...  │                      │
│              │ SHARED BASE            │                      │
│              │ [Base Card]            │                      │
│              └────────────────────────┘                      │
│                                                              │
│  ┌─────────────────────┐   ┌─────────────────────┐          │
│  │  Player 1 (You)     │   │  Player 2 (Teammate)│          │
│  │  [Battle Area]      │   │  [Battle Area]      │          │
│  │  [Deck] [Resources] │   │  [Deck] [Resources] │          │
│  └─────────────────────┘   └─────────────────────┘          │
│                   TEAM A (Your Team)                         │
│                                                              │
│  ┌──────────────────────────────────────────────┐           │
│  │            Your Hand                          │           │
│  │  [Card] [Card] [Card] [Card] [Card]          │           │
│  └──────────────────────────────────────────────┘           │
└─────────────────────────────────────────────────────────────┘
```

---

## Team Battle (2v2) Specifics

### Setup Differences

1. **Shared Shield Stack**:
   - Each player contributes 4 cards from top of deck (total 8)
   - Stacked in alternating order: P1, P3, P2, P4, P1, P3, P2, P4 (team leader on bottom)

2. **Shared Base**:
   - Team has 1 Base slot
   - Either team can play/replace Base

3. **Player Two Bonus**:
   - In 1v1: Player Two gets 1 EX Resource token
   - In 2v2: **Both players on Team B** get 1 EX Resource token each

### Gameplay Rules

**Turn Order**:
- Teams alternate: Team A Turn → Team B Turn
- Within a team, both players act **simultaneously** during their turn
- Share all phases (Start, Draw, Resource, Main, End)

**Battlefield Limits**:
- **6 Units max per team total** (not 6 per player!)
- Example: P1 has 4 Units, P2 has 2 Units = 6 total (limit reached)

**Resource/Costs**:
- Each player has their own Resources
- **Cannot use teammate's Resources** to pay costs
- **Cannot pay costs for teammate's cards**

**Friendly/Enemy Definitions**:
- "Friendly" includes teammate's Units
- "You/Your" refers only to card owner, not teammate
- "Opponent" is entire enemy team

**Blocker**:
- A Unit with <Blocker> can block attacks targeting **teammate's Units or player**

**Victory/Defeat**:
- If team takes damage when Shared Shield Stack is empty → team loses
- If any player on a team cannot draw from deck → team loses

**Decision-Making**:
- Teammates can share information and strategies freely
- If teammates disagree on team-wide decisions, **team leader has final say**

### Team Data Structures

```cpp
// In AGCGGameState

USTRUCT(BlueprintType)
struct FTeamInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TArray<int32> PlayerIDs;        // 2 players per team

    UPROPERTY(BlueprintReadOnly)
    int32 TeamLeaderID;             // First player in team

    UPROPERTY(BlueprintReadOnly)
    int32 TotalUnitsOnField;        // Track 6-unit limit

    UPROPERTY(BlueprintReadOnly)
    FCardInstance SharedBase;       // 1 Base for team

    UPROPERTY(BlueprintReadOnly)
    TArray<FCardInstance> SharedShieldStack;  // 8 shields
};

UPROPERTY(Replicated)
FTeamInfo TeamA;

UPROPERTY(Replicated)
FTeamInfo TeamB;
```

---

## Implementation Phases

### Phase 1: Core Data Model (Week 1)

**Deliverables**:
- `GCGTypes.h` with all enums and structs
- DataTable structure for cards
- 10 test cards in CSV format
- Card art placeholder system

**Tasks**:
1. Create `FCardData` struct with all Gundam fields
2. Create `FCardInstance` struct for runtime
3. Define all enums (CardType, CardZone, TurnPhase, Keyword, etc.)
4. Create `FEffectData`, `FActiveModifier` structs
5. Build test DataTable with 10 diverse cards:
   - 2 Units (vanilla)
   - 2 Units with Pilots (Link Units)
   - 1 Unit with Repair
   - 1 Unit with Breach
   - 1 Unit with Blocker + First Strike
   - 1 Command (Main timing)
   - 1 Base card
   - 1 Resource card

---

### Phase 2: Game Mode & State (Week 2)

**Deliverables**:
- `AGCGGameMode` with turn/phase state machine
- `AGCGGameState` with replicated turn info
- `AGCGPlayerState` with zone arrays
- Basic phase progression (no effects yet)

**Tasks**:
1. Implement `AGCGGameMode::StartNewTurn()`
2. Implement `AGCGGameMode::AdvancePhase()`
3. Implement phase handlers:
   - `ExecuteStartPhase()`: Set all cards active, trigger "start" effects (stub)
   - `ExecuteDrawPhase()`: Draw 1 card, check deck empty
   - `ExecuteResourcePhase()`: Place 1 Resource, check Resource Deck empty
   - `ExecuteMainPhase()`: Allow player actions (stub)
   - `ExecuteEndPhase()`: Trigger "end" effects (stub), hand limit
4. Setup replication for `CurrentPhase`, `TurnNumber`, `ActivePlayerID`

---

### Phase 3: Zone Management (Week 3)

**Deliverables**:
- `UGCGZoneSubsystem` for zone operations
- Zone capacity validation
- Card movement with replication

**Tasks**:
1. Implement `MoveCard(CardInstance, FromZone, ToZone, PlayerID)`
2. Implement zone limit checks:
   - Battle Area: 6 Units (1v1) or 6 per team (2v2)
   - Resource Area: 15 max
   - Shield Stack: decreases only
   - Base Section: 1 max
3. Implement shuffle for Deck and Resource Deck
4. Hook zone changes to replication (OnRep functions)

---

### Phase 4: Player Controller & Input (Week 4)

**Deliverables**:
- `AGCGPlayerController` with UI input handling
- Server RPCs for game actions
- Validation for all actions

**Tasks**:
1. Implement `ServerPlayCard(CardInstanceID, TargetZone)`
   - Validate: card in hand, valid zone, meet Lv requirement, afford cost
   - Pay cost: rest Resources (or remove EX Resources)
   - Move card to zone, apply effects
2. Implement `ServerDeclareAttack(AttackerID, TargetID, bTargetPlayer)`
   - Validate: attacker active, in Battle Area, target valid
3. Implement `ServerPassPriority()` for turn/action passing
4. Add client-side UI hooks (ButtonClicked → RPC)

---

### Phase 5: Combat System - Basic (Week 5)

**Deliverables**:
- Attack declaration and targeting
- Damage resolution (no keywords yet)
- Shield reveal and Trash

**Tasks**:
1. Implement `AGCGGameMode::DeclareAttack(AttackData)`
   - Rest attacker
   - Set target (player or Unit)
   - Enter Block Step
2. Implement `AGCGGameMode::ResolveBlockStep()`
   - Wait for Blocker activation (if any)
   - Change target if blocked
3. Implement `AGCGGameMode::ResolveDamageStep()`
   - Target Player: damage → Base HP, then Shields, then defeat
   - Target Unit: simultaneous damage (no First Strike yet)
   - Destroy Units if damage ≥ HP
4. Implement Shield reveal:
   - Pop top Shield, reveal, move to Trash
   - (No Burst handling yet)

---

### Phase 6: Keywords - Part 1 (Week 6)

**Deliverables**:
- Blocker, First Strike, Repair keywords functional

**Tasks**:
1. Implement `UGCGEffectExecutor::HandleBlockerKeyword()`
   - Detect <Blocker> on Units
   - Allow activation during Block Step
   - Redirect attack target
2. Implement `UGCGEffectExecutor::HandleFirstStrikeKeyword()`
   - Modify damage resolution order
   - Attacker deals damage first
   - Check if defender destroyed; if yes, attacker takes no damage
3. Implement `UGCGEffectExecutor::HandleRepairKeyword()`
   - At end of turn, for each Unit with Repair X
   - If DamageCounters > 0, reduce by X
   - Stack multiple Repair instances

---

### Phase 7: Keywords - Part 2 (Week 7)

**Deliverables**:
- Breach, Suppression, Burst keywords functional

**Tasks**:
1. Implement `UGCGEffectExecutor::HandleBreachKeyword()`
   - When Unit with Breach X destroys enemy Unit in combat
   - Deal X damage to opponent's Shield/Base
   - Trigger before Destroyed effects
2. Implement `UGCGGameMode::ResolveSuppressionDamage()`
   - When attacking player, destroy multiple Shields
   - Reveal all destroyed Shields simultaneously
   - Shield owner chooses Burst resolution order
3. Implement `UGCGGameMode::HandleBurst(Shield)`
   - Detect <Burst> keyword on Shield card
   - Offer choice: activate Burst effect or skip
   - Execute Burst effect if chosen

---

### Phase 8: Effect System (Weeks 8-9)

**Deliverables**:
- `UGCGEffectExecutor` with effect parsing and execution
- Support for Deploy, Attack, Destroyed, ActivateMain timings
- Modifier tracking (stat buffs/debuffs)

**Tasks**:
1. Implement effect timing hooks:
   - `OnDeploy`: when card enters Battle Area
   - `OnAttack`: when Unit attacks
   - `OnDestroyed`: when card destroyed
   - `ActivateMain`: manual activation in Main Phase
2. Implement effect operations:
   - Draw cards
   - Deal damage
   - Destroy Units
   - Apply stat modifiers (AP/HP/Cost)
   - Rest/Activate cards
3. Implement modifier system:
   - `ApplyModifier(Target, ModifierType, Amount, Duration)`
   - Track active modifiers on `FCardInstance`
   - Expire modifiers at correct timing (EndOfTurn, EndOfBattle)
4. Parse effect data from DataTable:
   - Read `FEffectData::Operations` array
   - Execute each operation in sequence

---

### Phase 9: Link Units & Pilot Pairing (Week 10)

**Deliverables**:
- Pilot pairing mechanic
- Link Unit validation (can attack on deploy turn)
- Link requirement checking

**Tasks**:
1. Implement `PairPilotWithUnit(PilotInstanceID, UnitInstanceID)`
   - Validate: both in play, Pilot not already paired
   - Set `Unit.PairedCardInstanceID = Pilot.InstanceID`
   - Move Pilot under Unit visually (UI)
2. Implement `ValidateLinkRequirement(LinkReq, Pilot)`
   - Check Pilot color matches
   - Check Pilot traits match
   - Return true if satisfied
3. Modify `CanAttackThisTurn(Unit)`:
   - If Unit deployed this turn, check `IsLinkUnit(Unit)`
   - Link Units can attack immediately
4. Implement "when paired" and "while paired" effect triggers

---

### Phase 10: UI/UMG - Playmat (Weeks 11-12)

**Deliverables**:
- WBP_TCG_Playmat with all zones
- WBP_TCG_Card widget with full bindings
- Drag-and-drop card play from hand
- Click-to-attack targeting

**Tasks**:
1. Create `WBP_TCG_Card`:
   - Bind CardName, AP, HP, Cost, Level to FCardData
   - Bind CardArt to TSoftObjectPtr
   - Display keyword icons (Blocker, Repair, etc.)
   - Display damage counters
   - Rotate/tilt for rested state
2. Create `WBP_TCG_Playmat`:
   - Layout zones: Deck, Resource Deck, Resource Area, Battle Area, Shield, Base, Trash
   - Show opponent board (top) and local player board (bottom)
   - Phase indicator (current phase highlighted)
   - Turn counter
3. Implement drag-and-drop:
   - Drag card from hand
   - Drop on Battle Area → `ServerPlayCard(InstanceID, BattleArea)`
4. Implement click-to-attack:
   - Click Unit → "Attack" button appears
   - Click target (enemy Unit or player) → `ServerDeclareAttack(...)`
5. Hook replication callbacks to UI updates:
   - `OnRep_BattleArea()` → refresh WBP_BattleArea
   - `OnRep_Hand()` → refresh WBP_HandZone

---

### Phase 11: 2v2 Team Battle Mode (Weeks 13-14)

**Deliverables**:
- `AGCGGameMode_2v2` with team logic
- Shared Shield Stack and Shared Base
- Team-wide Unit limit (6 max)
- Team turn system (simultaneous player actions)

**Tasks**:
1. Create `AGCGGameMode_2v2` extending `AGCGGameModeBase`
2. Implement `SetupTeamBattle()`:
   - Assign players to teams (P1, P2 = Team A; P3, P4 = Team B)
   - Each player places 4 cards from deck into Shared Shield Stack
   - Stack in alternating order: P1, P3, P2, P4, P1, P3, P2, P4
   - Place EX Base in Shared Base Section for both teams
   - Give Team B players 1 EX Resource token each
3. Modify zone limits:
   - Track `TeamInfo.TotalUnitsOnField`
   - Validate: `TotalUnitsOnField < 6` before deploying Unit
   - Increment/decrement on Unit deploy/destroy
4. Implement simultaneous team turns:
   - Both players on active team can take actions during shared phases
   - Share priority for Main Phase and Action Steps
5. Modify Blocker:
   - Can block attacks targeting teammate's Units or player
6. Modify victory conditions:
   - If Shared Shield Stack empty and team takes damage → team loses
   - If any team member deck empty → team loses
7. Create `WBP_TeamBattle_Playmat`:
   - Shared Shield/Base in center
   - 4 player boards (2 top, 2 bottom)
   - Visual indicators for teammates

---

### Phase 12: Networking & Replication (Week 15)

**Deliverables**:
- Full server-client architecture
- Hidden information (hands, decks)
- Anti-cheat validation

**Tasks**:
1. Audit all game logic for server authority:
   - All rules execute in `AGCGGameMode` (server-only)
   - Clients only send input via RPCs
2. Setup replication conditions:
   - `Hand`: COND_OwnerOnly
   - `BattleArea`, `ResourceArea`, `Trash`: public (all clients)
   - `DeckCount`: public, but not `Deck` contents
3. Validate all RPCs:
   - Check player owns card
   - Check card in expected zone
   - Check costs can be paid
   - Check timing is legal
4. Test 2-client PIE:
   - Run full match
   - Verify no desyncs
   - Verify hidden info respected

---

### Phase 13: Testing & Polish (Week 16)

**Deliverables**:
- Debug overlay UI
- Automated test scenarios
- Performance optimization

**Tasks**:
1. Create `WBP_DebugOverlay`:
   - Show active modifiers per card
   - Display effect queue
   - Event log (card played, attacked, destroyed)
   - Game state inspector (phase, turn, active player)
2. Create 5 gold test scenarios:
   - Basic 1v1 match to victory
   - Attack with Blocker and First Strike
   - Breach + Shield destruction + Burst
   - Link Unit deployment and attack
   - 2v2 Team Battle full match
3. Performance profiling:
   - Optimize card lookups (hash maps)
   - Minimize replication traffic
   - Lazy-load card art

---

## File Structure

```
GundamTCG/
├── Source/
│   └── GundamTCG/
│       ├── GundamTCG.Build.cs
│       ├── GundamTCG.h
│       ├── GundamTCG.cpp
│       │
│       ├── Core/
│       │   ├── GCGTypes.h                  // All enums, structs
│       │   ├── GCGGameInstance.h/cpp       // Game instance
│       │   └── GCGAssetManager.h/cpp       // Asset loading
│       │
│       ├── GameModes/
│       │   ├── GCGGameModeBase.h/cpp       // Menu/Lobby
│       │   ├── GCGGameMode_1v1.h/cpp       // Standard match
│       │   └── GCGGameMode_2v2.h/cpp       // Team battle
│       │
│       ├── GameState/
│       │   └── GCGGameState.h/cpp          // Replicated game state
│       │
│       ├── PlayerState/
│       │   └── GCGPlayerState.h/cpp        // Player zones, deck
│       │
│       ├── PlayerController/
│       │   └── GCGPlayerController.h/cpp   // Input, RPCs
│       │
│       ├── Subsystems/
│       │   ├── GCGZoneSubsystem.h/cpp      // Zone management
│       │   ├── GCGEffectExecutor.h/cpp     // Effect system
│       │   └── GCGEventBus.h/cpp           // Event delegates
│       │
│       └── AI/
│           └── GCGAIController.h/cpp       // AI opponent (future)
│
├── Content/
│   ├── Cards/
│   │   ├── Data/
│   │   │   ├── DT_AllCards.uasset          // Main card DataTable
│   │   │   └── DT_TestCards.uasset         // Test cards
│   │   │
│   │   ├── Art/
│   │   │   ├── Units/
│   │   │   ├── Pilots/
│   │   │   ├── Commands/
│   │   │   └── Bases/
│   │   │
│   │   └── Effects/
│   │       └── Particles, Sounds
│   │
│   ├── UI/
│   │   ├── WBP_TCG_Card.uasset
│   │   ├── WBP_TCG_Playmat.uasset
│   │   ├── WBP_TCG_Playmat_2v2.uasset
│   │   ├── WBP_HandZone.uasset
│   │   ├── WBP_BattleArea.uasset
│   │   ├── WBP_ShieldArea.uasset
│   │   ├── WBP_PhaseIndicator.uasset
│   │   └── WBP_DebugOverlay.uasset
│   │
│   ├── Maps/
│   │   ├── MainMenu.umap
│   │   ├── Match_1v1.umap
│   │   └── Match_2v2.umap
│   │
│   └── Testing/
│       └── GoldTestScenarios/
│
└── Config/
    ├── DefaultEngine.ini
    └── DefaultGame.ini
```

---

## Best Practices

### C++ Coding Standards

1. **Naming Conventions**:
   - Classes: `AGCGGameMode`, `UGCGZoneSubsystem`
   - Structs: `FCardData`, `FCardInstance`
   - Enums: `ECardType`, `ETurnPhase`
   - Functions: `MoveCard()`, `DeclareAttack()`
   - Variables: `CurrentPhase`, `bIsActive`

2. **Memory Management**:
   - Use `TArray`, `TMap`, `TSet` for collections
   - Use `TSoftObjectPtr` for card art (lazy loading)
   - Avoid raw pointers; use `TWeakObjectPtr` or `TSharedPtr`

3. **Replication**:
   - Always use `UPROPERTY(Replicated)` for networked data
   - Implement `GetLifetimeReplicatedProps()` in replicated classes
   - Use `ReplicatedUsing=OnRep_Function` for client-side reactions

4. **Validation**:
   - All Server RPCs must have `_Validation()` function
   - Validate: ownership, zone, costs, timing
   - Return `false` to reject invalid RPCs

### Data-Driven Design

1. **No Hardcoded Cards**:
   - All card stats and effects in DataTables
   - Use `FCardData` lookup by `CardNumber`

2. **Effect System**:
   - Effects defined as data (FEffectData)
   - Effect executor interprets and runs effects
   - New effects = new operations, not new classes

3. **Keywords**:
   - Keywords stored as `TSet<FName>` in `FCardData`
   - Keyword handlers in `UGCGEffectExecutor`
   - Easy to add new keywords

### Performance Optimization

1. **Minimize Replication**:
   - Only replicate public zones (BattleArea, ResourceArea, Trash)
   - Private zones (Hand, Deck) are owner-only or count-only
   - Use `COND_OwnerOnly` for hands

2. **Asset Loading**:
   - Use `TSoftObjectPtr` for card art
   - Load on-demand, not all at startup
   - Unload unused art after matches

3. **Card Lookups**:
   - Use `TMap<int32, FCardInstance>` for fast InstanceID lookup
   - Cache `FCardData` pointers after DataTable lookup

---

## Conclusion

This architecture provides a **clean, extensible, and networked foundation** for the Gundam TCG in Unreal Engine 5.6. By following the phased implementation plan, you will build a fully playable 1v1 and 2v2 digital card game with:

- **Data-driven cards** (no hardcoded logic)
- **Full keyword support** (Repair, Breach, Blocker, First Strike, etc.)
- **Effect system** (Deploy, Attack, Destroyed, ActivateMain, Burst)
- **Robust networking** (server authority, hidden information)
- **Team Battle mode** (shared shields, shared base, simultaneous turns)
- **Polished UI** (drag-and-drop, click-to-attack, animated cards)

**Next Steps**:
1. Review this document
2. Begin Phase 1: Core Data Model
3. Create `GCGTypes.h` with all structs and enums
4. Build test card DataTable
5. Iterate through phases 2-13

Good luck, and enjoy building the Gundam TCG! 🤖🎴

---

**END OF ARCHITECTURE DOCUMENT**
