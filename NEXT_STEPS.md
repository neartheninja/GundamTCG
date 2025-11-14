# One Piece TCG - Next Development Steps

## ✅ Completed
- [x] HUD Widget (WBP_TCG_HUD)
- [x] Card Widget (WBP_TCG_Card)
- [x] Card Actor (BP_CardActor)
- [x] Data Tables and CSV system
- [x] C++ Game State classes (PlayerState, GameMode, Types)
- [x] **Hand Display Widget (Step 1) - C++ Implementation Complete**

---

## 🎯 Step 1: Hand Display Widget (Current - Requires Unreal Editor)

### Status: C++ Code Complete ✅ | Blueprint Setup Required ⚠️

**What's Been Done:**
- Created `TCGHandWidget.h` and `TCGHandWidget.cpp`
- Implemented all core functionality:
  - Dynamic card spawning
  - Add/remove cards from hand
  - Card selection system
  - Hover effect support
  - Click handling infrastructure

**What You Need To Do:**
1. Open Unreal Editor
2. Compile the C++ code (Build → Build Solution)
3. Create `WBP_TCG_Hand` Blueprint (follow HAND_WIDGET_SETUP.md)
4. Test with sample data

**Files Created:**
- `/Source/OnePieceTCG_V2/TCGHandWidget.h` - Header with all properties
- `/Source/OnePieceTCG_V2/TCGHandWidget.cpp` - Implementation
- `/HAND_WIDGET_SETUP.md` - Complete setup guide

**Time to Complete:** 30-45 minutes (mostly in Unreal Editor)

---

## 🔜 Step 2: Click-to-Play Functionality (Next Priority)

**Goal:** Player clicks card in hand → card plays to board

### Implementation Plan:

#### A. Update WBP_TCG_Card (Blueprint)
1. Add transparent Button widget covering entire card
2. Bind `OnClicked` event:
   ```
   OnClicked → GetOwningPlayerController → PlayCardFromHand(CardIndex)
   ```

#### B. Add PlayCardFromHand to PlayerController (C++)

Create in `TCGPlayerController.h`:
```cpp
UFUNCTION(BlueprintCallable, Category = "Cards")
bool PlayCardFromHand(int32 HandIndex);
```

Implementation logic:
```cpp
bool ATCGPlayerController::PlayCardFromHand(int32 HandIndex)
{
    ATCGPlayerState* PS = GetPlayerState<ATCGPlayerState>();
    if (!PS || HandIndex < 0 || HandIndex >= PS->Hand.Num())
        return false;

    FCardData Card = PS->Hand[HandIndex];

    // Check if player has enough DON
    if (PS->AvailableDon < Card.Cost)
    {
        // Show "Not enough DON" message
        return false;
    }

    // Deduct DON cost
    PS->AvailableDon -= Card.Cost;

    // Play the card
    if (Card.CardType == ECardType::CHARACTER)
    {
        PS->PlayCharacter(HandIndex);
    }
    else if (Card.CardType == ECardType::STAGE)
    {
        // Add to StageZone
        PS->StageZone.Add(Card);
        PS->Hand.RemoveAt(HandIndex);
    }
    // ... handle other card types

    // Spawn visual actor on board
    SpawnCardActorOnBoard(Card);

    return true;
}
```

#### C. Update Hand Widget
- Connect `OnCardClicked` event to controller's `PlayCardFromHand()`
- Remove card from hand display after playing

**Files to Modify:**
- `Source/OnePieceTCG_V2/TCGPlayerController.h` (add function)
- `Source/OnePieceTCG_V2/TCGPlayerController.cpp` (implement logic)
- `Content/WBP_TCG_Card.uasset` (add button + click event)
- `Content/WBP_TCG_Hand.uasset` (connect events)

**Time Estimate:** 2-3 hours

---

## 🔜 Step 3: Phase Transition Button (High Priority)

**Goal:** Add "End Phase" button to HUD

### Implementation:

#### A. Add Button to WBP_TCG_HUD
1. Open `WBP_TCG_HUD` in Unreal Editor
2. Add Button widget:
   - Text: "End Main Phase" (dynamic based on current phase)
   - Position: Bottom-right corner
   - Style: Large, prominent button

#### B. Wire to Game Mode

In Blueprint Graph:
```
Event Button_EndPhase → OnClicked
  → GetGameMode
  → Cast to TCGGameMode
  → Call: AdvancePhase()
  → Update button text based on new phase
```

#### C. Dynamic Button Text

Add logic to update button text:
- Main Phase → "End Main Phase"
- Battle Phase → "End Battle Phase"
- Your Turn Ending → "End Turn"
- Opponent's Turn → Disabled

**Files to Modify:**
- `Content/WBP_TCG_HUD.uasset` (add button + event)

**Time Estimate:** 1 hour

---

## 📋 Step 4: Bridge C++ to Blueprint (Critical)

**Goal:** Make C++ game state automatically update UI

### Implementation:

#### A. Create Blueprint Events in PlayerState

Already done in `TCGPlayerState.h`:
- `OnHandUpdated()` ✅
- `OnLifeUpdated()` ✅
- `OnDonZoneUpdated()` ✅
- `OnCharacterZoneUpdated()` ✅

#### B. Implement in BP_TCGPlayerState

1. Open `BP_TCGPlayerState`
2. For each event, add:

```
Event OnHandUpdated
  → Get HUD Widget
  → HandWidget → UpdateHandDisplay(Hand)
```

```
Event OnLifeUpdated
  → Get HUD Widget
  → UpdateLifeDisplay(Life.Num())
```

```
Event OnDonZoneUpdated
  → Get HUD Widget
  → UpdateDonDisplay(AvailableDon, DonZone.Num())
```

#### C. Test with 2 Players

1. Editor → Play → New Editor Window (PIE)
2. Set Number of Players: 2
3. Verify both players see their own hands
4. Test card playing and state updates

**Files to Modify:**
- `Content/BP_TCGPlayerState.uasset` (implement events)
- `Content/WBP_TCG_HUD.uasset` (add update functions)

**Time Estimate:** 3-4 hours

---

## 🎨 Step 5: Polish - Card Animations (Medium Priority)

**Goal:** Add smooth animations for card actions

### Animations to Create:

1. **Draw Animation** (card slides in from deck)
2. **Play Animation** (card moves from hand to board)
3. **Hover Animation** (already planned in Step 1)
4. **Rest Animation** (card rotates 90 degrees)
5. **Attack Animation** (card lunges forward)

### Implementation:

In `WBP_TCG_Card`, create animations:

```
Animation: DrawCard
  - Start: Off-screen right (X: +1000)
  - End: Hand position (X: 0)
  - Duration: 0.5s
  - Curve: Ease Out

Animation: PlayCard
  - Start: Hand position
  - End: Board position
  - Duration: 0.3s
  - Curve: Ease In-Out

Animation: RestCard
  - Rotation: 0° → 90°
  - Duration: 0.2s
```

**Time Estimate:** 1-2 hours

---

## 🎮 Step 6: DON System UI (High Priority)

**Goal:** Visual DON counter and attachment system

### Implementation:

#### A. Create WBP_DONCounter Widget

Structure:
```
Canvas Panel
  └── Horizontal Box (DON cards)
       ├── Image (DON Active) x N
       └── Image (DON Rested) x M
```

#### B. DON Attachment UI

Add to WBP_TCG_Card:
- Small indicator showing attached DON count
- "+1000 per DON" power display
- Glow effect when DON attached

#### C. Attach DON Button

In HUD or card context menu:
```
Event AttachDON_Clicked
  → Get Selected Character
  → PlayerState → AttachDonToCharacter(CharacterID)
  → Update visuals
```

**Files to Create:**
- `Content/WBP_DONCounter.uasset` (new widget)

**Files to Modify:**
- `Content/WBP_TCG_Card.uasset` (add DON indicator)
- `Content/WBP_TCG_HUD.uasset` (integrate DON counter)

**Time Estimate:** 2 hours

---

## ⚔️ Step 7: Attack Declaration UI (High Priority)

**Goal:** Click-to-attack system with visual feedback

### Implementation:

#### A. Attack Flow

1. Player clicks their character (attacker)
2. Valid targets highlight (enemies + leader)
3. Player clicks target
4. Draw attack arrow from attacker to target
5. Trigger block/counter UI for defender

#### B. Visual Indicators

- **Green Glow**: Valid attacker (active, has power)
- **Red Glow**: Valid target
- **Arrow Widget**: Dynamic line from attacker to target
- **Pulse Animation**: Attack resolving

#### C. Code Structure

In `TCGPlayerController.h`:
```cpp
UFUNCTION(BlueprintCallable, Category = "Combat")
void SelectAttacker(int32 CharacterInstanceID);

UFUNCTION(BlueprintCallable, Category = "Combat")
void SelectTarget(int32 TargetInstanceID);

UFUNCTION(BlueprintCallable, Category = "Combat")
void ConfirmAttack();
```

**Time Estimate:** 3-4 hours

---

## 🛡️ Step 8: Block & Counter UI (High Priority)

**Goal:** Defender chooses blockers and counter cards

### Implementation:

#### A. Create WBP_DefenderChoice Widget

Popup showing:
- Attacking card (preview)
- Current attack power
- Options:
  - Block with character
  - Play counter card
  - Take damage

#### B. Block Selection

Show all valid blockers:
- Active characters
- Click to select blocker
- Confirm button

#### C. Counter Card Selection

Show hand with:
- Only counter cards highlighted
- Click to play counter
- Deduct cost from DON
- Add counter value to defense

**Files to Create:**
- `Content/WBP_DefenderChoice.uasset` (popup widget)

**Time Estimate:** 3-4 hours

---

## 🧪 Step 9: Full Gameplay Test (Critical)

**Goal:** Run a complete game from start to finish

### Test Checklist:

- [ ] Both players spawn with correct starting hands
- [ ] Draw phase works correctly
- [ ] DON phase adds DON to zones
- [ ] Cards can be played from hand
- [ ] DON cost is deducted correctly
- [ ] Characters can attack
- [ ] Blocking works
- [ ] Counter cards work
- [ ] Damage is applied to life cards
- [ ] Game ends when a player runs out of life
- [ ] Turn transitions work smoothly

### Test Setup:

1. **Play in Editor (PIE)**:
   ```
   Editor → Play → New Editor Window
   Number of Players: 2
   Net Mode: Play Offline
   ```

2. **Run Full Game**:
   - Player 1 plays cards
   - Player 1 attacks
   - Player 2 blocks/counters
   - Continue until win condition

3. **Log All Issues** in a test document

**Time Estimate:** 2-3 hours

---

## 🐛 Step 10: Bug Fixes & Polish (Ongoing)

**Goal:** Address issues found during testing

### Common Issues to Watch For:

- **Replication errors** (multiplayer state sync)
- **UI not updating** after state changes
- **Memory leaks** from spawned widgets
- **Card visuals not matching data**
- **Click detection issues**
- **Animation glitches**

### Polish Tasks:

- [ ] Add sound effects (card play, attack, damage)
- [ ] Improve button hover states
- [ ] Add tooltips to UI elements
- [ ] Optimize card spawning performance
- [ ] Add loading screens
- [ ] Add game options menu

**Time Estimate:** Ongoing

---

## 📚 Resources for Next Steps

### Unreal Engine Documentation:
- **UMG Widgets**: https://docs.unrealengine.com/5.6/en-US/umg-ui-designer-for-unreal-engine/
- **Replication**: https://docs.unrealengine.com/5.6/en-US/networking-and-multiplayer-in-unreal-engine/
- **Slate UI**: https://docs.unrealengine.com/5.6/en-US/slate-ui-framework-for-unreal-engine/

### One Piece TCG Rules:
- Review official rulebook for:
  - Attack timing
  - Block rules
  - Counter card usage
  - Trigger effects

---

## 🚀 Quick Start (Resume Development)

When you're ready to continue:

1. **Open Unreal Editor**
2. **Complete Step 1**: Follow `HAND_WIDGET_SETUP.md`
3. **Test Hand Widget** with sample data
4. **Move to Step 2**: Implement click-to-play
5. **Iterate**: Test → Fix → Polish

---

## 📝 Development Tips

1. **Test Early, Test Often**
   - Test each feature immediately after implementing
   - Don't wait until everything is done

2. **Use Debug Prints**
   - Add `UE_LOG()` statements liberally
   - Print card data, player state, phase transitions

3. **Save Frequently**
   - Blueprint work can crash the editor
   - Save after every major change

4. **Version Control**
   - Commit after each completed step
   - Use descriptive commit messages

5. **Ask for Help**
   - Unreal Engine forums
   - Discord communities
   - Stack Overflow

---

## 🎯 Milestone Goals

**Milestone 1: Playable Prototype** (Steps 1-5)
- Can play cards from hand
- Can see game state in HUD
- Basic turn flow works

**Milestone 2: Combat System** (Steps 6-8)
- DON system functional
- Attacking works
- Blocking/counters work

**Milestone 3: Complete Game** (Steps 9-10)
- Full gameplay loop
- Multiplayer works
- Polished UI

---

## ✨ Future Enhancements

After completing the core game:

- **Deck Builder**: In-game deck construction
- **Card Collection**: Track owned cards
- **AI Opponent**: Single-player mode
- **Replay System**: Watch previous games
- **Tournament Mode**: Bracket-style competition
- **Card Effects System**: Implement all card abilities
- **Online Multiplayer**: Steam/Epic integration
- **Mobile Support**: Touch controls

---

**Current Focus:** Complete Step 1 (Hand Widget Blueprint) in Unreal Editor

Good luck! 🎴⚓
