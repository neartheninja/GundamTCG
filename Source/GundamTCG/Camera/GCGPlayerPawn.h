// GCGPlayerPawn.h - Camera Container Pawn for Gundam TCG
// Unreal Engine 5.6 - Gundam TCG Implementation
// Handles camera positioning, transitions, and player viewport control

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GCGPlayerPawn.generated.h"

// Forward declarations
class USpringArmComponent;
class UCameraComponent;
class USceneComponent;

/**
 * Camera Preset - Predefined camera positions for different game states
 */
USTRUCT(BlueprintType)
struct FGCGCameraPreset
{
	GENERATED_BODY()

	/** Preset identifier (e.g., "Strategic", "HandFocus", "Combat") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FName PresetName;

	/** Camera position relative to table center */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector CameraLocation;

	/** Camera rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FRotator CameraRotation;

	/** Field of view (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float FieldOfView;

	/** Spring arm length (distance from pivot) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float ArmLength;

	/** Transition speed when moving to this preset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float TransitionSpeed;

	FGCGCameraPreset()
		: PresetName(NAME_None)
		, CameraLocation(FVector::ZeroVector)
		, CameraRotation(FRotator::ZeroRotator)
		, FieldOfView(75.0f)
		, ArmLength(150.0f)
		, TransitionSpeed(8.0f)
	{}
};

/**
 * Player Pawn - Camera Container
 *
 * PURPOSE:
 * This pawn serves as the player's "eyes" in the Gundam TCG game.
 * It doesn't represent a physical character, but rather the player's perspective
 * sitting at a table playing cards.
 *
 * CAMERA SYSTEM:
 * - Strategic View: Angled view of entire play area (default)
 * - Hand Focus: Close-up of player's hand
 * - Card Detail: Zoomed inspection of individual cards
 * - Combat Focus: Dynamic focus during battle phase
 *
 * CONTROLS:
 * - Right Mouse + Drag: Orbit camera
 * - Mouse Wheel: Zoom in/out
 * - Middle Mouse + Drag: Pan camera
 * - R: Reset to default view
 * - F: Focus opponent's field
 * - H: Focus your hand
 */
UCLASS()
class GUNDAMTCG_API AGCGPlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	AGCGPlayerPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ===== COMPONENTS =====

	/** Root component - represents table center */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* TableCenter;

	/** Spring arm for smooth camera movement */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* CameraArm;

	/** Main camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* MainCamera;

	// ===== CAMERA PRESETS =====

	/** All available camera presets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Presets")
	TMap<FName, FGCGCameraPreset> CameraPresets;

	/** Currently active preset */
	UPROPERTY(BlueprintReadOnly, Category = "Camera|Presets")
	FName CurrentPresetName;

	/** Are we transitioning between presets? */
	UPROPERTY(BlueprintReadOnly, Category = "Camera|Presets")
	bool bIsTransitioning;

	/** Target preset we're transitioning to */
	FGCGCameraPreset TargetPreset;

	/** Transition progress (0-1) */
	float TransitionAlpha;

	/** Transition speed */
	float TransitionSpeed;

	// ===== CAMERA SETTINGS =====

	/** Default camera preset to use at game start */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	FName DefaultPresetName;

	/** Minimum zoom distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float MinZoomDistance;

	/** Maximum zoom distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float MaxZoomDistance;

	/** Camera orbit speed (degrees/second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float OrbitSpeed;

	/** Camera pan speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float PanSpeed;

	/** Enable camera lag for smoother movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	bool bEnableCameraLag;

	/** Camera lag speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float CameraLagSpeed;

	// ===== CAMERA CONTROL FUNCTIONS =====

	/**
	 * Transition to a named camera preset
	 * @param PresetName Name of preset to transition to
	 * @param Duration Override transition duration (0 = instant, -1 = use preset speed)
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void TransitionToPreset(FName PresetName, float Duration = -1.0f);

	/**
	 * Instantly snap to a camera preset
	 * @param PresetName Name of preset
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SnapToPreset(FName PresetName);

	/**
	 * Reset camera to default view
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ResetCamera();

	/**
	 * Orbit camera around table center
	 * @param DeltaYaw Yaw change in degrees
	 * @param DeltaPitch Pitch change in degrees
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void OrbitCamera(float DeltaYaw, float DeltaPitch);

	/**
	 * Zoom camera in/out
	 * @param ZoomDelta Amount to zoom (positive = zoom in, negative = zoom out)
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ZoomCamera(float ZoomDelta);

	/**
	 * Pan camera (move pivot point)
	 * @param DeltaX Horizontal pan
	 * @param DeltaY Vertical pan
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void PanCamera(float DeltaX, float DeltaY);

	/**
	 * Focus camera on a specific world location
	 * @param TargetLocation Location to focus on
	 * @param Duration Transition duration
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void FocusOnLocation(FVector TargetLocation, float Duration = 0.5f);

	/**
	 * Focus camera on opponent's field
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void FocusOpponentField();

	/**
	 * Focus camera on player's hand
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void FocusPlayerHand();

	// ===== INPUT HANDLERS =====

	/** Input: Orbit camera (right mouse button held) */
	void Input_OrbitCamera(float AxisValueYaw, float AxisValuePitch);

	/** Input: Zoom camera (mouse wheel) */
	void Input_ZoomCamera(float AxisValue);

	/** Input: Pan camera (middle mouse button held) */
	void Input_PanCamera(float AxisValueX, float AxisValueY);

	/** Input: Reset camera to default */
	void Input_ResetCamera();

	/** Input: Focus opponent field */
	void Input_FocusOpponent();

	/** Input: Focus player hand */
	void Input_FocusHand();

	// ===== HELPER FUNCTIONS =====

	/**
	 * Initialize default camera presets
	 */
	void InitializeDefaultPresets();

	/**
	 * Update camera transition
	 * @param DeltaTime Time since last frame
	 */
	void UpdateCameraTransition(float DeltaTime);

	/**
	 * Apply a camera preset immediately
	 * @param Preset Preset to apply
	 */
	void ApplyCameraPreset(const FGCGCameraPreset& Preset);

	/**
	 * Clamp camera values to valid ranges
	 */
	void ClampCameraValues();

	// ===== BLUEPRINT EVENTS =====

	/**
	 * Called when camera starts transitioning to new preset
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Camera|Events")
	void OnCameraTransitionStarted(FName FromPreset, FName ToPreset);

	/**
	 * Called when camera finishes transitioning
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Camera|Events")
	void OnCameraTransitionCompleted(FName PresetName);

private:
	/** Store starting preset when transitioning */
	FGCGCameraPreset StartingPreset;

	/** Current camera orbit angles */
	float CurrentOrbitYaw;
	float CurrentOrbitPitch;

	/** Current zoom level */
	float CurrentZoom;

	/** Current pan offset */
	FVector CurrentPanOffset;
};
