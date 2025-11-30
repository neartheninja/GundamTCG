# Gundam TCG - Official FAQ Integration TODO

This document tracks the integration of official Gundam TCG FAQ rulings into the implementation.

Last Updated: 2025-11-15
Version: 2.0.0-alpha

---

## ✅ Already Implemented

### Keywords & Timings
- [x] **Burst** (Q80-83) - `EGCGKeyword::Burst` exists
  - Location: GCGTypes.h:100
  - Implementation: GCGKeywordSubsystem.cpp:ProcessBurst()
  - Note: Need to verify optional activation (Q81)

- [x] **Deploy** (Q65) - `EGCGEffectTiming::OnDeploy` exists
  - Location: GCGTypes.h:135
  - Triggers when card enters Battle Area

- [x] **Attack** (Q66-67) - `EGCGEffectTiming::OnAttack` exists
  - Location: GCGTypes.h:137
  - Triggers when Unit declares attack

- [x] **Destroy** (Q68-69) - `EGCGEffectTiming::OnDestroyed` exists
  - Location: GCGTypes.h:141
  - Triggers when card is destroyed and placed in trash
  - Note: Q69 says effect activates in trash, referencing previous state

- [x] **When Paired** (Q74-75) - `EGCGEffectTiming::WhenPaired` exists
  - Location: GCGTypes.h:146
  - Triggers when Pilot is paired with Unit
  - Supports qualified requirements (e.g., Zeon Pilot only)

- [x] **During Pair** (Q76-77) - `EGCGEffectTiming::WhilePaired` exists
  - Location: GCGTypes.h:147
  - Active while Unit is paired with Pilot
  - Can combine with other timings (e.g., WhilePaired + OnAttack)

- [x] **Once per Turn** (Q78-79) - `bOncePerTurn` in `FGCGEffectData`
  - Location: GCGTypes.h:300
  - Tracked per card instance (Q79: each copy activates separately)

### Link Units & Pilots
- [x] **Link Unit Pairing** (Q89)
  - Implementation: GCGLinkUnitSubsystem (Phase 9)
  - Link requirements can be unfulfilled but AP/HP still added
  - Summoning sickness bypass only when requirements fulfilled

- [x] **Pilot Rules** (Q85-89)
  - Q85: Pilots can't be placed alone (enforced in validation)
  - Q86-87: Can't swap Pilots or move Pilot to another Unit
  - Q88: Pilot effects resolve as though they belong to Unit
  - Implementation: GCGLinkUnitSubsystem

### Tokens
- [x] **Token Properties** (Q101-104)
  - Q101: Tokens have no color (Colors array empty)
  - Q102: Tokens have Lv = 0, Cost = 0
  - Q103: Pilots can pair with Unit tokens
  - Q104: Tokens count toward 6 Unit limit
  - Implementation: CreateTokenInstance() in GCGGameModeBase

### Commands
- [x] **Command Timing** (Q90-92, Q163)
  - Q90: Main timing (during main phase)
  - Q91: Action timing (during action steps)
  - Q92: Can have both Main/Action
  - Q163: Command+Pilot dual-type cards
  - Note: Main timing exists, Action timing needs to be added

---

## ✅ COMPLETED (v2.0.0-alpha Updates)

### 1. Activate·Action Timing (Q70-73)
**Status**: ✅ COMPLETE (Already existed)

**Implementation**:
- ✅ `ActivateAction` exists in `EGCGEffectTiming` (line 164)
- ✅ Can be triggered during action steps
- ⚠️ Numerical cost symbols (①②③) - TODO: Add to FGCGEffectCost
- ⚠️ "No conditions = just declare" - TODO: Add to effect execution

### 2. AP/HP Bounds (Q94-96)
**Status**: ✅ COMPLETE (Already correct)

**Implementation**:
- ✅ Q94: AP can't go below 0 → `FMath::Max(0, TotalAP)` in GetTotalAP() (line 712)
- ✅ Q95: Modifiers stack additively → Correctly implemented in loop
- ✅ Q96: HP checks use current HP → GetTotalHP() returns HP after modifiers

### 3. Damage Source Tracking (Q97-99)
**Status**: ✅ COMPLETE (v2.0.0-alpha)

**Implementation**:
- ✅ Added `EGCGDamageSource` enum (Battle, Effect, Shield)
- ✅ Added `LastDamageSource` field to FGCGCardInstance
- ✅ Initialized in constructor
- ⚠️ TODO: Update damage dealing functions to set LastDamageSource

**Code Locations**:
- GCGTypes.h:194-200 - EGCGDamageSource enum
- GCGTypes.h:708 - FGCGCardInstance::LastDamageSource field
- GCGTypes.h:726 - Constructor initialization

### 4. Target Scope System (Q84)
**Status**: ✅ COMPLETE (v2.0.0-alpha)

**Implementation**:
- ✅ Added `EGCGTargetScope` enum (Self, YourUnits, FriendlyUnits, EnemyUnits, AllUnits, etc.)
- ✅ Added `TargetScope` field to FGCGEffectOperation
- ✅ Initialized in constructor
- ⚠️ TODO: Update effect execution to filter targets by scope

**Code Locations**:
- GCGTypes.h:207-217 - EGCGTargetScope enum
- GCGTypes.h:324 - FGCGEffectOperation::TargetScope field
- GCGTypes.h:342 - Constructor initialization

---

## ❌ Needs Implementation

### Priority 1: Critical Mechanics

#### 1. Numerical Cost Symbols (Q72)
**Status**: ⚠️ MISSING

**Requirements**:
- [ ] Add numerical cost symbols (①②③④⑤) to `FGCGEffectCost`
- [ ] Implement "no conditions = just declare" activation (Q73)

**Code Locations**:
- GCGTypes.h: FGCGEffectCost structure
- FGCGEffectCost: Add `ActivateCost` field (int32)
- GCGEffectSubsystem: Check ActivateCost before allowing activation

**FAQ References**:
- Q72: ①②③ = numerical cost (pay resources)
- Q73: No conditions = just declare activation

---

#### 2. Target Validation (Q100)
**Status**: ⚠️ MISSING

**Requirements**:
- [ ] Add target validation to effect execution
- [ ] Prevent activation if no valid targets exist
- [ ] Add to CanPayCosts() check in GCGEffectSubsystem

**Code Locations**:
- GCGEffectSubsystem: Add ValidateTargets() before CheckConditions()
- FGCGEffectData: Add target selection metadata

**FAQ Reference**:
- Q100: Can't activate effect if target selection required but no targets available

**Example**:
```cpp
// Before executing effect:
if (Effect.RequiresTarget && GetValidTargets(Effect).Num() == 0)
{
    return false; // Can't activate
}
```

---

### Priority 2: Effect Resolution System

#### 3. Effect Resolution Order & Priority (Q105-112)
**Status**: ⚠️ MISSING - COMPLEX SYSTEM

**Requirements**:
- [ ] Continuous effects only affect Units in play at activation (Q105)
- [ ] "During this turn" effects persist if source destroyed (Q106)
- [ ] Active player resolves effects first, chooses order (Q107-108)
- [ ] New effects interrupt and resolve first (Q109)
- [ ] Burst effects get priority when triggered (Q110)
- [ ] Effects resolve even if source leaves field (Q111)
- [ ] Negation effects have priority (Q112)

**Code Locations**:
- NEW: GCGEffectStackSubsystem (needs creation)
- GCGEffectSubsystem: Integrate with stack
- GCGGameMode: Manage effect resolution timing

**FAQ References**:
- Q105: "All your Units get AP+2" only affects Units in play NOW
- Q106: "During this turn" effects persist even if source destroyed
- Q107-108: Active player resolves all effects first
- Q109: New effects interrupt and resolve with priority
- Q110: Burst interrupts resolution
- Q111: Effects already on stack resolve even if source destroyed
- Q112: Negation effects override other effects

**Design Notes**:
This requires a full **Effect Stack System** similar to Magic: The Gathering's stack:

```cpp
USTRUCT(BlueprintType)
struct FGCGEffectStackEntry
{
    UPROPERTY()
    int32 SourceCardInstanceID;

    UPROPERTY()
    FGCGEffectData EffectData;

    UPROPERTY()
    int32 OwnerPlayerID;

    UPROPERTY()
    bool bIsResolved = false;

    UPROPERTY()
    int32 Priority; // Higher = resolve first
};

class UGCGEffectStackSubsystem
{
    TArray<FGCGEffectStackEntry> EffectStack;

    void PushEffect(FGCGEffectStackEntry Entry);
    FGCGEffectStackEntry PopEffect(); // LIFO
    void ResolveStack();
    void InsertWithPriority(FGCGEffectStackEntry Entry);
};
```

**Resolution Order**:
1. Burst effects (highest priority)
2. New effects triggered during resolution
3. Active player's effects (in player's chosen order)
4. Standby player's effects (in player's chosen order)
5. Negation effects override at any time

---

## 📋 Implementation Plan

### Phase A: Critical Mechanics (Week 1)
1. Add `ActivateAction` timing
2. Add numerical cost symbols (①②③)
3. Add damage source tracking
4. Add target validation
5. Fix AP bounds (min 0)

### Phase B: Targeting System (Week 1)
6. Add `ETargetScope` enum
7. Implement Friendly vs Your targeting
8. Update effect operations to use scope

### Phase C: Effect Stack System (Week 2)
9. Create `GCGEffectStackSubsystem`
10. Implement priority-based resolution
11. Implement interrupt handling (Burst, new effects)
12. Implement continuous vs one-shot effects
13. Implement negation system

### Phase D: Testing & Validation (Week 2)
14. Write tests for each FAQ ruling
15. Create test cards demonstrating each mechanic
16. Validate with GCGValidationSubsystem

---

## 🧪 Test Cases

### Test Case 1: Activate·Action (Q70-73)
```
Card: Test Unit with "【Activate･Action】①: Draw 1 card"
Test 1: Can activate during my action step ✓
Test 2: Can activate during opponent's action step ✓
Test 3: Must pay 1 resource cost ✓
Test 4: Can't activate if no resources ✓
Test 5: Card with no conditions activates by declaration only ✓
```

### Test Case 2: Damage Persistence (Q99)
```
Card: Unit with 5 HP
Action: Deal 2 damage
Expected: DamageTaken = 2 at end of turn (doesn't reset) ✓
```

### Test Case 3: AP Minimum (Q94)
```
Card: Unit with 3 AP
Action: Apply "AP -5" modifier
Expected: GetTotalAP() returns 0, not -2 ✓
```

### Test Case 4: Effect Resolution Order (Q107-108)
```
Scenario: Active player has 2 effects trigger simultaneously
Expected: Active player chooses order of resolution ✓
Expected: Both resolve before standby player's effects ✓
```

### Test Case 5: Burst Priority (Q110)
```
Scenario: Breach destroys Shield with Burst
Expected: Burst resolves before continuing Breach resolution ✓
```

---

## 📝 Notes

- **Backward Compatibility**: All new additions must not break existing Phase 1-11 implementations
- **Data-Driven**: New mechanics should be definable in DataTables where possible
- **Testing**: Every FAQ ruling should have automated test coverage
- **Documentation**: Update GUNDAM_TCG_UE5_ARCHITECTURE.md with new mechanics

---

## 🔗 References

- Official Gundam TCG FAQ: [User-provided Q70-Q112]
- Implementation Plan: IMPLEMENTATION_PLAN.md
- Architecture Doc: GUNDAM_TCG_UE5_ARCHITECTURE.md
- Type Definitions: Source/OnePieceTCG_V2/GCGTypes.h
