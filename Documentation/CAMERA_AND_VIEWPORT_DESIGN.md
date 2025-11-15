# Gundam TCG - Camera & Viewport Design

## Overview

This document outlines the ideal camera perspective and viewport design for the Gundam Trading Card Game digital implementation, focusing on immersion, usability, and quality-of-life features.

---

## Camera Perspective Philosophy

### Design Goals
1. **Immersive First-Person Hand View** - Players feel like they're physically holding their cards
2. **Strategic Angled Play Area** - Clear view of all zones with depth perception
3. **Smooth Transitions** - Seamless camera movement between focus areas
4. **Accessibility** - Adjustable settings for different player preferences

---

## Primary Camera System: "Tabletop Perspective"

### Base Camera Setup

**Position**: Seated player perspective at a physical table
- **Height**: 150-180cm above table surface (eye level when seated)
- **Angle**: 15-25° downward tilt (natural head angle when looking at table)
- **Distance**: 120-150cm from player's position

**Camera Type**: Spring Arm Camera with smooth interpolation
```cpp
// Recommended UE5 setup
UCameraComponent* MainCamera;
USpringArmComponent* CameraArm;

// Base configuration
CameraArm->TargetArmLength = 150.0f;      // Distance from player position
CameraArm->SetRelativeRotation(FRotator(-20.0f, 0.0f, 0.0f)); // Downward tilt
CameraArm->bEnableCameraLag = true;        // Smooth movement
CameraArm->CameraLagSpeed = 8.0f;          // Responsiveness
CameraArm->bEnableCameraRotationLag = true;
CameraArm->CameraRotationLagSpeed = 10.0f;
```

---

## Zone Layout & Camera Focus Areas

### 1. Hand Zone (Primary Focus - First Person Style)

**Visual Design:**
- Cards arc across bottom 40% of screen
- Slightly angled toward player (10-15° tilt)
- Cards fan out in a natural arc (like holding physical cards)
- Selected card lifts forward and enlarges (1.2x scale)

**Camera Behavior:**
```
Default State:
- Position: Centered on player
- Tilt: -10° (slight downward, mostly looking straight)
- FOV: 75° (wider to see full hand arc)
- Hand cards at bottom, play area visible above

Hover State:
- Focused card moves forward 20cm
- Card rotates to face camera directly (0° tilt)
- Slight blur on other cards (depth of field)
- Card details clearly readable
```

**Implementation Notes:**
- Use Widget Component attached to 3D card meshes
- Raycast from camera for hover detection
- Smooth lerp transitions (0.15s duration)

### 2. Play Area (Angled Strategic View)

**Layout:**
```
┌─────────────────────────────────────────┐
│         Opponent Deck/Hand/Shield       │ Top (Far)
├─────────────────────────────────────────┤
│      Opponent Battle Area (6 slots)     │
├─────────────────────────────────────────┤
│           Resource Areas (Both)         │ Middle
├─────────────────────────────────────────┤
│      Player Battle Area (6 slots)       │
├─────────────────────────────────────────┤
│        Player Base (Large, Center)      │ Bottom (Near)
└─────────────────────────────────────────┘
```

**Viewing Angle:**
- **Tilt**: 35-45° downward (angled, NOT top-down)
- **Height**: 250cm above table
- **Distance**: Cards at top (opponent) are ~200cm from camera
- **Cards at bottom (player)**: ~100cm from camera
- **Perspective**: Natural depth - far cards appear smaller

**Why This Angle Works:**
✅ Can see all cards clearly
✅ Natural depth perception (units closer = larger)
✅ Opponent's area feels "across the table"
✅ Easy to distinguish zones
✅ Not disorienting (not top-down 90°)

### 3. Card Detail View (Inspection Mode)

**Trigger**: Right-click card, or press 'Z' to zoom
**Effect**:
- Card fills 60% of screen
- Full resolution art visible
- Card text clearly readable
- Background blurred (depth of field)
- Semi-transparent to see board state
- Press ESC or click away to exit

**Camera Position:**
```
- Distance: 40cm from card
- Card rotates to face camera (perpendicular)
- Subtle rotation animation on enter
- Soft lighting from above
```

---

## Camera Transition States

### 1. Setup Phase
```
Camera: Wide view showing both players' deck areas
Tilt: -25°
Focus: Center of table
Animation: Slow zoom in as game starts
```

### 2. Main Gameplay (Default)
```
Camera: Angled strategic view
Player can see:
  - Full hand at bottom (always visible)
  - Both battle areas
  - Resource areas
  - Opponent's field (partially)
```

### 3. Combat Phase
```
Camera: Dynamic focus on attacking units
Behavior:
  - Smooth pan to attacking unit
  - Follow attack animation
  - Show damage numbers
  - Return to strategic view
```

### 4. Stack Resolution (Card Effects)
```
Camera: Focus on effect source
UI: Effect stack overlay on right side
Lighting: Spotlight on active effect card
```

---

## Quality of Life (QOL) Features

### Essential QOL Features

#### 1. **Card Readability**
- [ ] **Auto-Zoom on Hover**: Any card enlarges when mouse hovers (0.2s delay)
- [ ] **Quick Inspect**: Right-click any card for full-screen detail view
- [ ] **Text Size Options**: Small/Medium/Large card text
- [ ] **High-Res Art Toggle**: Option to load higher resolution card art

#### 2. **Zone Highlighting**
- [ ] **Valid Drop Zones**: Green outline when dragging card
- [ ] **Invalid Zones**: Red outline with prohibition icon
- [ ] **Zone Labels**: Toggleable zone name overlays
- [ ] **Color-Coded Zones**: Subtle color tinting (blue = friendly, red = opponent)

#### 3. **Card Interaction**
- [ ] **Drag & Drop**: Natural drag from hand to play
- [ ] **Click to Play**: Alternative - click card, click destination
- [ ] **Undo Last Action**: Ctrl+Z during action step (before confirming)
- [ ] **Auto-Arrange**: Cards in zones auto-organize neatly
- [ ] **Manual Repositioning**: Drag cards within same zone to reorder

#### 4. **Information Display**
- [ ] **Stat Overlays**: AP/HP/Cost always visible on cards
- [ ] **Damage Indicators**: Red damage numbers on damaged units
- [ ] **Status Icons**: Visual indicators for rested/active state
- [ ] **Lv Tracker**: Player Lv prominently displayed
- [ ] **Resource Counter**: Active resources / Total resources

#### 5. **Combat QOL**
- [ ] **Attack Arrows**: Draw arrow from attacker to target
- [ ] **Damage Preview**: Show potential damage before confirming
- [ ] **Attack Animation Skip**: Hold SPACE to skip animations
- [ ] **Combat Log**: Scrollable history of all combat actions
- [ ] **Shield Stack Indicator**: Clear count of remaining shields

#### 6. **Turn Flow**
- [ ] **Phase Indicators**: Large phase name display on transitions
- [ ] **Priority Indicator**: Glowing border around active player
- [ ] **Action Confirmations**: "Are you sure?" for end turn
- [ ] **Timer Display**: Countdown timer for timed matches
- [ ] **Quick Concede**: ESC > Concede (with confirmation)

#### 7. **Search & Filter**
- [ ] **Deck Search UI**: When effects search deck
- [ ] **Filter by Type**: Quick filter buttons (Units/Commands/etc)
- [ ] **Filter by Color**: Color filter buttons
- [ ] **Sort Options**: Cost/AP/HP/Name sorting
- [ ] **Search Box**: Type to filter by name

#### 8. **Accessibility**
- [ ] **Colorblind Modes**: Multiple palette options
- [ ] **Text-to-Speech**: Read card text aloud
- [ ] **High Contrast Mode**: Increased visual distinction
- [ ] **Keybind Customization**: Rebindable hotkeys
- [ ] **Mouse Sensitivity**: Adjustable camera rotation speed

#### 9. **Camera Controls**
- [ ] **Orbit Camera**: Hold right mouse + drag to rotate view
- [ ] **Zoom**: Mouse wheel to zoom in/out (limits: 100cm - 400cm)
- [ ] **Pan**: Middle mouse + drag to pan camera
- [ ] **Reset View**: Press 'R' to reset to default angle
- [ ] **Focus Opponent**: Press 'F' to center camera on opponent's field
- [ ] **Focus Hand**: Press 'H' to focus on your hand

#### 10. **Multiplayer QOL**
- [ ] **Player Avatars**: Profile pictures above play areas
- [ ] **Emotes**: Quick communication (Good Game, Thinking, etc.)
- [ ] **Chat**: Text chat (with mute option)
- [ ] **Spectator Mode**: Friends can watch matches
- [ ] **Replay System**: Save and watch past games

---

## Advanced Camera Features

### Dynamic Focus System

**Auto-Focus Triggers:**
1. **Card Drawn**: Brief zoom on drawn card (0.5s)
2. **Card Played**: Camera follows card from hand to play zone
3. **Unit Attacks**: Camera pans to show attacker → target
4. **Card Destroyed**: Brief focus before moving to graveyard
5. **Effect Activates**: Spotlight on effect source

**User Control:**
- Toggle auto-focus in settings
- Manual override with camera controls
- "Focus Last Event" hotkey (default: 'E')

### Cinematic Moments

**Game Start**:
```
1. Fade in from black
2. Camera high above table (bird's eye)
3. Zoom in to strategic angle
4. Shuffle deck animations
5. Draw opening hand
6. Cards fan out
7. "Mulligan?" prompt
```

**Battle Phase**:
```
1. Phase transition effect
2. Attacking units glow
3. Click unit to attack
4. Draw attack arrow to target
5. Unit "charges forward" animation
6. Impact VFX
7. Damage numbers fly up
8. Return to strategic view
```

**Game End**:
```
1. Final damage resolves
2. Camera focuses on losing player's Base
3. Destruction effect
4. Camera pulls back
5. Victory/Defeat screen
6. Match statistics
```

---

## Implementation Roadmap

### Phase 1: Core Camera (MVP)
- [ ] Static strategic angle camera
- [ ] Basic hand view at bottom
- [ ] Click to play cards
- [ ] No animations (instant zone changes)
- [ ] Simple UI overlays

### Phase 2: Polish & Transitions
- [ ] Smooth camera transitions
- [ ] Card movement animations
- [ ] Hover to enlarge
- [ ] Drag and drop
- [ ] Zone highlighting

### Phase 3: QOL Features
- [ ] All essential QOL features (see list above)
- [ ] Camera orbit/zoom/pan
- [ ] Card detail view
- [ ] Combat animations
- [ ] Effect stack visualization

### Phase 4: Advanced Features
- [ ] Dynamic auto-focus
- [ ] Cinematic moments
- [ ] Replay system
- [ ] Spectator mode
- [ ] Full accessibility suite

---

## Technical Specifications

### Recommended UE5 Components

```cpp
// GCGPlayerPawn.h - Camera container pawn
class AGCGPlayerPawn : public APawn
{
    UPROPERTY(EditAnywhere)
    USpringArmComponent* CameraArm;

    UPROPERTY(EditAnywhere)
    UCameraComponent* MainCamera;

    UPROPERTY(EditAnywhere)
    UCameraComponent* HandCamera;  // Secondary camera for hand view

    UPROPERTY(EditAnywhere)
    USceneComponent* TableCenter;  // Pivot point

    // Camera states
    UPROPERTY(EditAnywhere)
    TArray<FGCGCameraPreset> CameraPresets;

    // Transition
    void TransitionToPreset(FName PresetName, float Duration = 0.5f);
    void OrbitCamera(float DeltaYaw, float DeltaPitch);
    void ZoomCamera(float ZoomDelta);
};

// GCGCameraPreset.h - Saved camera positions
USTRUCT(BlueprintType)
struct FGCGCameraPreset
{
    UPROPERTY(EditAnywhere)
    FName PresetName;  // "Strategic", "HandFocus", "Combat", etc.

    UPROPERTY(EditAnywhere)
    FVector CameraLocation;

    UPROPERTY(EditAnywhere)
    FRotator CameraRotation;

    UPROPERTY(EditAnywhere)
    float FieldOfView;

    UPROPERTY(EditAnywhere)
    float TransitionSpeed;
};
```

### Performance Considerations

**Optimization Tips:**
1. **LOD for Cards**: Lower detail for distant/opponent cards
2. **Occlusion Culling**: Don't render hidden card backs
3. **Texture Streaming**: Load high-res art only when inspecting
4. **Particle Budgets**: Limit simultaneous VFX
5. **Batch Rendering**: Instance cards with same materials

---

## Reference Implementations

### Games with Great Camera Systems

1. **Magic: The Gathering Arena**
   - Angled strategic view ✓
   - Clean zone separation ✓
   - Smooth card transitions ✓

2. **Hearthstone**
   - First-person hand view ✓
   - Cinematic effects ✓
   - Clear board state ✓

3. **Yu-Gi-Oh! Master Duel**
   - Detailed card inspection ✓
   - Fast gameplay pace ✓
   - Chain resolution visualization ✓

4. **Legends of Runeterra**
   - Beautiful animations ✓
   - Excellent QOL features ✓
   - Attack flow ✓

### What to Borrow from Each

**From MTGA**:
- Zone layout (clean, organized)
- Auto-pass priority options
- Detailed combat log

**From Hearthstone**:
- Hand card arc presentation
- Satisfying card slam animations
- Clear visual feedback

**From Master Duel**:
- Chain resolution step-by-step
- Card effect highlights
- Fast auto-play options

**From LoR**:
- Attack token system visualization
- Beautiful card art focus
- Smooth spell stack

---

## Mobile Considerations (Future)

If targeting mobile/tablet:

**Camera Adjustments:**
- Simplified angle (less dramatic tilt)
- Larger UI elements
- Touch-optimized controls
- Vertical orientation option

**Simplified QOL:**
- Tap to enlarge (no hover)
- Gesture controls (pinch to zoom)
- Smaller max hand size (5-6 cards visible)
- Auto-play suggestions

---

## Summary: Ideal Setup

**Main Camera**:
- Angled strategic view (35-40° tilt)
- Height: 250cm
- Distance: 150cm from player position
- FOV: 70-75°

**Hand Display**:
- Bottom 40% of screen
- Arc formation (natural card holding)
- Always visible
- Hover to enlarge

**Transitions**:
- Smooth interpolation (0.3-0.5s)
- Auto-focus on important events
- Manual camera control available
- Reset to default always possible

**QOL Priority**:
1. Hover to zoom
2. Zone highlighting
3. Drag & drop
4. Combat preview
5. Undo action
6. Card detail view
7. Combat log
8. Phase indicators
9. Accessibility options
10. Emotes/communication

This creates an **immersive, strategic, and highly usable** digital card game experience that feels both like playing at a physical table and takes full advantage of digital capabilities.
