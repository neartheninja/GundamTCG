# Hand Widget Setup Guide

## Overview
This guide will walk you through setting up the **WBP_TCG_Hand** widget - a horizontal hand display for your One Piece TCG game.

## What We've Created

### C++ Base Classes
1. **TCGHandWidget.h** - Header file with all properties and functions
2. **TCGHandWidget.cpp** - Implementation of the hand widget logic

### Key Features
- ✅ Horizontal card layout with customizable spacing
- ✅ Dynamic card spawning from array data
- ✅ Card selection system
- ✅ Hover effect support (implemented in Blueprint)
- ✅ Click handling for playing cards
- ✅ Integration with existing FCardData structure

---

## Step-by-Step Setup

### Step 1: Compile the C++ Code

1. **Close Unreal Editor** (if it's open)
2. **Right-click** on `OnePieceTCG_V2.uproject`
3. Select **"Generate Visual Studio project files"** (or use your IDE)
4. **Open the solution** in your IDE
5. **Build the project** (Build → Build Solution)
6. **Launch Unreal Editor** from your IDE or by opening the .uproject file

**Alternative (Command Line on Linux):**
```bash
cd /home/user/OnePieceTCG_V2
./Engine/Build/BatchFiles/Linux/Build.sh OnePieceTCG_V2Editor Linux Development -project=/home/user/OnePieceTCG_V2/OnePieceTCG_V2.uproject
```

---

### Step 2: Create the Blueprint Widget (WBP_TCG_Hand)

1. **Open Unreal Editor**
2. In the **Content Browser**, navigate to your main Content folder
3. **Right-click** → **User Interface** → **Widget Blueprint**
4. Name it: **`WBP_TCG_Hand`**
5. **Double-click** to open the Widget Blueprint editor

---

### Step 3: Configure the Widget Blueprint

#### A. Set the Parent Class
1. In the **Class Settings** (top toolbar), click **"Class Options"**
2. Set **Parent Class** to: **`TCGHandWidget`** (the C++ class we created)
3. **Compile** the blueprint

#### B. Design the Widget Layout

**Root Widget:**
- Add a **Canvas Panel** as the root (or keep the default)

**Main Container:**
1. Add a **Horizontal Box** to the Canvas Panel
2. Rename it to: **`CardContainer`** (IMPORTANT - this matches the C++ binding)
3. Set its anchors to bottom-center:
   - Anchors: Bottom-Center
   - Position X: 0
   - Position Y: -50 (slight offset from bottom)
   - Alignment: (0.5, 1.0)
   - Size To Content: ☑️ Checked

**Layout Settings for CardContainer:**
- In the **Details Panel**:
  - **Horizontal Alignment**: Center
  - **Vertical Alignment**: Bottom

---

### Step 4: Configure Widget Settings

In the **Details Panel** (with WBP_TCG_Hand selected), you'll see new categories from the C++ class:

#### Hand Settings
- **Card Widget Class**: Select **`WBP_TCG_Card`** (your existing card widget)
- **Max Cards In Hand**: `10`
- **Card Spacing**: `10.0`
- **Card Width**: `180.0`
- **Card Height**: `252.0`
- **Card Scale**: `1.0`

#### Hover Settings
- **Hover Lift Amount**: `30.0` (pixels)
- **Hover Scale Amount**: `1.1` (110% size)
- **Hover Animation Duration**: `0.2` (seconds)

#### Selection Settings
- **Selected Card Tint**: Yellow (RGB: 1.0, 1.0, 0.5)

---

### Step 5: Implement Card Hover Effects (Blueprint)

Since hover effects need to be visual, we'll implement them in Blueprint:

1. Open **WBP_TCG_Card** (your card widget)
2. In the **Designer** tab, select the root widget or main image
3. Go to the **Graph** tab

#### Add Hover Events:

**On Mouse Enter:**
```
Event On Mouse Enter
  → Play Animation (Create animation: "HoverUp")
     - Translation: Y = -30 (moves up)
     - Scale: 1.1
     - Duration: 0.2s
```

**On Mouse Leave:**
```
Event On Mouse Leave
  → Play Animation (Create animation: "HoverDown")
     - Translation: Y = 0 (returns to normal)
     - Scale: 1.0
     - Duration: 0.2s
```

#### Create the Animations:
1. Click **"+ Animation"** at bottom of the UMG editor
2. Name it: **`HoverUp`**
3. Add tracks for:
   - **Render Transform** → Translation Y: `0` to `-30`
   - **Render Transform** → Scale: `1.0` to `1.1`
4. Set duration: `0.2` seconds

Repeat for **`HoverDown`** (reverse values)

---

### Step 6: Add Click Handling (Blueprint)

In **WBP_TCG_Card**:

1. Add a **Button** widget that covers the entire card
   - Name it: `CardButton`
   - Make it invisible: Set **Style → Normal → Tint** to transparent (alpha = 0)
   - Set **Size to Content**: ☐ Unchecked
   - Anchors: Fill entire parent

2. In the **Graph**, add:

```
Event CardButton → On Clicked
  → Get Owning Player
  → Get Player Controller
  → Get HUD
  → Cast to TCGHandWidget (or find the hand widget reference)
  → Call: OnCardClicked(CardIndex, CardData)
```

---

### Step 7: Connect to Player State

In your **BP_TCGPlayerController** or **BP_TCGPlayerState**:

1. Add a reference to **WBP_TCG_Hand**
2. Create an **Event BeginPlay** node
3. Add:

```
Event Begin Play
  → Create Widget (Class: WBP_TCG_Hand)
  → Add to Viewport
  → Store reference as: HandWidgetRef
```

4. Bind the hand update event:

```
Event OnRep_Hand (from PlayerState)
  → HandWidgetRef → UpdateHandDisplay(Hand Array)
```

---

### Step 8: Test the Hand Widget

#### Test Setup 1: Manual Test in HUD

1. Open **WBP_TCG_HUD**
2. Add **WBP_TCG_Hand** as a child widget
3. Position it at the bottom of the screen
4. Create a test function:

```
Event Construct (or any test event)
  → Create Test Cards Array (size: 5)
  → For each card:
     - Set CardID, CardName, Cost, Power
     - Add to array
  → Call: WBP_TCG_Hand → UpdateHandDisplay(Test Cards Array)
```

#### Test Setup 2: Use Demo Data

If you have the demo deck data from previous setup:

```
Event Construct
  → Get Player State
  → Cast to TCGPlayerState
  → Get: Hand (array)
  → WBP_TCG_Hand → UpdateHandDisplay(Hand)
```

---

## Usage Examples

### Adding a Card to Hand
```cpp
// In C++
HandWidget->AddCardToHand(NewCardData);
```

```
// In Blueprint
HandWidgetRef → Add Card to Hand(New Card Data)
```

### Removing a Card When Played
```cpp
// In C++
HandWidget->RemoveCardFromHand(CardIndex);
```

```
// In Blueprint
Event OnCardClicked(Card Index, Card Data)
  → Play Card (your game logic)
  → HandWidgetRef → Remove Card From Hand(Card Index)
```

### Updating Entire Hand
```cpp
// In C++
HandWidget->UpdateHandDisplay(PlayerState->Hand);
```

```
// In Blueprint
Get Player State → Get Hand
  → HandWidgetRef → Update Hand Display(Hand)
```

---

## Troubleshooting

### Issue: Cards Not Showing
**Solution:**
- Make sure `CardWidgetClass` is set to `WBP_TCG_Card`
- Verify `CardContainer` is bound (name must match exactly)
- Check that `Hand Array` has data

### Issue: Click Not Working
**Solution:**
- Ensure the Button in `WBP_TCG_Card` is set to `Visibility: Visible`
- Check that `Is Enabled` is checked on the button
- Verify click events are bound in the Graph

### Issue: Hover Effects Not Working
**Solution:**
- Make sure animations are created and named correctly
- Check that `On Mouse Enter/Leave` events are bound
- Verify the card widget has `Visibility: Visible` (not Hit Test Invisible)

### Issue: Compilation Errors
**Solution:**
- Check that `TCGTypes.h` exists and is included
- Verify all required modules are in `.Build.cs`: `UMG`, `Slate`, `SlateCore`
- Clean and rebuild the project

---

## Next Steps

Now that you have the hand widget working, you can:

1. ✅ **Implement Click-to-Play** (Step 2 in your roadmap)
   - Wire the `OnCardClicked` event to `PlayCharacter()` function
   - Check DON cost and resources
   - Spawn card actor on board

2. ✅ **Add Phase Transition Button** (Step 3)
   - Add "End Phase" button to `WBP_TCG_HUD`
   - Connect to `ATCGGameMode::AdvancePhase()`

3. ✅ **Connect to Real Game State** (Step 4)
   - Use `OnRep_Hand()` callbacks to update UI
   - Test with 2 players

---

## API Reference

### Public Functions

| Function | Description |
|----------|-------------|
| `UpdateHandDisplay(Cards)` | Replace entire hand with new cards |
| `AddCardToHand(Card)` | Add single card to hand |
| `RemoveCardFromHand(Index)` | Remove card at index |
| `ClearHand()` | Remove all cards |
| `SelectCard(Index)` | Select card by index |
| `DeselectAll()` | Deselect all cards |
| `GetSelectedCard()` | Get currently selected card data |
| `HasSelectedCard()` | Check if any card is selected |

### Events

| Event | Parameters | Description |
|-------|------------|-------------|
| `OnCardClicked` | CardIndex, CardData | Fired when player clicks a card |
| `OnCardHovered` | CardIndex, CardData | Fired when mouse enters card |
| `OnCardUnhovered` | CardIndex | Fired when mouse leaves card |
| `OnHandUpdated` | None | Fired when hand contents change |

---

## File Locations

```
/Source/OnePieceTCG_V2/
  ├── TCGHandWidget.h          # Hand widget header
  ├── TCGHandWidget.cpp        # Hand widget implementation
  ├── TCGTypes.h               # Card data structures
  └── TCGPlayerState.h         # Player state with Hand array

/Content/
  ├── WBP_TCG_Hand.uasset      # Hand widget Blueprint (YOU CREATE THIS)
  ├── WBP_TCG_Card.uasset      # Card widget (EXISTING)
  └── WBP_TCG_HUD.uasset       # Main HUD (EXISTING)
```

---

## Quick Start Checklist

- [ ] Compile C++ code
- [ ] Create WBP_TCG_Hand Blueprint
- [ ] Set parent class to TCGHandWidget
- [ ] Add Horizontal Box named "CardContainer"
- [ ] Set Card Widget Class to WBP_TCG_Card
- [ ] Add hover animations to WBP_TCG_Card
- [ ] Add click button to WBP_TCG_Card
- [ ] Test with sample data
- [ ] Connect to Player State
- [ ] Implement click-to-play functionality

---

## Support

If you encounter any issues:
1. Check the Output Log in Unreal Editor for errors
2. Verify all names match exactly (CardContainer, etc.)
3. Make sure WBP_TCG_Card has SetCardData function
4. Check that player state has Hand array with FCardData

Good luck with your One Piece TCG development! 🎴
