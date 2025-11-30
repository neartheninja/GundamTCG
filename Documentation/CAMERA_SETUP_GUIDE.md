// Camera & Viewport Setup Guide
# Gundam TCG - Camera & Viewport Setup Guide

Quick reference for implementing and configuring the camera system.

---

## File Structure

```
Source/GundamTCG/Camera/
├── GCGPlayerPawn.h          # Camera pawn header
└── GCGPlayerPawn.cpp        # Camera pawn implementation

Documentation/
├── CAMERA_AND_VIEWPORT_DESIGN.md    # Full design document
└── CAMERA_SETUP_GUIDE.md            # This file
```

---

## Quick Start: Adding Camera to Your Project

### Step 1: Create Camera Pawn in Unreal Editor

1. **Content Browser** → Right-click → Blueprint Class
2. Parent Class: `GCGPlayerPawn` (if compiled) OR `Pawn`
3. Name: `BP_GCG_CameraPawn`

### Step 2: Configure Game Mode

1. Open your Game Mode blueprint (`BP_GCGGameMode_1v1`)
2. Class Defaults → Default Pawn Class: `BP_GCG_CameraPawn`
3. Player Controller Class: `GCGPlayerController` (or create one)

### Step 3: Set Up Input Bindings

**Project Settings → Input**

#### Axis Mappings:

| Axis Name | Input | Scale | Condition |
|-----------|-------|-------|-----------|
| CameraOrbitYaw | Mouse X | 1.0 | Right Mouse Button held |
| CameraOrbitPitch | Mouse Y | -1.0 | Right Mouse Button held |
| CameraZoom | Mouse Wheel Axis | 1.0 | Always |
| CameraPanX | Mouse X | 1.0 | Middle Mouse Button held |
| CameraPanY | Mouse Y | -1.0 | Middle Mouse Button held |

#### Action Mappings:

| Action Name | Input Key |
|-------------|-----------|
| ResetCamera | R |
| FocusOpponent | F |
| FocusHand | H |

### Step 4: Configure Camera Pawn Instance

In `BP_GCG_CameraPawn` Blueprint:

**Components:**
- TableCenter (Scene Root)
  - CameraArm (Spring Arm)
    - MainCamera (Camera)

**Camera Arm Settings:**
- Target Arm Length: 250.0
- Socket Offset: (0, 0, 0)
- Enable Camera Lag: ✓
- Camera Lag Speed: 8.0
- Enable Camera Rotation Lag: ✓
- Camera Rotation Lag Speed: 10.0
- Do Collision Test: ✗ (no collision for card game)

**Main Camera Settings:**
- Field of View: 75.0
- Aspect Ratio: Auto
- Post Process: (Configure as needed)

**Pawn Settings:**
- Default Preset Name: "Strategic"
- Min Zoom Distance: 100.0
- Max Zoom Distance: 400.0
- Orbit Speed: 45.0
- Pan Speed: 100.0

---

## Camera Presets

Default presets are created automatically in `InitializeDefaultPresets()`:

### Strategic View (Default)
- **Use**: Main gameplay view
- **Position**: Centered on table
- **Rotation**: -35° pitch, 0° yaw
- **Arm Length**: 250 cm
- **FOV**: 75°

### Hand Focus
- **Use**: When examining hand
- **Position**: -100 cm toward player
- **Rotation**: -15° pitch
- **Arm Length**: 120 cm
- **FOV**: 80° (wider for full hand)

### Opponent Focus
- **Use**: Press 'F' to view opponent's field
- **Position**: +100 cm toward opponent
- **Rotation**: -45° pitch
- **Arm Length**: 180 cm
- **FOV**: 70°

### Combat View
- **Use**: During battle phase
- **Position**: Center, elevated 20 cm
- **Rotation**: -30° pitch
- **Arm Length**: 200 cm
- **FOV**: 65° (focused)

### Overview
- **Use**: Game setup, wide view
- **Position**: Center, elevated 50 cm
- **Rotation**: -50° pitch
- **Arm Length**: 350 cm
- **FOV**: 85° (very wide)

---

## Using Camera Presets in Blueprints

### Switch to a Preset

```cpp
// C++
CameraPawn->TransitionToPreset(FName("Strategic"), 0.5f);

// Blueprint
Transition To Preset
  Preset Name: "Strategic"
  Duration: 0.5 (seconds)
```

### Create Custom Preset

```cpp
// Blueprint: Create in Event Graph
Event BeginPlay
  -> Make FGCGCameraPreset
       Preset Name: "MyCustomView"
       Camera Location: (0, 0, 100)
       Camera Rotation: (-40, 0, 0)
       Field Of View: 70
       Arm Length: 200
       Transition Speed: 8
  -> Add (Map)
       Target: Camera Presets
       Key: "MyCustomView"
       Value: (struct from above)
```

---

## Manual Camera Control

Players can manually adjust camera:

**Orbit (Rotate)**:
- Hold Right Mouse + Move Mouse
- Yaw: -180° to 180°
- Pitch: -80° to -5° (clamped to prevent flipping)

**Zoom**:
- Mouse Wheel Up: Zoom In
- Mouse Wheel Down: Zoom Out
- Range: 100 cm to 400 cm

**Pan (Move)**:
- Hold Middle Mouse + Move Mouse
- Moves table center point
- Clamped to ±200 cm in X/Y

**Reset**:
- Press 'R': Returns to default "Strategic" preset
- Smooth transition over 0.5 seconds

---

## Connecting Camera to Game Events

### Example: Focus on Card Played

```cpp
// In your PlayerActionSubsystem or GameMode

void UGCGPlayerActionSubsystem::OnCardPlayed(const FGCGCardInstance& Card, EGCGCardZone ToZone)
{
    // Get camera pawn
    AGCGPlayerPawn* CameraPawn = Cast<AGCGPlayerPawn>(GetWorld()->GetFirstPlayerController()->GetPawn());

    if (CameraPawn && ToZone == EGCGCardZone::BattleArea)
    {
        // Get card's world position (you need to implement this)
        FVector CardWorldPosition = GetCardWorldPosition(Card);

        // Focus camera on the card
        CameraPawn->FocusOnLocation(CardWorldPosition, 0.3f);

        // After 1 second, return to strategic view
        FTimerHandle ReturnTimer;
        GetWorld()->GetTimerManager().SetTimer(ReturnTimer, [CameraPawn]()
        {
            CameraPawn->TransitionToPreset(FName("Strategic"), 0.5f);
        }, 1.0f, false);
    }
}
```

### Example: Switch View During Phases

```cpp
// In GameMode's OnPhaseChanged

void AGCGGameMode_1v1::OnPhaseChanged(EGCGTurnPhase NewPhase)
{
    AGCGPlayerPawn* CameraPawn = GetCameraPawn();

    switch (NewPhase)
    {
        case EGCGTurnPhase::Setup:
            CameraPawn->TransitionToPreset(FName("Overview"), 1.0f);
            break;

        case EGCGTurnPhase::Draw:
            CameraPawn->TransitionToPreset(FName("HandFocus"), 0.3f);
            break;

        case EGCGTurnPhase::Battle:
            CameraPawn->TransitionToPreset(FName("Combat"), 0.5f);
            break;

        default:
            CameraPawn->TransitionToPreset(FName("Strategic"), 0.4f);
            break;
    }
}
```

---

## Testing Camera in Editor

### Method 1: PIE (Play In Editor)

1. Press Play
2. Camera should auto-possess and use presets
3. Test controls:
   - Right-click + drag to orbit
   - Mouse wheel to zoom
   - Press R to reset
   - Press F/H to test focus hotkeys

### Method 2: Simulate

1. Click Simulate
2. Select camera pawn in World Outliner
3. Details panel shows all camera properties
4. Modify presets in real-time
5. Test transitions with buttons

### Debug Commands (Console)

```
// Show camera debug info
showdebug camera

// Teleport camera
camera setpawn BP_GCG_CameraPawn

// Slow motion (test transitions)
slomo 0.3

// Normal speed
slomo 1.0
```

---

## Common Issues & Solutions

### Issue: Camera feels "floaty" or laggy

**Solution**:
- Increase `CameraLagSpeed` (try 12-15)
- Decrease `TargetArmLength` for faster response
- Disable camera lag if you want instant response

### Issue: Can't see cards clearly from strategic view

**Solution**:
- Decrease `ArmLength` in Strategic preset (try 200 cm)
- Increase FOV slightly (try 80°)
- Adjust pitch angle to be less steep (try -30° instead of -35°)

### Issue: Hand cards are cut off at bottom

**Solution**:
- Increase FOV in HandFocus preset (try 85-90°)
- Adjust card layout to be higher on screen
- Move camera back (increase ArmLength)

### Issue: Opponent's cards too small

**Solution**:
- This is intentional for perspective
- Players can press 'F' to focus opponent field
- Hovering cards should enlarge them (implement in UI)

### Issue: Camera flips upside down

**Solution**:
- Pitch is not clamped correctly
- Check `ClampCameraValues()` is being called
- Verify pitch range: -80° to -5°

---

## Performance Optimization

### Reduce Draw Calls

```cpp
// In Camera Component
MainCamera->bConstrainAspectRatio = false;  // Auto-adjust
MainCamera->PostProcessBlendWeight = 0.0f;  // Disable if not using PP
```

### Cull Distant Objects

```cpp
// Set max draw distance
MainCamera->OrthoFarClipPlane = 5000.0f;

// Enable distance culling per actor
StaticMeshComponent->bAllowCullDistanceVolume = true;
StaticMeshComponent->LDMaxDrawDistance = 3000.0f;
```

### LOD for Cards

Implement multiple detail levels for card meshes:
- LOD 0: High detail (player's hand, inspected cards)
- LOD 1: Medium detail (player's battle area)
- LOD 2: Low detail (opponent's field)
- LOD 3: Very low (deck/graveyard piles)

---

## Blueprint vs C++ Trade-offs

### C++ Advantages:
✅ Precise control
✅ Better performance
✅ Type safety
✅ Easier to debug

### Blueprint Advantages:
✅ Visual scripting
✅ Faster iteration
✅ Designer-friendly
✅ No recompile needed

**Recommendation**: Use C++ for core camera pawn, Blueprint for preset customization and game event responses.

---

## Next Steps

1. **Implement Camera Pawn**: Compile `GCGPlayerPawn` classes
2. **Create Blueprint**: `BP_GCG_CameraPawn` based on C++ class
3. **Set Up Inputs**: Configure in Project Settings
4. **Test Presets**: Play in editor, test all views
5. **Integrate Events**: Connect to game phases/actions
6. **Add UI**: Card hover zoom, zone highlights
7. **Polish**: Animations, VFX, transitions
8. **Optimize**: LODs, culling, draw distance

---

## Additional Resources

- **Full Design Doc**: `CAMERA_AND_VIEWPORT_DESIGN.md`
- **UE5 Camera Docs**: https://docs.unrealengine.com/5.0/en-US/camera-components-in-unreal-engine/
- **Spring Arm Docs**: https://docs.unrealengine.com/5.0/en-US/API/Runtime/Engine/GameFramework/USpringArmComponent/

---

## Summary

The camera system provides:
- **Immersive first-person hand view** (like holding physical cards)
- **Strategic angled play area** (see entire board clearly)
- **Smooth transitions** between focus areas
- **Manual control** for player preference
- **Game event integration** (auto-focus on actions)

**Default Controls**:
- Right Mouse + Drag: Orbit
- Mouse Wheel: Zoom
- Middle Mouse + Drag: Pan
- R: Reset
- F: Focus Opponent
- H: Focus Hand

Start with the "Strategic" preset and customize from there!
