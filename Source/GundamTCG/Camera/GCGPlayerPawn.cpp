// GCGPlayerPawn.cpp - Camera Container Pawn Implementation
// Unreal Engine 5.6 - Gundam TCG Implementation

#include "GCGPlayerPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/KismetMathLibrary.h"

AGCGPlayerPawn::AGCGPlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create components
	TableCenter = CreateDefaultSubobject<USceneComponent>(TEXT("TableCenter"));
	RootComponent = TableCenter;

	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(TableCenter);
	CameraArm->TargetArmLength = 150.0f;
	CameraArm->bEnableCameraLag = true;
	CameraArm->CameraLagSpeed = 8.0f;
	CameraArm->bEnableCameraRotationLag = true;
	CameraArm->CameraRotationLagSpeed = 10.0f;
	CameraArm->bDoCollisionTest = false; // No collision for card game camera

	MainCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("MainCamera"));
	MainCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
	MainCamera->SetFieldOfView(75.0f);

	// Default settings
	DefaultPresetName = FName("Strategic");
	CurrentPresetName = NAME_None;
	bIsTransitioning = false;
	TransitionAlpha = 0.0f;
	TransitionSpeed = 8.0f;

	MinZoomDistance = 100.0f;
	MaxZoomDistance = 400.0f;
	OrbitSpeed = 45.0f;
	PanSpeed = 100.0f;
	bEnableCameraLag = true;
	CameraLagSpeed = 8.0f;

	CurrentOrbitYaw = 0.0f;
	CurrentOrbitPitch = -35.0f;
	CurrentZoom = 150.0f;
	CurrentPanOffset = FVector::ZeroVector;
}

void AGCGPlayerPawn::BeginPlay()
{
	Super::BeginPlay();

	// Initialize default camera presets
	InitializeDefaultPresets();

	// Snap to default preset
	SnapToPreset(DefaultPresetName);
}

void AGCGPlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update camera transition if active
	if (bIsTransitioning)
	{
		UpdateCameraTransition(DeltaTime);
	}
}

void AGCGPlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Camera controls
	// Note: These need to be bound in project input settings
	// This is a reference for what inputs should be created

	PlayerInputComponent->BindAxis("CameraOrbitYaw", this, &AGCGPlayerPawn::Input_OrbitCamera);
	PlayerInputComponent->BindAxis("CameraOrbitPitch", this, &AGCGPlayerPawn::Input_OrbitCamera);
	PlayerInputComponent->BindAxis("CameraZoom", this, &AGCGPlayerPawn::Input_ZoomCamera);
	PlayerInputComponent->BindAxis("CameraPanX", this, &AGCGPlayerPawn::Input_PanCamera);
	PlayerInputComponent->BindAxis("CameraPanY", this, &AGCGPlayerPawn::Input_PanCamera);

	PlayerInputComponent->BindAction("ResetCamera", IE_Pressed, this, &AGCGPlayerPawn::Input_ResetCamera);
	PlayerInputComponent->BindAction("FocusOpponent", IE_Pressed, this, &AGCGPlayerPawn::Input_FocusOpponent);
	PlayerInputComponent->BindAction("FocusHand", IE_Pressed, this, &AGCGPlayerPawn::Input_FocusHand);
}

// ===== CAMERA CONTROL FUNCTIONS =====

void AGCGPlayerPawn::TransitionToPreset(FName PresetName, float Duration)
{
	if (!CameraPresets.Contains(PresetName))
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCGPlayerPawn::TransitionToPreset - Preset '%s' not found"), *PresetName.ToString());
		return;
	}

	// Get target preset
	TargetPreset = CameraPresets[PresetName];

	// Store starting state
	StartingPreset.CameraLocation = TableCenter->GetRelativeLocation();
	StartingPreset.CameraRotation = CameraArm->GetRelativeRotation();
	StartingPreset.FieldOfView = MainCamera->FieldOfView;
	StartingPreset.ArmLength = CameraArm->TargetArmLength;

	// Set transition parameters
	if (Duration == 0.0f)
	{
		// Instant snap
		SnapToPreset(PresetName);
		return;
	}
	else if (Duration > 0.0f)
	{
		// Override speed based on duration
		TransitionSpeed = 1.0f / Duration;
	}
	else
	{
		// Use preset's speed
		TransitionSpeed = TargetPreset.TransitionSpeed;
	}

	// Start transition
	FName PreviousPreset = CurrentPresetName;
	CurrentPresetName = PresetName;
	bIsTransitioning = true;
	TransitionAlpha = 0.0f;

	// Fire event
	OnCameraTransitionStarted(PreviousPreset, PresetName);
}

void AGCGPlayerPawn::SnapToPreset(FName PresetName)
{
	if (!CameraPresets.Contains(PresetName))
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCGPlayerPawn::SnapToPreset - Preset '%s' not found"), *PresetName.ToString());
		return;
	}

	FGCGCameraPreset& Preset = CameraPresets[PresetName];
	ApplyCameraPreset(Preset);

	CurrentPresetName = PresetName;
	bIsTransitioning = false;
	TransitionAlpha = 1.0f;

	OnCameraTransitionCompleted(PresetName);
}

void AGCGPlayerPawn::ResetCamera()
{
	TransitionToPreset(DefaultPresetName, 0.5f);
}

void AGCGPlayerPawn::OrbitCamera(float DeltaYaw, float DeltaPitch)
{
	// Update orbit angles
	CurrentOrbitYaw += DeltaYaw;
	CurrentOrbitPitch += DeltaPitch;

	// Clamp pitch to prevent camera flipping
	CurrentOrbitPitch = FMath::Clamp(CurrentOrbitPitch, -80.0f, -5.0f);

	// Apply rotation
	FRotator NewRotation(CurrentOrbitPitch, CurrentOrbitYaw, 0.0f);
	CameraArm->SetRelativeRotation(NewRotation);
}

void AGCGPlayerPawn::ZoomCamera(float ZoomDelta)
{
	// Update zoom level
	CurrentZoom = FMath::Clamp(CurrentZoom + ZoomDelta, MinZoomDistance, MaxZoomDistance);

	// Apply zoom
	CameraArm->TargetArmLength = CurrentZoom;
}

void AGCGPlayerPawn::PanCamera(float DeltaX, float DeltaY)
{
	// Calculate pan offset in camera space
	FVector RightVector = CameraArm->GetRightVector();
	FVector ForwardVector = CameraArm->GetForwardVector();
	ForwardVector.Z = 0.0f; // Keep pan on horizontal plane
	ForwardVector.Normalize();

	CurrentPanOffset += RightVector * DeltaX * PanSpeed;
	CurrentPanOffset += ForwardVector * DeltaY * PanSpeed;

	// Clamp pan offset to reasonable range
	CurrentPanOffset.X = FMath::Clamp(CurrentPanOffset.X, -200.0f, 200.0f);
	CurrentPanOffset.Y = FMath::Clamp(CurrentPanOffset.Y, -200.0f, 200.0f);

	// Apply pan
	TableCenter->SetRelativeLocation(CurrentPanOffset);
}

void AGCGPlayerPawn::FocusOnLocation(FVector TargetLocation, float Duration)
{
	// Create custom preset focused on target location
	FGCGCameraPreset FocusPreset;
	FocusPreset.PresetName = FName("CustomFocus");
	FocusPreset.CameraLocation = TargetLocation;

	// Calculate rotation to look at target
	FVector CameraWorldLocation = CameraArm->GetComponentLocation();
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(CameraWorldLocation, TargetLocation);
	FocusPreset.CameraRotation = LookAtRotation;

	FocusPreset.ArmLength = 100.0f; // Close-up
	FocusPreset.FieldOfView = 60.0f; // Narrower FOV for focus
	FocusPreset.TransitionSpeed = 1.0f / Duration;

	// Add or update custom focus preset
	CameraPresets.Add(FName("CustomFocus"), FocusPreset);

	// Transition to it
	TransitionToPreset(FName("CustomFocus"), Duration);
}

void AGCGPlayerPawn::FocusOpponentField()
{
	TransitionToPreset(FName("OpponentFocus"), 0.5f);
}

void AGCGPlayerPawn::FocusPlayerHand()
{
	TransitionToPreset(FName("HandFocus"), 0.5f);
}

// ===== INPUT HANDLERS =====

void AGCGPlayerPawn::Input_OrbitCamera(float AxisValueYaw, float AxisValuePitch)
{
	if (FMath::Abs(AxisValueYaw) > 0.01f || FMath::Abs(AxisValuePitch) > 0.01f)
	{
		float DeltaYaw = AxisValueYaw * OrbitSpeed * GetWorld()->GetDeltaSeconds();
		float DeltaPitch = AxisValuePitch * OrbitSpeed * GetWorld()->GetDeltaSeconds();
		OrbitCamera(DeltaYaw, DeltaPitch);
	}
}

void AGCGPlayerPawn::Input_ZoomCamera(float AxisValue)
{
	if (FMath::Abs(AxisValue) > 0.01f)
	{
		ZoomCamera(AxisValue * -10.0f); // Negative for natural mouse wheel direction
	}
}

void AGCGPlayerPawn::Input_PanCamera(float AxisValueX, float AxisValueY)
{
	if (FMath::Abs(AxisValueX) > 0.01f || FMath::Abs(AxisValueY) > 0.01f)
	{
		float DeltaX = AxisValueX * GetWorld()->GetDeltaSeconds();
		float DeltaY = AxisValueY * GetWorld()->GetDeltaSeconds();
		PanCamera(DeltaX, DeltaY);
	}
}

void AGCGPlayerPawn::Input_ResetCamera()
{
	ResetCamera();
}

void AGCGPlayerPawn::Input_FocusOpponent()
{
	FocusOpponentField();
}

void AGCGPlayerPawn::Input_FocusHand()
{
	FocusPlayerHand();
}

// ===== HELPER FUNCTIONS =====

void AGCGPlayerPawn::InitializeDefaultPresets()
{
	// Strategic View (Default)
	FGCGCameraPreset StrategicPreset;
	StrategicPreset.PresetName = FName("Strategic");
	StrategicPreset.CameraLocation = FVector(0.0f, 0.0f, 0.0f); // Centered on table
	StrategicPreset.CameraRotation = FRotator(-35.0f, 0.0f, 0.0f); // Angled view
	StrategicPreset.ArmLength = 250.0f;
	StrategicPreset.FieldOfView = 75.0f;
	StrategicPreset.TransitionSpeed = 8.0f;
	CameraPresets.Add(StrategicPreset.PresetName, StrategicPreset);

	// Hand Focus
	FGCGCameraPreset HandFocusPreset;
	HandFocusPreset.PresetName = FName("HandFocus");
	HandFocusPreset.CameraLocation = FVector(-100.0f, 0.0f, 0.0f); // Closer to player
	HandFocusPreset.CameraRotation = FRotator(-15.0f, 0.0f, 0.0f); // Slight downward angle
	HandFocusPreset.ArmLength = 120.0f;
	HandFocusPreset.FieldOfView = 80.0f; // Wider to see full hand arc
	HandFocusPreset.TransitionSpeed = 10.0f;
	CameraPresets.Add(HandFocusPreset.PresetName, HandFocusPreset);

	// Opponent Focus
	FGCGCameraPreset OpponentFocusPreset;
	OpponentFocusPreset.PresetName = FName("OpponentFocus");
	OpponentFocusPreset.CameraLocation = FVector(100.0f, 0.0f, 0.0f); // Closer to opponent
	OpponentFocusPreset.CameraRotation = FRotator(-45.0f, 0.0f, 0.0f); // Steeper angle
	OpponentFocusPreset.ArmLength = 180.0f;
	OpponentFocusPreset.FieldOfView = 70.0f;
	OpponentFocusPreset.TransitionSpeed = 8.0f;
	CameraPresets.Add(OpponentFocusPreset.PresetName, OpponentFocusPreset);

	// Combat View
	FGCGCameraPreset CombatPreset;
	CombatPreset.PresetName = FName("Combat");
	CombatPreset.CameraLocation = FVector(0.0f, 0.0f, 20.0f); // Slightly elevated
	CombatPreset.CameraRotation = FRotator(-30.0f, 0.0f, 0.0f);
	CombatPreset.ArmLength = 200.0f;
	CombatPreset.FieldOfView = 65.0f; // Narrower for focus
	CombatPreset.TransitionSpeed = 12.0f; // Faster for combat
	CameraPresets.Add(CombatPreset.PresetName, CombatPreset);

	// Wide Overview (Setup phase)
	FGCGCameraPreset OverviewPreset;
	OverviewPreset.PresetName = FName("Overview");
	OverviewPreset.CameraLocation = FVector(0.0f, 0.0f, 50.0f);
	OverviewPreset.CameraRotation = FRotator(-50.0f, 0.0f, 0.0f);
	OverviewPreset.ArmLength = 350.0f;
	OverviewPreset.FieldOfView = 85.0f;
	OverviewPreset.TransitionSpeed = 6.0f;
	CameraPresets.Add(OverviewPreset.PresetName, OverviewPreset);

	UE_LOG(LogTemp, Log, TEXT("AGCGPlayerPawn::InitializeDefaultPresets - Initialized %d camera presets"), CameraPresets.Num());
}

void AGCGPlayerPawn::UpdateCameraTransition(float DeltaTime)
{
	if (!bIsTransitioning)
	{
		return;
	}

	// Update transition alpha
	TransitionAlpha += DeltaTime * TransitionSpeed;

	if (TransitionAlpha >= 1.0f)
	{
		// Transition complete
		TransitionAlpha = 1.0f;
		bIsTransitioning = false;
		ApplyCameraPreset(TargetPreset);
		OnCameraTransitionCompleted(CurrentPresetName);
		return;
	}

	// Smooth interpolation (ease in-out)
	float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, TransitionAlpha);

	// Interpolate camera properties
	FVector NewLocation = FMath::Lerp(StartingPreset.CameraLocation, TargetPreset.CameraLocation, SmoothAlpha);
	FRotator NewRotation = FMath::Lerp(StartingPreset.CameraRotation, TargetPreset.CameraRotation, SmoothAlpha);
	float NewFOV = FMath::Lerp(StartingPreset.FieldOfView, TargetPreset.FieldOfView, SmoothAlpha);
	float NewArmLength = FMath::Lerp(StartingPreset.ArmLength, TargetPreset.ArmLength, SmoothAlpha);

	// Apply interpolated values
	TableCenter->SetRelativeLocation(NewLocation);
	CameraArm->SetRelativeRotation(NewRotation);
	MainCamera->SetFieldOfView(NewFOV);
	CameraArm->TargetArmLength = NewArmLength;

	// Update current values for manual controls
	CurrentOrbitYaw = NewRotation.Yaw;
	CurrentOrbitPitch = NewRotation.Pitch;
	CurrentZoom = NewArmLength;
}

void AGCGPlayerPawn::ApplyCameraPreset(const FGCGCameraPreset& Preset)
{
	TableCenter->SetRelativeLocation(Preset.CameraLocation);
	CameraArm->SetRelativeRotation(Preset.CameraRotation);
	MainCamera->SetFieldOfView(Preset.FieldOfView);
	CameraArm->TargetArmLength = Preset.ArmLength;

	// Update current values
	CurrentOrbitYaw = Preset.CameraRotation.Yaw;
	CurrentOrbitPitch = Preset.CameraRotation.Pitch;
	CurrentZoom = Preset.ArmLength;

	ClampCameraValues();
}

void AGCGPlayerPawn::ClampCameraValues()
{
	// Clamp zoom
	CurrentZoom = FMath::Clamp(CurrentZoom, MinZoomDistance, MaxZoomDistance);
	CameraArm->TargetArmLength = CurrentZoom;

	// Clamp pitch
	CurrentOrbitPitch = FMath::Clamp(CurrentOrbitPitch, -80.0f, -5.0f);

	// Apply clamped rotation
	FRotator ClampedRotation(CurrentOrbitPitch, CurrentOrbitYaw, 0.0f);
	CameraArm->SetRelativeRotation(ClampedRotation);
}
